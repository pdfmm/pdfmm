/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfVecObjects.h"

#include <algorithm>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfReference.h"
#include "PdfStream.h"

using namespace std;
using namespace PoDoFo;

#define MAX_XREF_GEN_NUM 65535

static bool CompareObject(const PdfObject* obj1, const PdfObject* obj2);
static bool CompareReference(const PdfObject* obj, const PdfReference& ref);

struct ObjectComparatorPredicate
{
public:
    inline bool operator()(const PdfObject* const& pObj, const PdfObject* const& pObj2) const
    {
        return pObj->GetIndirectReference() < pObj2->GetIndirectReference();
    }
};

struct ReferenceComparatorPredicate
{
public:
    inline bool operator()( const PdfReference & pObj, const PdfReference & pObj2 ) const
    {
        return pObj < pObj2;
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
        return p1 ? (p1->GetIndirectReference() == m_ref ) : false;
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
size_t PdfVecObjects::m_MaxReserveSize = static_cast<size_t>(8388607); // cf. Table C.1 in section C.2 of PDF32000_2008.pdf

PdfVecObjects::PdfVecObjects(PdfDocument& document) :
    m_Document(&document),
    m_CanReuseObjectNumbers(true),
    m_ObjectCount(1),
    m_sorted(true),
    m_StreamFactory(nullptr)
{
}

PdfVecObjects::~PdfVecObjects()
{
    Clear();
}

void PdfVecObjects::Clear()
{
    for (auto obj : m_vector)
        delete obj;

    m_vector.clear();
    m_ObjectCount = 1;
    m_sorted = true;
    m_StreamFactory = nullptr;
}

PdfObject* PdfVecObjects::GetObject(const PdfReference& ref) const
{
    const_cast<PdfVecObjects&>(*this).Sort();
    TCIVecObjects it = std::lower_bound(m_vector.begin(), m_vector.end(), ref, CompareReference);
    if (it == m_vector.end() || (*it)->GetIndirectReference() != ref)
        return nullptr;

    return *it;
}

unique_ptr<PdfObject> PdfVecObjects::RemoveObject(const PdfReference& ref, bool markAsFree)
{
    const_cast<PdfVecObjects&>(*this).Sort();
    TCIVecObjects it = std::lower_bound(m_vector.begin(), m_vector.end(), ref, CompareReference);
    if (it == m_vector.end() || (*it)->GetIndirectReference() != ref)
        return nullptr;

    auto pObj = *(it);
    if (markAsFree)
        SafeAddFreeObject(pObj->GetIndirectReference());

    m_vector.erase(it);
    m_sorted = false;
    return unique_ptr<PdfObject>(pObj);
}

unique_ptr<PdfObject> PdfVecObjects::RemoveObject(const TIVecObjects& it)
{
    auto pObj = *it;
    m_vector.erase(it);
    m_sorted = false;
    return unique_ptr<PdfObject>(pObj);
}

PdfReference PdfVecObjects::getNextFreeObject()
{
    // Try to first use list of free objects
    if (m_CanReuseObjectNumbers && !m_lstFreeObjects.empty())
    {
        PdfReference freeObjectRef = m_lstFreeObjects.front();
        m_lstFreeObjects.pop_front();
        return freeObjectRef;
    }

    // If no free objects are available, create a new object with generation 0
    uint16_t nextObjectNum = static_cast<uint16_t>(m_ObjectCount);
    while (true)
    {
        if ((size_t)(nextObjectNum + 1) == m_MaxReserveSize)
            PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Reached the maximum number of indirect objects");

        // Check also if the object number it not available,
        // e.g. it reached maximum generation number (65535)
        if (m_lstUnavailableObjects.find(nextObjectNum) == m_lstUnavailableObjects.end())
            break;

        nextObjectNum++;
    }

    return PdfReference(nextObjectNum, 0);
}

PdfObject* PdfVecObjects::CreateDictionaryObject(const string_view& type)
{
    auto dict = PdfDictionary();
    if (!type.empty())
        dict.AddKey(PdfName::KeyType, PdfName(type));

    auto ret = new PdfObject(dict, true);
    addNewObject(ret);
    return ret;
}

PdfObject* PdfVecObjects::CreateObject(const PdfVariant& variant)
{
    auto ret = new PdfObject(variant, true);
    addNewObject(ret);
    return ret;
}

int32_t PdfVecObjects::SafeAddFreeObject(const PdfReference& reference)
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

bool PdfVecObjects::TryAddFreeObject(const PdfReference& reference)
{
    return tryAddFreeObject(reference.ObjectNumber(), reference.GenerationNumber()) != -1;
}

int32_t PdfVecObjects::tryAddFreeObject(uint32_t objnum, uint32_t gennum)
{
    // Documentation 3.4.3 Cross-Reference Table states: "The maximum
    // generation number is 65535; when a cross reference entry reaches
    // this value, it is never reused."
    // NOTE: gennum is uint32 to accomodate overflows from callers
    if (gennum >= MAX_XREF_GEN_NUM)
    {
        m_lstUnavailableObjects.insert(gennum);
        return -1;
    }

    AddFreeObject(PdfReference(objnum, gennum));
    return gennum;
}

void PdfVecObjects::AddFreeObject(const PdfReference& reference)
{
    std::pair<TIPdfReferenceList, TIPdfReferenceList> it =
        std::equal_range(m_lstFreeObjects.begin(), m_lstFreeObjects.end(), reference, ReferenceComparatorPredicate());

    if (it.first != it.second && !m_lstFreeObjects.empty())
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
        m_lstFreeObjects.insert(it.first, reference);
    }
}

void PdfVecObjects::PushObject(const PdfReference& ref, PdfObject* obj)
{
    if (GetObject(ref))
    {
        PdfError::LogMessage(LogSeverity::Warning, "Object: %" PDF_FORMAT_INT64 " 0 R will be deleted and loaded again.", ref.ObjectNumber());
        RemoveObject(ref, false);
    }

    obj->SetIndirectReference(ref);
    AddObject(obj);
}

void PdfVecObjects::addNewObject(PdfObject* obj)
{
    PdfReference ref = getNextFreeObject();
    obj->SetIndirectReference(ref);
    obj->SetDocument(*m_Document);
    AddObject(obj);
}

void PdfVecObjects::AddObject(PdfObject* obj)
{
    SetObjectCount(obj->GetIndirectReference());
    obj->SetDocument(*m_Document);
    m_vector.push_back(obj);
    m_sorted = false;
}

void PdfVecObjects::Sort()
{
    if (m_sorted)
        return;

    std::sort(m_vector.begin(), m_vector.end(), CompareObject);
    m_sorted = true;
}

#pragma region Untested

void PdfVecObjects::CollectGarbage(PdfObject& trailer)
{
    // We do not have any objects that have
    // to be on the top, like in a linearized PDF.
    // So we just use an empty list.
    TPdfReferenceSet setLinearizedGroup;
    this->RenumberObjects(trailer, &setLinearizedGroup, true);
}

void PdfVecObjects::RenumberObjects(PdfObject& trailer, TPdfReferenceSet* notDelete, bool doGarbageCollection)
{
    throw runtime_error("Fixme, we don't support taking address of PdfReference anymore. See InsertOneReferenceIntoVector");

    TVecReferencePointerList list;
    TIVecReferencePointerList it;
    TIReferencePointerList itList;
    int i = 0;

    m_lstFreeObjects.clear();

    const_cast<PdfVecObjects&>(*this).Sort();

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
        m_vector[i]->m_IndirectReference = ref;

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

void PdfVecObjects::insertOneReferenceIntoVector(const PdfObject& obj, TVecReferencePointerList& list)
{
    (void)list;
    throw runtime_error("Fixme, we don't support taking address of PdfReference anymore");

    /* FIX-ME
    PODOFO_RAISE_LOGIC_IF( !m_bSorted,
                           "PdfVecObjects must be sorted before calling PdfVecObjects::InsertOneReferenceIntoVector!" );

    // we asume that pObj is a reference - no checking here because of speed
    std::pair<TCIVecObjects,TCIVecObjects> it =
        std::equal_range( m_vector.begin(), m_vector.end(), &obj, ObjectComparatorPredicate() );

    if( it.first != it.second )
    {
        // ignore this reference
        return;
    }

    size_t index = it.first - m_vector.begin();
    list[index].push_back( const_cast<PdfReference*>(&(obj.GetReference() )) );
    */
}

void PdfVecObjects::insertReferencesIntoVector(const PdfObject& obj, TVecReferencePointerList& list)
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

void PdfVecObjects::GetObjectDependencies(const PdfObject& obj, TPdfReferenceList& list) const
{
    if (obj.IsReference())
    {
        std::pair<TPdfReferenceList::iterator, TPdfReferenceList::iterator> itEqualRange
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

void PdfVecObjects::buildReferenceCountVector(TVecReferencePointerList& list)
{
    TCIVecObjects it = m_vector.begin();

    list.clear();
    list.resize(!m_vector.empty());

    while (it != m_vector.end())
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

void PdfVecObjects::garbageCollection(TVecReferencePointerList& list, PdfObject&, TPdfReferenceSet* notDelete)
{
    TIVecReferencePointerList it = list.begin();
    int pos = 0;
    bool contains = false;

    while (it != list.end())
    {
        contains = notDelete ? (notDelete->find(m_vector[pos]->GetIndirectReference()) != notDelete->end()) : false;
        if (it->size() == 0 && !contains)
        {
            m_vector.erase(m_vector.begin() + pos);
        }

        pos++;
        it++;
    }

    m_ObjectCount = pos++;
}

#pragma endregion

void PdfVecObjects::Detach(Observer* observer)
{
    TIVecObservers it = m_vecObservers.begin();

    while (it != m_vecObservers.end())
    {
        if (*it == observer)
        {
            m_vecObservers.erase(it);
            break;
        }
        else
        {
            it++;
        }
    }
}

PdfStream* PdfVecObjects::CreateStream(PdfObject& parent)
{
    PdfStream* stream = m_StreamFactory == nullptr ?
        new PdfMemStream(parent) :
        m_StreamFactory->CreateStream(parent);

    return stream;
}

void PdfVecObjects::WriteObject(PdfObject& obj)
{
    // Tell any observers that there are new objects to write
    for (auto& observer : m_vecObservers)
        observer->WriteObject(obj);
}

void PdfVecObjects::Finish()
{
    // always work on a copy of the vector
    // in case a child invalidates our iterators
    // with a call to attach or detach.
    TVecObservers copy(m_vecObservers);
    for (auto& observer : copy)
        observer->Finish();
}

void PdfVecObjects::BeginAppendStream(const PdfStream& stream)
{
    for (auto& observer : m_vecObservers)
        observer->BeginAppendStream(stream);
}
    
void PdfVecObjects::EndAppendStream(const PdfStream& stream)
{
    for (auto& observer : m_vecObservers)
        observer->EndAppendStream(stream);
}

void PdfVecObjects::SetCanReuseObjectNumbers(bool canReuseObjectNumbers)
{
    m_CanReuseObjectNumbers = canReuseObjectNumbers;

    if (!m_CanReuseObjectNumbers)
        m_lstFreeObjects.clear();
}

size_t PdfVecObjects::GetSize() const
{
    return m_vector.size();
}

void PdfVecObjects::Reserve(size_t size)
{
    if (size <= m_MaxReserveSize) // Fix CVE-2018-5783
    {
        m_vector.reserve(size);
    }
    else
    {
        PdfError::DebugMessage("Call to PdfVecObjects::Reserve with %"
            PDF_SIZE_FORMAT" is over allowed limit of %"
            PDF_SIZE_FORMAT".\n", size, m_MaxReserveSize);
    }
}

void PdfVecObjects::Attach(Observer* observer)
{
    m_vecObservers.push_back(observer);
}

void PdfVecObjects::SetStreamFactory(StreamFactory* factory)
{
    m_StreamFactory = factory;
}

PdfObject* PdfVecObjects::GetBack()
{
    return m_vector.back();
}

void PdfVecObjects::SetObjectCount(const PdfReference& ref)
{
    if (ref.ObjectNumber() >= m_ObjectCount)
    {
        // "m_bObjectCount" is used for the next free object number.
        // We need to use the greatest object number + 1 for the next free object number.
        // Otherwise, object number overlap would have occurred.
        m_ObjectCount = ref.ObjectNumber() + 1;
    }
}

PdfObject*& PdfVecObjects::operator[](size_t index) { return m_vector[index]; }

TCIVecObjects PdfVecObjects::begin() const
{
    const_cast<PdfVecObjects&>(*this).Sort();
    return m_vector.begin();
}

TCIVecObjects PdfVecObjects::end() const
{
    return m_vector.end();
}

size_t PdfVecObjects::size() const
{
    return m_vector.size();
}

bool CompareObject(const PdfObject* p1, const PdfObject* p2)
{
    return *p1 < *p2;
}

bool CompareReference(const PdfObject* obj, const PdfReference& ref)
{
    return obj->GetIndirectReference() < ref;
}
