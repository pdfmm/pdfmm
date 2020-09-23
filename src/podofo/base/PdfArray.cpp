/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

PdfArray::PdfArray()
{
}

PdfArray::PdfArray( const PdfObject &var )
{
    this->push_back( var );
}

PdfArray::PdfArray(const PdfArray & rhs)
    : PdfContainerDataType(rhs), m_objects(rhs.m_objects)
{
}

void PdfArray::RemoveAt(int index)
{
    if (index < 0 || index >= (int)m_objects.size())
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Index is out of bounds");

    m_objects.erase(m_objects.begin() + index);
}

PdfObject & PdfArray::findAt(int index) const
{
    if (index < 0 || index >= (int)m_objects.size())
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Index is out of bounds");

    PdfObject &obj = const_cast<PdfArray *>(this)->m_objects[index];
    if ( obj.IsReference() )
        return GetIndirectObject( obj.GetReference() );
    else
        return obj;
}

void PdfArray::clear()
{
    AssertMutable();
    if ( m_objects.size() == 0 )
        return;

    m_objects.clear();
    SetDirty();
}

PdfArray::iterator PdfArray::insert( const iterator &pos, const PdfObject &val )
{
    AssertMutable();

    SetDirty();
    iterator ret = m_objects.insert( pos, val );
    auto document = GetObjectDocument();
    if (document != nullptr)
        ret->SetDocument(*document);
    return ret;
}

void PdfArray::erase( const iterator &pos )
{
    AssertMutable();

    m_objects.erase( pos );
    SetDirty();
}

void PdfArray::erase( const iterator &first, const iterator &last )
{
    AssertMutable();

    m_objects.erase( first, last );
    SetDirty();
}
 
PdfArray& PdfArray::operator=( const PdfArray &rhs )
{
    if (this != &rhs)
    {
        m_objects = rhs.m_objects;
        this->PdfContainerDataType::operator=( rhs );
    }
    else
    {
        //do nothing
    }
    
    return *this;
}

void PdfArray::resize(size_t count, const PdfObject &val)
{
    AssertMutable();

    size_t currentSize = m_objects.size();
    m_objects.resize( count, val );
    auto document = GetObjectDocument();
    if (document != nullptr)
    {
        for ( size_t i = currentSize; i < count; i++ )
            m_objects[i].SetDocument(*document);
    }

    if (currentSize != count)
        SetDirty();
}

void PdfArray::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, 
                      const PdfEncrypt* pEncrypt ) const
{
    PdfArray::const_iterator it = this->begin();

    int count = 1;

    if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
    {
        pDevice->Print( "[ " );
    }
    else
    {
        pDevice->Print( "[" );
    }

    while( it != this->end() )
    {
        (*it).Write( pDevice, eWriteMode, pEncrypt );
        if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
        {
            pDevice->Print( (count % 10 == 0) ? "\n" : " " );
        }

        ++it;
        ++count;
    }

    pDevice->Print( "]" );
}

// TODO: IsDirty in a container should be modified automatically by its children??? YES! And stop on first parent not dirty
bool PdfArray::IsDirty() const
{
    // If the array itself is dirty
    // return immediately
    // otherwise check all children.
    if( PdfContainerDataType::IsDirty() )
        return true;

    PdfArray::const_iterator it(this->begin());
    while( it != this->end() )
    {
        if( (*it).IsDirty() )
            return true;

        ++it;
    }

    return false;
}

void PdfArray::ResetDirtyInternal()
{
    // Propagate state to all subclasses
    PdfArray::iterator it(this->begin());
    while( it != this->end() )
    {
        (*it).SetDirty(false);
        ++it;
    }
}

void PdfArray::SetOwner( PdfObject *pOwner )
{
    PdfContainerDataType::SetOwner( pOwner );
    auto document = pOwner->GetDocument();
    if (document != nullptr)
    {
        // Set owmership for all children
        PdfArray::iterator it = this->begin();
        PdfArray::iterator end = this->end();
        for ( ; it != end; it++ )
            it->SetDocument(*document);
    }
}

const PdfObject & PdfArray::FindAt(int idx) const
{
    return findAt(idx);
}

PdfObject & PdfArray::FindAt(int idx)
{
    return findAt(idx);
}

int PdfArray::GetSize() const
{
    return (int)m_objects.size();
}

void PdfArray::push_back(const PdfObject& var)
{
    insert(end(), var);
}

void PdfArray::Clear()
{
    clear();
}

size_t PdfArray::size() const
{
    return m_objects.size();
}

bool PdfArray::empty() const
{
    return m_objects.empty();
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
    return m_objects.begin();
}

PdfArray::const_iterator PdfArray::begin() const
{
    return m_objects.begin();
}

PdfArray::iterator PdfArray::end()
{
    return m_objects.end();
}

PdfArray::const_iterator PdfArray::end() const
{
    return m_objects.end();
}

PdfArray::reverse_iterator PdfArray::rbegin()
{
    return m_objects.rbegin();
}

PdfArray::const_reverse_iterator PdfArray::rbegin() const
{
    return m_objects.rbegin();
}

PdfArray::reverse_iterator PdfArray::rend()
{
    return m_objects.rend();
}

PdfArray::const_reverse_iterator PdfArray::rend() const
{
    return m_objects.rend();
}

void PdfArray::reserve(size_type n)
{
    AssertMutable();

    m_objects.reserve(n);
}

PdfObject& PdfArray::front()
{
    return m_objects.front();
}

const PdfObject& PdfArray::front() const
{
    return m_objects.front();
}

PdfObject& PdfArray::back()
{
    return m_objects.back();
}

const PdfObject& PdfArray::back() const
{
    return m_objects.back();
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
