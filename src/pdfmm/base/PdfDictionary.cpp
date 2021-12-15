/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfDictionary.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

PdfDictionary::PdfDictionary() { }

PdfDictionary::PdfDictionary(const PdfDictionary& rhs)
    : PdfDataContainer()
{
    this->operator=(rhs);
}

const PdfDictionary& PdfDictionary::operator=(const PdfDictionary& rhs)
{
    m_Map = rhs.m_Map;
    PdfDataContainer::operator=(rhs);
    return *this;
}

bool PdfDictionary::operator==(const PdfDictionary& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    return m_Map == rhs.m_Map;
}

bool PdfDictionary::operator!=(const PdfDictionary& rhs) const
{
    if (this != &rhs)
        return true;

    // We don't check owner
    return m_Map != rhs.m_Map;
}

void PdfDictionary::Clear()
{
    if (!m_Map.empty())
    {
        m_Map.clear();
        SetDirty();
    }
}

PdfObject& PdfDictionary::AddKey(const PdfName& key, const PdfObject& obj)
{
    return addKey(key, obj);
}

void PdfDictionary::AddKeyIndirect(const PdfName& key, const PdfObject* obj)
{
    if (obj == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall not be null");

    if (IsIndirectReferenceAllowed(*obj))
        (void)addKey(key, obj->GetIndirectReference());
    else
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Given object shall allow indirect insertion");
}

PdfObject& PdfDictionary::AddKeyIndirectSafe(const PdfName& key, const PdfObject& obj)
{
    if (IsIndirectReferenceAllowed(obj))
        return addKey(key, obj.GetIndirectReference());
    else
        return addKey(key, obj);
}

PdfObject& PdfDictionary::addKey(const PdfName& key, const PdfObject& obj)
{
    auto added = addKey(key, obj, false);
    if (added.second)
        SetDirty();

    return added.first->second;
}

pair<PdfDictionary::Map::iterator, bool> PdfDictionary::addKey(const PdfName& identifier, const PdfObject& obj, bool noDirtySet)
{
    // NOTE: Empty PdfNames are legal according to the PDF specification.
    // Don't check for it

    pair<Map::iterator, bool> inserted = m_Map.insert(std::make_pair(identifier, obj));
    if (!inserted.second)
    {
        if (noDirtySet)
            inserted.first->second.Assign(obj);
        else
            inserted.first->second = obj;
    }

    inserted.first->second.SetParent(this);
    auto document = GetObjectDocument();
    if (document != nullptr)
        inserted.first->second.SetDocument(*document);

    return inserted;
}

PdfObject* PdfDictionary::getKey(const PdfName& key) const
{
    if (key.IsNull())
        return nullptr;

    auto it = m_Map.find(key);
    if (it == m_Map.end())
        return nullptr;

    return &const_cast<PdfObject&>(it->second);
}

PdfObject* PdfDictionary::findKey(const PdfName& key) const
{
    PdfObject* obj = getKey(key);
    if (obj == nullptr)
        return nullptr;

    if (obj->IsReference())
        return &GetIndirectObject(obj->GetReference());
    else
        return obj;

    return nullptr;
}

PdfObject* PdfDictionary::findKeyParent(const PdfName& key) const
{
    PdfObject* obj = findKey(key);
    if (obj == nullptr)
    {
        PdfObject* parent = findKey("Parent");
        if (parent == nullptr)
        {
            return nullptr;
        }
        else
        {
            if (parent->IsDictionary())
                return parent->GetDictionary().findKeyParent(key);
            else
                return nullptr;
        }
    }
    else
    {
        return obj;
    }
}

bool PdfDictionary::HasKey(const PdfName& key) const
{
    // NOTE: Empty PdfNames are legal according to the PDF specification,
    // don't check for it
    return m_Map.find(key) != m_Map.end();
}

bool PdfDictionary::RemoveKey(const PdfName& identifier)
{
    Map::iterator found = m_Map.find(identifier);
    if (found == m_Map.end())
        return false;

    m_Map.erase(found);
    SetDirty();
    return true;
}

void PdfDictionary::Write(PdfOutputDevice& device, PdfWriteMode writeMode,
    const PdfEncrypt* encrypt) const
{
    if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
        device.Write("<<\n");
    else
        device.Write("<<");

    if (this->HasKey(PdfName::KeyType))
    {
        // Type has to be the first key in any dictionary
        if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
            device.Write("/Type ");
        else
            device.Write("/Type");

        this->getKey(PdfName::KeyType)->GetVariant().Write(device, writeMode, encrypt);

        if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
            device.Put('\n');
    }

    for (auto& pair : m_Map)
    {
        if (pair.first != PdfName::KeyType)
        {
            pair.first.Write(device, writeMode, nullptr);
            if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
                device.Put(' '); // write a separator

            pair.second.GetVariant().Write(device, writeMode, encrypt);
            if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
                device.Put('\n');
        }
    }

    device.Write(">>");
}

void PdfDictionary::ResetDirtyInternal()
{
    // Propagate state to all sub objects
    for (auto& pair : m_Map)
        pair.second.ResetDirty();
}

void PdfDictionary::SetOwner(PdfObject* owner)
{
    PdfDataContainer::SetOwner(owner);
    auto document = owner->GetDocument();

    // Set owmership for all children
    for (auto& pair : m_Map)
    {
        pair.second.SetParent(this);
        if (document != nullptr)
            pair.second.SetDocument(*document);
    }
}

const PdfObject* PdfDictionary::GetKey(const PdfName& key) const
{
    return getKey(key);
}

PdfObject* PdfDictionary::GetKey(const PdfName& key)
{
    return getKey(key);
}

const PdfObject* PdfDictionary::FindKey(const PdfName& key) const
{
    return findKey(key);
}

PdfObject* PdfDictionary::FindKey(const PdfName& key)
{
    return findKey(key);
}

const PdfObject& PdfDictionary::MustFindKey(const PdfName& key) const
{
    auto obj = findKey(key);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfDictionary::MustFindKey(const PdfName& key)
{
    auto obj = findKey(key);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

const PdfObject* PdfDictionary::FindKeyParent(const PdfName& key) const
{
    return findKeyParent(key);
}

PdfObject* PdfDictionary::FindKeyParent(const PdfName& key)
{
    return findKeyParent(key);
}

const PdfObject& PdfDictionary::MustFindKeyParent(const PdfName& key) const
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfDictionary::MustFindKeyParent(const PdfName& key)
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

unsigned PdfDictionary::GetSize() const
{
    return (unsigned)m_Map.size();
}

PdfDictionaryIndirectIterator PdfDictionary::GetIndirectIterator()
{
    return PdfDictionaryIndirectIterator(*this);
}

const PdfDictionaryIndirectIterator PdfDictionary::GetIndirectIterator() const
{
    return PdfDictionaryIndirectIterator(const_cast<PdfDictionary&>(*this));
}

const PdfObject& PdfDictionary::MustGetKey(const PdfName& key) const
{
    auto obj = getKey(key);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfObject& PdfDictionary::MustGetKey(const PdfName& key)
{
    auto obj = getKey(key);
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *obj;
}

PdfDictionary::iterator PdfDictionary::begin()
{
    return m_Map.begin();
}

PdfDictionary::iterator PdfDictionary::end()
{
    return m_Map.end();
}

PdfDictionary::const_iterator PdfDictionary::begin() const
{
    return m_Map.begin();
}

PdfDictionary::const_iterator PdfDictionary::end() const
{
    return m_Map.end();
}

size_t PdfDictionary::size() const
{
    return m_Map.size();
}

PdfDictionaryIndirectIterator::PdfDictionaryIndirectIterator(PdfDictionary& dict)
    : m_dict(&dict)
{
}

PdfDictionaryIndirectIterator::iterator PdfDictionaryIndirectIterator::begin()
{
    return iterator(m_dict->begin());
}

PdfDictionaryIndirectIterator::iterator PdfDictionaryIndirectIterator::end()
{
    return iterator(m_dict->end());
}

PdfDictionaryIndirectIterator::const_iterator PdfDictionaryIndirectIterator::begin() const
{
    return const_iterator(m_dict->begin());
}

PdfDictionaryIndirectIterator::const_iterator PdfDictionaryIndirectIterator::end() const
{
    return const_iterator(m_dict->end());
}
