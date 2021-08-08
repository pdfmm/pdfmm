/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfIndirectObjectList.h"

#include <algorithm>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfReference.h"
#include "PdfStream.h"

using namespace std;
using namespace mm;

#define MAX_XREF_GEN_NUM 65535

static bool CompareObject(const PdfObject* obj1, const PdfObject* obj2);
static bool CompareReference(const PdfObject* obj, const PdfReference& ref);

struct ObjectComparatorPredicate
{
public:
    inline bool operator()(const PdfObject* const& obj1, const PdfObject* const& obj2) const
    {
        return obj1->GetIndirectReference() < obj2->GetIndirectReference();
    }
};

struct ReferenceComparatorPredicate
{
public:
    inline bool operator()(const PdfReference& obj1, const PdfReference& obj2) const
    {
        return obj1 < obj2;
    }
};

//RG: 1) Should this class not be moved to the header file
class ObjectsComparator
{
public:
    ObjectsComparator(const PdfReference& ref)
        : m_ref(ref) { }

    bool operator()(const PdfObject* p1) const
    {
        return p1 ? (p1->GetIndirectReference() == m_ref) : false;
    }

private:
    ObjectsComparator() = delete;
    ObjectsComparator(const ObjectsComparator& rhs) = delete;
    ObjectsComparator& operator=(const ObjectsComparator& rhs) = delete;

private:
    const PdfReference m_ref;
};

// This is static, IMHO (mabri) different values per-instance could cause confusion.
// It has to be defined here because of the one-definition rule.
size_t PdfIndirectObjectList::m_MaxReserveSize = static_cast<size_t>(8388607); // cf. Table C.1 in section C.2 of PDF32000_2008.pdf

PdfIndirectObjectList::PdfIndirectObjectList(PdfDocument& document) :
    m_Document(&document),
    m_CanReuseObjectNumbers(true),
    m_ObjectCount(1),
    m_sorted(true),
    m_StreamFactory(nullptr)
{
}

PdfIndirectObjectList::~PdfIndirectObjectList()
{
    Clear();
}

void PdfIndirectObjectList::Clear()
{
    for (auto obj : m_Objects)
        delete obj;

    m_Objects.clear();
    m_ObjectCount = 1;
    m_sorted = true;
    m_StreamFactory = nullptr;
}

PdfObject& PdfIndirectObjectList::MustGetObject(const PdfReference& ref) const
{
    auto obj = GetObject(ref);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject* PdfIndirectObjectList::GetObject(const PdfReference& ref) const
{
    const_cast<PdfIndirectObjectList&>(*this).Sort();
    auto it = std::lower_bound(m_Objects.begin(), m_Objects.end(), ref, CompareReference);
    if (it == m_Objects.end() || (*it)->GetIndirectReference() != ref)
        return nullptr;

    return *it;
}

unique_ptr<PdfObject> PdfIndirectObjectList::RemoveObject(const PdfReference& ref, bool markAsFree)
{
    const_cast<PdfIndirectObjectList&>(*this).Sort();
    auto it = std::lower_bound(m_Objects.begin(), m_Objects.end(), ref, CompareReference);
    if (it == m_Objects.end() || (*it)->GetIndirectReference() != ref)
        return nullptr;

    auto obj = *it;
    if (markAsFree)
        SafeAddFreeObject(obj->GetIndirectReference());

    m_Objects.erase(it);
    m_sorted = false;
    return unique_ptr<PdfObject>(obj);
}

unique_ptr<PdfObject> PdfIndirectObjectList::RemoveObject(const iterator& it)
{
    auto obj = *it;
    m_Objects.erase(it);
    m_sorted = false;
    return unique_ptr<PdfObject>(obj);
}

PdfReference PdfIndirectObjectList::getNextFreeObject()
{
    // Try to first use list of free objects
    if (m_CanReuseObjectNumbers && !m_FreeObjects.empty())
    {
        PdfReference freeObjectRef = m_FreeObjects.front();
        m_FreeObjects.pop_front();
        return freeObjectRef;
    }

    // If no free objects are available, create a new object with generation 0
    uint16_t nextObjectNum = static_cast<uint16_t>(m_ObjectCount);
    while (true)
    {
        if ((size_t)(nextObjectNum + 1) == m_MaxReserveSize)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Reached the maximum number of indirect objects");

        // Check also if the object number it not available,
        // e.g. it reached maximum generation number (65535)
        if (m_UnavailableObjects.find(nextObjectNum) == m_UnavailableObjects.end())
            break;

        nextObjectNum++;
    }

    return PdfReference(nextObjectNum, 0);
}

PdfObject* PdfIndirectObjectList::CreateDictionaryObject(const string_view& type)
{
    auto dict = PdfDictionary();
    if (!type.empty())
        dict.AddKey(PdfName::KeyType, PdfName(type));

    auto ret = new PdfObject(dict, true);
    addNewObject(ret);
    return ret;
}

PdfObject* PdfIndirectObjectList::CreateObject(const PdfVariant& variant)
{
    auto ret = new PdfObject(variant, true);
    addNewObject(ret);
    return ret;
}

int32_t PdfIndirectObjectList::SafeAddFreeObject(const PdfReference& reference)
{
    // From 3.4.3 Cross-Reference Table:
    // "When an indirect object is deleted, its cross-reference
    // entry is marked free and it is added to the linked list
    // of free entries.The entry’s generation number is incremented by
    // 1 to indicate the generation number to be used the next time an
    // object with that object number is created. Thus, each time
    // the entry is reused, it is given a new generation number."
    return tryAddFreeObject(reference.ObjectNumber(), reference.GenerationNumber() + 1);
}

bool PdfIndirectObjectList::TryAddFreeObject(const PdfReference& reference)
{
    return tryAddFreeObject(reference.ObjectNumber(), reference.GenerationNumber()) != -1;
}

int32_t PdfIndirectObjectList::tryAddFreeObject(uint32_t objnum, uint32_t gennum)
{
    // Documentation 3.4.3 Cross-Reference Table states: "The maximum
    // generation number is 65535; when a cross reference entry reaches
    // this value, it is never reused."
    // NOTE: gennum is uint32 to accomodate overflows from callers
    if (gennum >= MAX_XREF_GEN_NUM)
    {
        m_UnavailableObjects.insert(gennum);
        return -1;
    }

    AddFreeObject(PdfReference(objnum, gennum));
    return gennum;
}

void PdfIndirectObjectList::AddFreeObject(const PdfReference& reference)
{
    auto it = std::equal_range(m_FreeObjects.begin(), m_FreeObjects.end(), reference, ReferenceComparatorPredicate());
    if (it.first != it.second && !m_FreeObjects.empty())
    {
        // Be sure that no reference is added twice to free list
        PdfError::DebugMessage("Adding %d to free list, is already contained in it!", reference.ObjectNumber());
        return;
    }
    else
    {
        // When append free objects from external doc we need plus one number objects
        SetObjectCount(reference);

        // Insert so that list stays sorted
        m_FreeObjects.insert(it.first, reference);
    }
}

void PdfIndirectObjectList::PushObject(const PdfReference& ref, PdfObject* obj)
{
    if (GetObject(ref))
    {
        PdfError::LogMessage(LogSeverity::Warning, "Object: %" PDF_FORMAT_INT64 " 0 R will be deleted and loaded again.", ref.ObjectNumber());
        RemoveObject(ref, false);
    }

    obj->SetIndirectReference(ref);
    AddObject(obj);
}

void PdfIndirectObjectList::addNewObject(PdfObject* obj)
{
    PdfReference ref = getNextFreeObject();
    obj->SetIndirectReference(ref);
    obj->SetDocument(*m_Document);
    AddObject(obj);
}

void PdfIndirectObjectList::AddObject(PdfObject* obj)
{
    SetObjectCount(obj->GetIndirectReference());
    obj->SetDocument(*m_Document);
    m_Objects.push_back(obj);
    m_sorted = false;
}

void PdfIndirectObjectList::Sort()
{
    if (m_sorted)
        return;

    std::sort(m_Objects.begin(), m_Objects.end(), CompareObject);
    m_sorted = true;
}

#pragma region Untested

void PdfIndirectObjectList::CollectGarbage(PdfObject& trailer)
{
    // We do not have any objects that have
    // to be on the top, like in a linearized PDF.
    // So we just use an empty list.
    TPdfReferenceSet setLinearizedGroup;
    this->RenumberObjects(trailer, &setLinearizedGroup, true);
}

void PdfIndirectObjectList::RenumberObjects(PdfObject& trailer, TPdfReferenceSet* notDelete, bool doGarbageCollection)
{
    throw runtime_error("Fixme, we don't support taking address of PdfReference anymore. See InsertOneReferenceIntoVector");

    TVecReferencePointerList list;
    TIVecReferencePointerList it;
    TIReferencePointerList itList;
    int i = 0;

    m_FreeObjects.clear();

    const_cast<PdfIndirectObjectList&>(*this).Sort();

    // The following call slows everything down
    // optimization welcome
    buildReferenceCountVector(list);
    insertReferencesIntoVector(trailer, list);

    if (doGarbageCollection)
        garbageCollection(list, trailer, notDelete);

    it = list.begin();
    while (it != list.end())
    {
        PdfReference ref(i + 1, 0);
        m_Objects[i]->m_IndirectReference = ref;

        itList = it->begin();
        while (itList != it->end())
        {
            **itList = ref;
            itList++;
        }

        i++;
        it++;
    }
}

void PdfIndirectObjectList::insertOneReferenceIntoVector(const PdfObject& obj, TVecReferencePointerList& list)
{
    (void)list;
    throw runtime_error("FIX-ME, we don't support taking address of PdfReference anymore");

    /* FIX-ME
    PDFMM_RAISE_LOGIC_IF( !m_Sorted,
                           "PdfIndirectObjectList must be sorted before calling PdfIndirectObjectList::InsertOneReferenceIntoVector!" );

    // we asume that obj is a reference - no checking here because of speed
    auto std::equal_range( m_Objects.begin(), m_Objects.end(), &obj, ObjectComparatorPredicate());
    if( it.first != it.second )
    {
        // ignore this reference
        return;
    }

    size_t index = it.first - m_Objects.begin();
    list[index].push_back( const_cast<PdfReference*>(&(obj.GetReference() )) );
    */
}

void PdfIndirectObjectList::insertReferencesIntoVector(const PdfObject& obj, TVecReferencePointerList& list)
{
    if (obj.IsReference())
    {
        insertOneReferenceIntoVector(obj, list);
    }
    else if (obj.IsArray())
    {
        for (auto& child : obj.GetArray())
        {
            if (child.IsReference())
                insertOneReferenceIntoVector(child, list);
            else if (child.IsArray() || child.IsDictionary())
                insertReferencesIntoVector(child, list);
        }
    }
    else if (obj.IsDictionary())
    {
        for (auto& pair : obj.GetDictionary())
        {
            if (pair.second.IsReference())
                insertOneReferenceIntoVector(pair.second, list);
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            else if (pair.second.IsArray() ||
                pair.second.IsDictionary())
                insertReferencesIntoVector(pair.second, list);
        }
    }
}

void PdfIndirectObjectList::GetObjectDependencies(const PdfObject& obj, PdfReferenceList& list) const
{
    if (obj.IsReference())
    {
        std::pair<PdfReferenceList::iterator, PdfReferenceList::iterator> itEqualRange
            = std::equal_range(list.begin(), list.end(), obj.GetReference());
        if (itEqualRange.first == itEqualRange.second)
        {
            list.insert(itEqualRange.first, obj.GetReference());

            const PdfObject* referencedObject = this->GetObject(obj.GetReference());
            if (referencedObject != nullptr)
            {
                GetObjectDependencies(*referencedObject, list);
            }
        }
    }
    else if (obj.IsArray())
    {
        for (auto& child : obj.GetArray())
        {
            if (child.IsArray() || child.IsDictionary() || child.IsReference())
                GetObjectDependencies(child, list);
        }
    }
    else if (obj.IsDictionary())
    {
        for (auto& pair : obj.GetDictionary())
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            if (pair.second.IsArray() ||
                pair.second.IsDictionary() ||
                pair.second.IsReference())
                GetObjectDependencies(pair.second, list);
        }
    }
}

void PdfIndirectObjectList::buildReferenceCountVector(TVecReferencePointerList& list)
{
    auto it = m_Objects.begin();

    list.clear();
    list.resize(!m_Objects.empty());

    while (it != m_Objects.end())
    {
        auto& obj = **it;
        if (obj.IsReference())
        {
            insertOneReferenceIntoVector(obj, list);
        }
        else if (obj.IsArray() || obj.IsDictionary())
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            insertReferencesIntoVector(obj, list);
        }

        it++;
    }
}

void PdfIndirectObjectList::garbageCollection(TVecReferencePointerList& list, PdfObject&, TPdfReferenceSet* notDelete)
{
    TIVecReferencePointerList it = list.begin();
    int pos = 0;
    bool contains = false;

    while (it != list.end())
    {
        contains = notDelete ? (notDelete->find(m_Objects[pos]->GetIndirectReference()) != notDelete->end()) : false;
        if (it->size() == 0 && !contains)
        {
            m_Objects.erase(m_Objects.begin() + pos);
        }

        pos++;
        it++;
    }

    m_ObjectCount = pos++;
}

#pragma endregion

void PdfIndirectObjectList::Detach(Observer* observer)
{
    auto it = m_observers.begin();
    while (it != m_observers.end())
    {
        if (*it == observer)
        {
            m_observers.erase(it);
            break;
        }
        else
        {
            it++;
        }
    }
}

PdfStream* PdfIndirectObjectList::CreateStream(PdfObject& parent)
{
    PdfStream* stream = m_StreamFactory == nullptr ?
        new PdfMemStream(parent) :
        m_StreamFactory->CreateStream(parent);

    return stream;
}

void PdfIndirectObjectList::WriteObject(PdfObject& obj)
{
    // Tell any observers that there are new objects to write
    for (auto& observer : m_observers)
        observer->WriteObject(obj);
}

void PdfIndirectObjectList::Finish()
{
    // always work on a copy of the vector
    // in case a child invalidates our iterators
    // with a call to attach or detach.
    ObserverList copy(m_observers);
    for (auto& observer : copy)
        observer->Finish();
}

void PdfIndirectObjectList::BeginAppendStream(const PdfStream& stream)
{
    for (auto& observer : m_observers)
        observer->BeginAppendStream(stream);
}

void PdfIndirectObjectList::EndAppendStream(const PdfStream& stream)
{
    for (auto& observer : m_observers)
        observer->EndAppendStream(stream);
}

void PdfIndirectObjectList::SetCanReuseObjectNumbers(bool canReuseObjectNumbers)
{
    m_CanReuseObjectNumbers = canReuseObjectNumbers;

    if (!m_CanReuseObjectNumbers)
        m_FreeObjects.clear();
}

size_t PdfIndirectObjectList::GetSize() const
{
    return m_Objects.size();
}

void PdfIndirectObjectList::Reserve(size_t size)
{
    if (size <= m_MaxReserveSize) // Fix CVE-2018-5783
    {
        m_Objects.reserve(size);
    }
    else
    {
        PdfError::DebugMessage("Call to PdfIndirectObjectList::Reserve with %"
            PDF_SIZE_FORMAT" is over allowed limit of %"
            PDF_SIZE_FORMAT".\n", size, m_MaxReserveSize);
    }
}

void PdfIndirectObjectList::Attach(Observer* observer)
{
    m_observers.push_back(observer);
}

void PdfIndirectObjectList::SetStreamFactory(StreamFactory* factory)
{
    m_StreamFactory = factory;
}

PdfObject* PdfIndirectObjectList::GetBack()
{
    return m_Objects.back();
}

void PdfIndirectObjectList::SetObjectCount(const PdfReference& ref)
{
    if (ref.ObjectNumber() >= m_ObjectCount)
    {
        // "m_ObjectCount" is used for the next free object number.
        // We need to use the greatest object number + 1 for the next free object number.
        // Otherwise, object number overlap would have occurred.
        m_ObjectCount = ref.ObjectNumber() + 1;
    }
}

PdfObject*& PdfIndirectObjectList::operator[](size_t index) { return m_Objects[index]; }

PdfIndirectObjectList::iterator PdfIndirectObjectList::begin() const
{
    const_cast<PdfIndirectObjectList&>(*this).Sort();
    return m_Objects.begin();
}

PdfIndirectObjectList::iterator PdfIndirectObjectList::end() const
{
    return m_Objects.end();
}

size_t PdfIndirectObjectList::size() const
{
    return m_Objects.size();
}

bool CompareObject(const PdfObject* p1, const PdfObject* p2)
{
    return *p1 < *p2;
}

bool CompareReference(const PdfObject* obj, const PdfReference& ref)
{
    return obj->GetIndirectReference() < ref;
}
