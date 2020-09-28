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

#include "PdfDictionary.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

using namespace PoDoFo;

PdfDictionary::PdfDictionary() { }

PdfDictionary::PdfDictionary( const PdfDictionary & rhs )
    : PdfContainerDataType()
{
    this->operator=( rhs );
}

const PdfDictionary & PdfDictionary::operator=( const PdfDictionary & rhs )
{
    m_mapKeys = rhs.m_mapKeys;
    PdfContainerDataType::operator=( rhs );
    return *this;
}

bool PdfDictionary::operator==( const PdfDictionary& rhs ) const
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

    if( !m_mapKeys.empty() )
    {
        m_mapKeys.clear();
        SetDirty();
    }
}

PdfObject & PdfDictionary::AddKey( const PdfName & identifier, const PdfObject & rObject )
{
    AssertMutable();
    auto& added = addKey(identifier, rObject);
    SetDirty();
    return added;
}

void PdfDictionary::AddKey( const PdfName & identifier, const PdfObject* pObject )
{
    this->AddKey( identifier, *pObject );
}

PdfObject& PdfDictionary::addKey(const PdfName& identifier, const PdfObject& rObject)
{
    // NOTE: Empty PdfNames are legal according to the PDF specification.
    // Don't check for it

    std::pair<TKeyMap::iterator, bool> inserted = m_mapKeys.insert(std::make_pair(identifier, rObject));
    if (!inserted.second)
        inserted.first->second = rObject;

    inserted.first->second.SetParent(this);
    auto document = GetObjectDocument();
    if (document != nullptr)
        inserted.first->second.SetDocument(*document);

    return inserted.first->second;
}

PdfObject * PdfDictionary::getKey( const PdfName & key ) const
{
    if( !key.GetLength() )
        return nullptr;

    TCIKeyMap it = m_mapKeys.find( key );
    if( it == m_mapKeys.end() )
        return nullptr;

    return &const_cast<PdfObject &>( it->second );
}

PdfObject * PdfDictionary::findKey( const PdfName &key ) const
{
    PdfObject *obj = getKey( key );
    if (obj == nullptr)
        return nullptr;

    if (obj->IsReference())
        return &GetIndirectObject(obj->GetReference());
    else
        return obj;

    return nullptr;
}

PdfObject * PdfDictionary::findKeyParent( const PdfName & key ) const
{
    PdfObject *obj = findKey( key );
    if (obj == nullptr)
    {
        PdfObject *parent = findKey( "Parent" );
        if ( parent == nullptr )
        {
            return nullptr;
        }
        else
        {
            if ( parent->IsDictionary() )
                return parent->GetDictionary().findKeyParent( key );
            else
                return nullptr;
        }
    }
    else
    {
        return obj;
    }
}

bool PdfDictionary::GetKeyAsBool(const PdfName& key, bool defvalue) const
{
    auto pObject = findKey(key);
    bool ret;
    if (pObject == nullptr || !pObject->TryGetBool(ret))
        return defvalue;

    return ret;
}

int64_t PdfDictionary::GetKeyAsNumber( const PdfName & key, int64_t defvalue) const
{
    auto pObject = findKey(key);
    int64_t ret;
    if( pObject == nullptr || !pObject->TryGetNumber(ret))
        return defvalue;

    return ret;
}

int64_t PdfDictionary::GetKeyAsNumberLenient(const PdfName& key, int64_t defvalue) const
{
    auto pObject = findKey(key);
    int64_t ret;
    if (pObject == nullptr || !pObject->TryGetNumberLenient(ret))
        return defvalue;

    return ret;
}

double PdfDictionary::GetKeyAsReal(const PdfName & key, double defvalue) const
{
    auto pObject = findKey(key);
    double ret;
    if (pObject == nullptr || !pObject->TryGetReal(ret))
        return defvalue;

    return ret;
}

double PdfDictionary::GetKeyAsRealStrict(const PdfName& key, double defvalue) const
{
    auto pObject = findKey(key);
    double ret;
    if (pObject == nullptr || !pObject->TryGetRealStrict(ret))
        return defvalue;

    return ret;
}

PdfName PdfDictionary::GetKeyAsName(const PdfName & key, const PdfName& defvalue) const
{
    auto pObject = findKey(key);
    const PdfName* ret;
    if (pObject == nullptr || !pObject->TryGetName(ret))
        return defvalue;
    
    return *ret;
}

PdfString PdfDictionary::GetKeyAsString(const PdfName& key, const PdfString& defvalue) const
{
    auto pObject = findKey(key);
    const PdfString* ret;
    if (pObject == nullptr || !pObject->TryGetString(ret))
        return defvalue;

    return *ret;
}

PdfReference PdfDictionary::GetKeyAsReference(const PdfName& key, const PdfReference& defvalue) const
{
    auto pObject = findKey(key);
    PdfReference ret;
    if (pObject == nullptr || !pObject->TryGetReference(ret))
        return defvalue;

    return ret;
}

bool PdfDictionary::HasKey( const PdfName & key ) const
{
    // NOTE: Empty PdfNames are legal according to the PDF specification,
    // don't check for it
    return m_mapKeys.find( key ) != m_mapKeys.end();
}

bool PdfDictionary::RemoveKey( const PdfName & identifier )
{
    AssertMutable();

    TKeyMap::iterator found = m_mapKeys.find( identifier );
    if( found == m_mapKeys.end() )
        return false;

    m_mapKeys.erase( found );
    SetDirty();
    return true;
}

void PdfDictionary::Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode,
    const PdfEncrypt* pEncrypt) const
{
    TCIKeyMap     itKeys;

    if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
    {
        pDevice.Print( "<<\n" );
    } 
    else
    {
        pDevice.Print( "<<" );
    }
    itKeys     = m_mapKeys.begin();

    if( this->HasKey( PdfName::KeyType ) ) 
    {
        // Type has to be the first key in any dictionary
        if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
        {
            pDevice.Print( "/Type " );
        }
        else
        {
            pDevice.Print( "/Type" );
        }

        this->GetKey( PdfName::KeyType )->GetVariant().Write( pDevice, eWriteMode, pEncrypt );

        if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
        {
            pDevice.Print( "\n" );
        }
    }

    while( itKeys != m_mapKeys.end() )
    {
        if( itKeys->first != PdfName::KeyType )
        {
            itKeys->first.Write(pDevice, eWriteMode, nullptr);
            if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
            {
                pDevice.Write( " ", 1 ); // write a separator
            }
            itKeys->second.GetVariant().Write( pDevice, eWriteMode, pEncrypt );
            if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean ) 
            {
                pDevice.Write( "\n", 1 );
            }
        }
        
        ++itKeys;
    }

    pDevice.Print( ">>" );
}

void PdfDictionary::ResetDirtyInternal()
{
    // Propagate state to all sub objects
    TKeyMap::iterator it = m_mapKeys.begin();
    while( it != m_mapKeys.end() )
    {
        it->second.ResetDirty();
        ++it;
    }
}

TIKeyMap PdfDictionary::begin()
{
    return m_mapKeys.begin();
}

TIKeyMap PdfDictionary::end()
{
    return m_mapKeys.end();
}

TCIKeyMap PdfDictionary::begin() const
{
    return m_mapKeys.begin();
}

TCIKeyMap PdfDictionary::end() const
{
    return m_mapKeys.end();
}

void PdfDictionary::SetOwner( PdfObject *pOwner )
{
    PdfContainerDataType::SetOwner( pOwner );
    auto document = pOwner->GetDocument();

    // Set owmership for all children
    TIKeyMap it = this->begin();
    TIKeyMap end = this->end();
    for (; it != end; it++)
    {
        it->second.SetParent(this);
        if (document != nullptr)
            it->second.SetDocument(*document);
    }
}

const PdfObject * PdfDictionary::GetKey( const PdfName &key ) const
{
    return getKey(key);
}

PdfObject * PdfDictionary::GetKey( const PdfName &key )
{
    return getKey(key);
}

const PdfObject * PdfDictionary::FindKey( const PdfName &key ) const
{
    return findKey(key);
}

PdfObject * PdfDictionary::FindKey( const PdfName &key )
{
    return findKey(key);
}

const PdfObject* PdfDictionary::FindKeyParent( const PdfName &key ) const
{
    return findKeyParent(key);
}

PdfObject* PdfDictionary::FindKeyParent( const PdfName &key )
{
    return findKeyParent(key);
}

size_t PdfDictionary::GetSize() const
{
    return m_mapKeys.size();
}

const PdfObject& PdfDictionary::MustGetKey( const PdfName & key ) const
{
    const PdfObject* obj = GetKey( key );
    if (!obj)
        PODOFO_RAISE_ERROR( EPdfError::NoObject );
    return *obj;
}
