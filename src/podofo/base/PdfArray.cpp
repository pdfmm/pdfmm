/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfArray.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

#include <limits>

using namespace PoDoFo;

PdfArray::PdfArray() { }

PdfArray::PdfArray( const PdfObject &var )
{
    add(var);
}

PdfArray::PdfArray(const PdfArray & rhs)
    : PdfContainerDataType(rhs), m_objects(rhs.m_objects)
{
}

void PdfArray::RemoveAt(size_t index)
{
    // TODO: Set dirty only if really removed
    if (index >= m_objects.size())
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Index is out of bounds");

    m_objects.erase(m_objects.begin() + index);
    SetDirty();
}

const PdfObject& PdfArray::FindAt(unsigned idx) const
{
    return findAt(idx);
}

PdfObject& PdfArray::FindAt(unsigned idx)
{
    return findAt(idx);
}

unsigned PdfArray::GetSize() const
{
    return (unsigned)m_objects.size();
}

void PdfArray::Add(const PdfObject& obj)
{
    AssertMutable();
    add(obj);
    SetDirty();
}

void PdfArray::AddIndirect(const PdfObject& obj)
{
    if (IsIndirectReferenceAllowed(obj))
    {
        Add(obj.GetIndirectReference());
        return;
    }

    Add(obj);
    return;
}

void PdfArray::SetAt(const PdfObject& obj, size_t idx)
{
    AssertMutable();
    if (IsIndirectReferenceAllowed(obj))
    {
        m_objects.at(idx) = PdfObject(obj.GetIndirectReference());
        return;
    }

    m_objects.at(idx) = obj;
}

void PdfArray::SetAtIndirect(const PdfObject& obj, size_t idx)
{
    AssertMutable();
    m_objects.at(idx) = obj;
}

void PdfArray::Clear()
{
    clear();
}

void PdfArray::Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode,
    const PdfEncrypt* pEncrypt) const
{
    auto it = m_objects.begin();

    int count = 1;

    if ((eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean)
    {
        pDevice.Print("[ ");
    }
    else
    {
        pDevice.Print("[");
    }

    while (it != m_objects.end())
    {
        it->GetVariant().Write(pDevice, eWriteMode, pEncrypt);
        if ((eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean)
        {
            pDevice.Print((count % 10 == 0) ? "\n" : " ");
        }

        ++it;
        ++count;
    }

    pDevice.Print("]");
}

void PdfArray::ResetDirtyInternal()
{
    // Propagate state to all subclasses
    for (auto& obj : m_objects)
        obj.ResetDirty();
}

void PdfArray::SetOwner(PdfObject* pOwner)
{
    PdfContainerDataType::SetOwner(pOwner);
    auto document = pOwner->GetDocument();
    if (document != nullptr)
    {
        // Set owmership for all children
        for (auto& obj : m_objects)
            obj.SetDocument(*document);
    }
}

void PdfArray::add(const PdfObject& obj)
{
    insertAt(m_objects.end(), obj);
}

PdfArray::iterator PdfArray::insertAt(const iterator& pos, const PdfObject& val)
{
    auto ret = m_objects.insert(pos, val);
    ret->SetParent(this);
    auto document = GetObjectDocument();
    if (document != nullptr)
        ret->SetDocument(*document);

    return ret;
}

PdfObject& PdfArray::findAt(unsigned index) const
{
    if (index >= (unsigned)m_objects.size())
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Index is out of bounds");

    PdfObject& obj = const_cast<PdfArray&>(*this).m_objects[index];
    if (obj.IsReference())
        return GetIndirectObject(obj.GetReference());
    else
        return obj;
}

void PdfArray::push_back(const PdfObject& obj)
{
    AssertMutable();
    add(obj);
    SetDirty();
}

size_t PdfArray::size() const
{
    return m_objects.size();
}

bool PdfArray::empty() const
{
    return m_objects.empty();
}

void PdfArray::clear()
{
    AssertMutable();
    if (m_objects.size() == 0)
        return;

    m_objects.clear();
    SetDirty();
}

PdfArray::iterator PdfArray::insert(const iterator& pos, const PdfObject& obj)
{
    AssertMutable();
    auto ret = insertAt(pos, obj);
    SetDirty();
    return ret;
}

void PdfArray::erase(const iterator& pos)
{
    // TODO: Set dirty only if really removed
    AssertMutable();
    m_objects.erase(pos);
    SetDirty();
}

void PdfArray::erase(const iterator& first, const iterator& last)
{
    // TODO: Set dirty only if really removed
    AssertMutable();
    m_objects.erase(first, last);
    SetDirty();
}

void PdfArray::resize(size_t count, const PdfObject& val)
{
    AssertMutable();

    size_t currentSize = m_objects.size();
    m_objects.resize(count, val);
    auto document = GetObjectDocument();

    for (size_t i = currentSize; i < count; i++)
    {
        auto& obj = m_objects[i];
        obj.SetParent(this);
        if (document != nullptr)
            obj.SetDocument(*document);
    }

    if (currentSize != count)
        SetDirty();
}

void PdfArray::reserve(size_type n)
{
    AssertMutable();
    m_objects.reserve(n);
}

PdfObject& PdfArray::operator[](size_t index)
{
    AssertMutable();
    return m_objects.at(index);
}

const PdfObject& PdfArray::operator[](size_t index) const
{
    return m_objects.at(index);
}

PdfArray::iterator PdfArray::begin()
{
    AssertMutable();
    return m_objects.begin();
}

PdfArray::const_iterator PdfArray::begin() const
{
    return m_objects.begin();
}

PdfArray::iterator PdfArray::end()
{
    AssertMutable();
    return m_objects.end();
}

PdfArray::const_iterator PdfArray::end() const
{
    return m_objects.end();
}

PdfArray::reverse_iterator PdfArray::rbegin()
{
    AssertMutable();
    return m_objects.rbegin();
}

PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return m_objects.rbegin();
}

PdfArray::reverse_iterator PdfArray::rend()
{
    AssertMutable();
    return m_objects.rend();
}

PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return m_objects.rend();
}

PdfObject& PdfArray::front()
{
    AssertMutable();
    return m_objects.front();
}

const PdfObject& PdfArray::front() const
{
    return m_objects.front();
}

PdfObject& PdfArray::back()
{
    AssertMutable();
    return m_objects.back();
}

const PdfObject& PdfArray::back() const
{
    return m_objects.back();
}

PdfArray& PdfArray::operator=(const PdfArray& rhs)
{
    if (&rhs == this)
        return *this;

    m_objects = rhs.m_objects;
    PdfContainerDataType::operator=(rhs);
    return *this;
}

bool PdfArray::operator==(const PdfArray& rhs) const
{
    if (this == &rhs)
        return true;

    // We don't check owner
    return m_objects == rhs.m_objects;
}

bool PdfArray::operator!=(const PdfArray& rhs) const
{
    if (this == &rhs)
        return false;

    // We don't check owner
    return m_objects != rhs.m_objects;
}
