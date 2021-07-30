/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfDictionary.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace PoDoFo;

PdfDictionary::PdfDictionary() { }

PdfDictionary::PdfDictionary(const PdfDictionary& rhs)
{
    this->operator=(rhs);
}

const PdfDictionary& PdfDictionary::operator=(const PdfDictionary& rhs)
{
    m_mapKeys = rhs.m_mapKeys;
    PdfContainerDataType::operator=(rhs);
    return *this;
}

bool PdfDictionary::operator==(const PdfDictionary& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    return m_mapKeys == rhs.m_mapKeys;
}

bool PdfDictionary::operator!=(const PdfDictionary& rhs) const
{
    if (this != &rhs)
        return true;

    // We don't check owner
    return m_mapKeys != rhs.m_mapKeys;
}

void PdfDictionary::Clear()
{
    AssertMutable();

    if (!m_mapKeys.empty())
    {
        m_mapKeys.clear();
        SetDirty();
    }
}

PdfObject& PdfDictionary::AddKey(const PdfName& identifier, const PdfObject& obj)
{
    AssertMutable();
    auto added = addKey(identifier, obj, false);
    if (added.second)
        SetDirty();

    return added.first->second;
}

PdfObject& PdfDictionary::AddKeyIndirect(const PdfName& key, const PdfObject& obj)
{
    if (IsIndirectReferenceAllowed(obj))
        return AddKey(key, obj.GetIndirectReference());

    return AddKey(key, obj);
}

pair<PdfDictionary::Map::iterator, bool> PdfDictionary::addKey(const PdfName& identifier, const PdfObject& obj, bool noDirtySet)
{
    // NOTE: Empty PdfNames are legal according to the PDF specification.
    // Don't check for it

    pair<Map::iterator, bool> inserted = m_mapKeys.insert(std::make_pair(identifier, obj));
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
    if (!key.GetLength())
        return nullptr;

    auto it = m_mapKeys.find(key);
    if (it == m_mapKeys.end())
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
    return m_mapKeys.find(key) != m_mapKeys.end();
}

bool PdfDictionary::RemoveKey(const PdfName& identifier)
{
    AssertMutable();

    Map::iterator found = m_mapKeys.find(identifier);
    if (found == m_mapKeys.end())
        return false;

    m_mapKeys.erase(found);
    SetDirty();
    return true;
}

void PdfDictionary::Write(PdfOutputDevice& device, PdfWriteMode writeMode,
    const PdfEncrypt* encrypt) const
{
    if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
    {
        device.Print("<<\n");
    }
    else
    {
        device.Print("<<");
    }

    if (this->HasKey(PdfName::KeyType))
    {
        // Type has to be the first key in any dictionary
        if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
        {
            device.Print("/Type ");
        }
        else
        {
            device.Print("/Type");
        }

        this->GetKey(PdfName::KeyType)->GetVariant().Write(device, writeMode, encrypt);

        if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
        {
            device.Print("\n");
        }
    }

    for (auto& pair : m_mapKeys)
    {
        if (pair.first != PdfName::KeyType)
        {
            pair.first.Write(device, writeMode, nullptr);
            if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
            {
                device.Write(" ", 1); // write a separator
            }
            pair.second.GetVariant().Write(device, writeMode, encrypt);
            if ((writeMode & PdfWriteMode::Clean) == PdfWriteMode::Clean)
            {
                device.Write("\n", 1);
            }
        }
    }

    device.Print(">>");
}

void PdfDictionary::ResetDirtyInternal()
{
    // Propagate state to all sub objects
    for (auto& pair : m_mapKeys)
        pair.second.ResetDirty();
}

void PdfDictionary::SetOwner(PdfObject* owner)
{
    PdfContainerDataType::SetOwner(owner);
    auto document = owner->GetDocument();

    // Set owmership for all children
    for (auto& pair : m_mapKeys)
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

const PdfObject * PdfDictionary::FindKey( const PdfName &key ) const
{
    return findKey(key);
}

PdfObject* PdfDictionary::FindKey(const PdfName& key)
{
    return findKey(key);
}

const PdfObject* PdfDictionary::FindKeyParent(const PdfName& key) const
{
    return findKeyParent(key);
}

PdfObject* PdfDictionary::FindKeyParent(const PdfName& key)
{
    return findKeyParent(key);
}

size_t PdfDictionary::GetSize() const
{
    return m_mapKeys.size();
}

const PdfObject& PdfDictionary::MustGetKey(const PdfName& key) const
{
    const PdfObject* obj = GetKey(key);
    if (obj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *obj;
}

double PdfDictionary::GetAsRealStrict(const PdfName& key, double defvalue) const
{
    auto obj = getKey(key);
    if (obj == nullptr)
        return defvalue;

    return obj->GetRealStrict();
}

int64_t PdfDictionary::GetAsNumberLenient(const PdfName& key, int64_t defvalue) const
{
    auto obj = getKey(key);
    if (obj == nullptr)
        return defvalue;

    return obj->GetNumberLenient();
}

double PdfDictionary::FindAsRealStrict(const PdfName& key, double defvalue) const
{
    auto obj = findKey(key);
    if (obj == nullptr)
        return defvalue;

    return obj->GetRealStrict();
}

int64_t PdfDictionary::FindAsNumberLenient(const PdfName& key, int64_t defvalue) const
{
    auto obj = findKey(key);
    if (obj == nullptr)
        return defvalue;

    return obj->GetNumberLenient();
}

double PdfDictionary::FindParentAsRealStrict(const PdfName& key, double defvalue) const
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        return defvalue;

    return obj->GetRealStrict();
}

int64_t PdfDictionary::FindParentAsNumberLenient(const PdfName& key, int64_t defvalue) const
{
    auto obj = findKeyParent(key);
    if (obj == nullptr)
        return defvalue;

    return obj->GetNumberLenient();
}

PdfDictionary::iterator PdfDictionary::begin()
{
    return m_mapKeys.begin();
}

PdfDictionary::iterator PdfDictionary::end()
{
    return m_mapKeys.end();
}

PdfDictionary::const_iterator PdfDictionary::begin() const
{
    return m_mapKeys.begin();
}

PdfDictionary::const_iterator PdfDictionary::end() const
{
    return m_mapKeys.end();
}

size_t PdfDictionary::size() const
{
    return m_mapKeys.size();
}
