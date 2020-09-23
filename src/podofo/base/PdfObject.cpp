/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfObject.h"

#include <doc/PdfDocument.h>
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfFileStream.h"
#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"
#include "PdfMemStream.h"

#include <sstream>
#include <fstream>
#include <string.h>

using namespace std;

using namespace PoDoFo;

PdfObject::PdfObject()
    : m_Variant( PdfDictionary() )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfReference & rRef, const char* pszType )
    : m_Variant( PdfDictionary() ), m_reference( rRef )
{
    InitPdfObject();

    if( pszType )
        this->GetDictionary().AddKey( PdfName::KeyType, PdfName( pszType ) );
}

PdfObject::PdfObject( const PdfVariant & var )
    : m_Variant( var )
{
    InitPdfObject();
}

PdfObject::PdfObject( bool b )
    : m_Variant( b )
{
    InitPdfObject();
}

PdfObject::PdfObject( int64_t l )
    : m_Variant( l )
{
    InitPdfObject();
}

PdfObject::PdfObject( double d )
    : m_Variant( d )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfString & rsString )
    : m_Variant( rsString )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfName & rName )
    : m_Variant( rName )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfReference & rRef )
    : m_Variant( rRef )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfArray & tList )
    : m_Variant( tList )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfDictionary & rDict )
    : m_Variant( rDict )
{
    InitPdfObject();
}

// NOTE: Don't copy owner. Copied objects must be always detached.
// Ownership will be set automatically elsewhere. Also don't copy
// reference
PdfObject::PdfObject( const PdfObject & rhs ) 
    : m_Variant( rhs )
{
    InitPdfObject();
    copyFrom(rhs);
}

void PdfObject::ForceCreateStream()
{
    DelayedLoadStream();
    forceCreateStream();
}

void PdfObject::SetDocument(PdfDocument& document)
{
    if (m_Document == &document)
    {
        // The inner owner for variant data objects is guaranteed to be same
        return;
    }

    m_Document = &document;
    SetVariantOwner();
}


void PdfObject::DelayedLoad() const
{
    if (m_bDelayedLoadDone)
        return;

    const_cast<PdfObject&>(*this).DelayedLoadImpl();
    m_bDelayedLoadDone = true;
    const_cast<PdfObject&>(*this).SetVariantOwner();
}

void PdfObject::DelayedLoadImpl()
{
    // Default implementation of virtual void DelayedLoadImpl() throws, since delayed
    // loading should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

void PdfObject::SetVariantOwner()
{
    auto eDataType = m_Variant.GetDataType();
    switch ( eDataType )
    {
        case EPdfDataType::Dictionary:
            static_cast<PdfContainerDataType &>(m_Variant.GetDictionary()).SetOwner( this );
            break;
        case EPdfDataType::Array:
            static_cast<PdfContainerDataType &>(m_Variant.GetArray()).SetOwner( this );
            break;
        default:
            break;
    }
}

void PdfObject::FreeStream()
{
    m_pStream = nullptr;
}

void PdfObject::InitPdfObject()
{
    m_Document = nullptr;
    m_Parent = nullptr;
    m_bDirty = false;
    m_bImmutable = false;
    m_bDelayedLoadDone = true;
    m_DelayedLoadStreamDone = true;
    m_pStream = nullptr;
    SetVariantOwner();
}

void PdfObject::Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode,
                      PdfEncrypt* pEncrypt) const
{
    DelayedLoad();
    DelayedLoadStream();

    if( m_reference.IsIndirect() )
    {
        // CHECK-ME We want to make this in all the cases for PDF/A Compatibility
        //if( (eWriteMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean )
        {
            pDevice.Print( "%i %i obj\n", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        }
        //else
        //{
        //    pDevice.Print( "%i %i obj", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        //}
    }

    if(pEncrypt)
        pEncrypt->SetCurrentReference( m_reference );

    if( pEncrypt && m_pStream != nullptr )
    {
        // Set length if it is a key
        PdfFileStream* pFileStream = dynamic_cast<PdfFileStream*>(m_pStream.get());
        if( !pFileStream )
        {
            // PdfFileStream handles encryption internally
            size_t lLength = pEncrypt->CalculateStreamLength(m_pStream->GetLength());
            *(const_cast<PdfObject*>(this)->GetIndirectKey( PdfName::KeyLength )) = PdfObject(static_cast<int64_t>(lLength));
        }
    }

    m_Variant.Write(pDevice, eWriteMode, pEncrypt);
    pDevice.Print("\n");

    if( m_pStream )
        m_pStream->Write(pDevice, pEncrypt);

    if( m_reference.IsIndirect())
        pDevice.Print("endobj\n");
}

// REMOVE-ME
PdfObject* PdfObject::GetIndirectKey( const PdfName & key ) const
{
    if ( !this->IsDictionary() )
        return NULL;

    return const_cast<PdfObject*>( this->GetDictionary().FindKey( key ) );
}

// REMOVE-ME
PdfObject* PdfObject::MustGetIndirectKey(const PdfName & key) const
{
    PdfObject* obj = GetIndirectKey(key);
    if (!obj)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);
    return obj;
}

size_t PdfObject::GetObjectLength( EPdfWriteMode eWriteMode )
{
    PdfOutputDevice device;

    this->Write( device, eWriteMode, nullptr );

    return device.GetLength();
}

PdfStream & PdfObject::GetOrCreateStream()
{
    DelayedLoadStream();
    return getOrCreateStream();
}

const PdfStream* PdfObject::GetStream() const
{
    DelayedLoadStream();
    return m_pStream.get();
}

PdfStream* PdfObject::GetStream()
{
    DelayedLoadStream();
    return m_pStream.get();
}

bool PdfObject::HasStream() const
{
    DelayedLoadStream();
    return m_pStream != nullptr;
}

PdfStream & PdfObject::getOrCreateStream()
{
    forceCreateStream();
    return *m_pStream;
}

void PdfObject::forceCreateStream()
{
    if (m_pStream != nullptr)
        return;

    if (m_Variant.GetDataType() != EPdfDataType::Dictionary)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Tried to get stream of non-dictionary object");

    if (m_Document == nullptr)
        m_pStream.reset(new PdfMemStream(this));
    else
        m_pStream.reset(m_Document->GetObjects().CreateStream(this));
}

PdfStream * PdfObject::getStream()
{
    return m_pStream.get();
}

void PdfObject::DelayedLoadStream() const
{
    DelayedLoad();
    delayedLoadStream();

}

void PdfObject::delayedLoadStream() const
{
    if (!m_DelayedLoadStreamDone)
    {
        const_cast<PdfObject &>(*this).DelayedLoadStreamImpl();
        m_DelayedLoadStreamDone = true;
    }
}

void PdfObject::SetIndirectReference(const PdfReference &reference)
{
    m_reference = reference;
}

void PdfObject::FlateCompressStream() 
{
    // TODO: If the stream isn't already in memory, defer loading and compression until first read of the stream to save some memory.
    DelayedLoadStream();

    //if( m_pStream )
    //    m_pStream->FlateCompress();
}

const PdfObject & PdfObject::operator=(const PdfObject & rhs)
{
    if (&rhs == this)
        return *this;

    rhs.DelayedLoad();
    m_Variant = rhs.m_Variant;
    copyFrom(rhs);
    SetDirty(true);
    return *this;
}

// NOTE: Don't copy owner. Objects being assigned always keep current ownership
void PdfObject::copyFrom(const PdfObject & rhs)
{
    // NOTE: Don't call rhs.DelayedLoad() here. It's implicitly
    // called in PdfVariant assignment or copy constructor
    rhs.delayedLoadStream();
    m_reference = rhs.m_reference;
    SetVariantOwner();

    if (rhs.m_pStream)
    {
        auto &stream = getOrCreateStream();
        stream = *rhs.m_pStream;
    }

    // Assume the delayed load of the stream is performed
    m_DelayedLoadStreamDone = true;
}

void PdfObject::EnableDelayedLoadingStream()
{
    m_DelayedLoadStreamDone = false;
}

void PdfObject::DelayedLoadStreamImpl()
{
    // Default implementation throws, since delayed loading of
    // steams should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

// TODO: IsDirty in a container should be modified automatically by its children??? YES! And stop on first parent not dirty
bool PdfObject::IsDirty() const
{
    // If this is a object with
    // stream, the streams dirty
    // flag might be set.
    if (m_bDirty)
        return m_bDirty;

    switch (m_Variant.GetDataType())
    {
    // Arrays and Dictionaries
    // handle dirty status by themselfes
    case EPdfDataType::Array:
        return m_Variant.GetArray().IsDirty();
    case EPdfDataType::Dictionary:
        return m_Variant.GetDictionary().IsDirty();
    case EPdfDataType::Bool:
    case EPdfDataType::Number:
    case EPdfDataType::Real:
    case EPdfDataType::String:
    case EPdfDataType::Name:
    case EPdfDataType::RawData:
    case EPdfDataType::Reference:
    case EPdfDataType::Null:
    case EPdfDataType::Unknown:
    default:
        return m_bDirty;
    };
}

// FIXME: This is completely wrong and unsafe. PoDoFo does the same
void PdfObject::SetDirty(bool bDirty)
{
    m_bDirty = bDirty;

    if (!m_bDirty)
    {
        // Propogate new dirty state to subclasses
        switch (m_Variant.GetDataType())
        {
            // Arrays and Dictionaries
            // handle dirty status by themselfes
        case EPdfDataType::Array:
            m_Variant.GetArray().SetDirty();
            break;
        case EPdfDataType::Dictionary:
            m_Variant.GetDictionary().SetDirty();
            break;
        case EPdfDataType::Bool:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
        case EPdfDataType::String:
        case EPdfDataType::Name:
        case EPdfDataType::RawData:
        case EPdfDataType::Reference:
        case EPdfDataType::Null:
        case EPdfDataType::Unknown:
        default:
            break;
        };
    }
}

void PdfObject::SetImmutable(bool bImmutable)
{
    DelayedLoad();
    m_bImmutable = bImmutable;

    switch (m_Variant.GetDataType())
    {
    // Arrays and Dictionaries
    // handle dirty status by themselfes
    case EPdfDataType::Array:
        m_Variant.GetArray().SetImmutable(bImmutable);
        break;
    case EPdfDataType::Dictionary:
        m_Variant.GetDictionary().SetImmutable(bImmutable);
        break;

    case EPdfDataType::Bool:
    case EPdfDataType::Number:
    case EPdfDataType::Real:
    case EPdfDataType::String:
    case EPdfDataType::Name:
    case EPdfDataType::RawData:
    case EPdfDataType::Reference:
    case EPdfDataType::Null:
    case EPdfDataType::Unknown:
    default:
        // Do nothing
        break;
    }
}

PdfObject::operator const PdfVariant& () const
{
    DelayedLoad();
    return m_Variant;
}

const PdfVariant& PdfObject::GetVariant() const
{
    DelayedLoad();
    return m_Variant;
}

void PdfObject::Clear()
{
    DelayedLoad();
    m_Variant.Clear();
}

EPdfDataType PdfObject::GetDataType() const
{
    DelayedLoad();
    return m_Variant.GetDataType();
}

void PdfObject::ToString(std::string& rsData, EPdfWriteMode eWriteMode) const
{
    DelayedLoad();
    m_Variant.ToString(rsData, eWriteMode);
}

bool PdfObject::GetBool() const
{
    DelayedLoad();
    return m_Variant.GetBool();
}

bool PdfObject::TryGetBool(bool& value) const
{
    DelayedLoad();
    return m_Variant.TryGetBool(value);
}

int64_t PdfObject::GetNumberLenient() const
{
    DelayedLoad();
    return m_Variant.GetNumberLenient();
}

bool PdfObject::TryGetNumberLenient(int64_t& value) const
{
    DelayedLoad();
    return m_Variant.TryGetNumberLenient(value);
}

int64_t PdfObject::GetNumber() const
{
    DelayedLoad();
    return m_Variant.GetNumber();
}

bool PdfObject::TryGetNumber(int64_t& value) const
{
    DelayedLoad();
    return m_Variant.TryGetNumber(value);
}

double PdfObject::GetReal() const
{
    DelayedLoad();
    return m_Variant.GetReal();
}

bool PdfObject::TryGetReal(double& value) const
{
    DelayedLoad();
    return m_Variant.TryGetReal(value);
}

const PdfData& PdfObject::GetRawData() const
{
    DelayedLoad();
    return m_Variant.GetRawData();
}

PdfData& PdfObject::GetRawData()
{
    DelayedLoad();
    return m_Variant.GetRawData();
}

bool PdfObject::TryGetRawData(const PdfData*& data) const
{
    DelayedLoad();
    return m_Variant.TryGetRawData(data);
}

bool PdfObject::TryGetRawData(PdfData*& data)
{
    DelayedLoad();
    return m_Variant.TryGetRawData(data);
}

double PdfObject::GetRealStrict() const
{
    DelayedLoad();
    return m_Variant.GetRealStrict();
}

bool PdfObject::TryGetRealStrict(double& value) const
{
    DelayedLoad();
    return m_Variant.TryGetRealStrict(value);
}

const PdfString& PdfObject::GetString() const
{
    DelayedLoad();
    return m_Variant.GetString();
}

bool PdfObject::TryGetString(const PdfString*& str) const
{
    DelayedLoad();
    return m_Variant.TryGetString(str);
}

const PdfName& PdfObject::GetName() const
{
    DelayedLoad();
    return m_Variant.GetName();
}

bool PdfObject::TryGetName(const PdfName*& name) const
{
    DelayedLoad();
    return m_Variant.TryGetName(name);
}

const PdfArray& PdfObject::GetArray() const
{
    DelayedLoad();
    return m_Variant.GetArray();
}

PdfArray& PdfObject::GetArray()
{
    DelayedLoad();
    return m_Variant.GetArray();
}

bool PdfObject::TryGetArray(const PdfArray*& arr) const
{
    DelayedLoad();
    return m_Variant.TryGetArray(arr);
}

bool PdfObject::TryGetArray(PdfArray*& arr)
{
    DelayedLoad();
    return m_Variant.TryGetArray(arr);
}

const PdfDictionary& PdfObject::GetDictionary() const
{
    DelayedLoad();
    return m_Variant.GetDictionary();
}

PdfDictionary& PdfObject::GetDictionary()
{
    DelayedLoad();
    return m_Variant.GetDictionary();
}

bool PdfObject::TryGetDictionary(const PdfDictionary*& dict) const
{
    DelayedLoad();
    return m_Variant.TryGetDictionary(dict);
}

bool PdfObject::TryGetDictionary(PdfDictionary*& dict)
{
    DelayedLoad();
    return m_Variant.TryGetDictionary(dict);
}

PdfReference PdfObject::GetReference() const
{
    DelayedLoad();
    return m_Variant.GetReference();
}

bool PdfObject::TryGetReference(PdfReference& ref) const
{
    DelayedLoad();
    return m_Variant.TryGetReference(ref);
}

void PdfObject::SetBool(bool b)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetBool(b);
    SetDirty(true);
}

void PdfObject::SetNumber(int64_t l)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetNumber(l);
    SetDirty(true);
}

void PdfObject::SetReal(double d)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetReal(d);
    SetDirty(true);
}

void PdfObject::SetName(const PdfName& name)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetName(name);
    SetDirty(true);
}

void PdfObject::SetString(const PdfString& str)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetString(str);
    SetDirty(true);
}

void PdfObject::SetReference(const PdfReference& ref)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetReference(ref);
    SetDirty(true);
}

const char* PdfObject::GetDataTypeString() const
{
    DelayedLoad();
    return m_Variant.GetDataTypeString();
}

bool PdfObject::IsBool() const
{
    return GetDataType() == EPdfDataType::Bool;
}

bool PdfObject::IsNumber() const
{
    return GetDataType() == EPdfDataType::Number;
}

bool PdfObject::IsRealStrict() const
{
    return GetDataType() == EPdfDataType::Real;
}

bool PdfObject::IsNumberOrReal() const
{
    EPdfDataType eDataType = GetDataType();
    return eDataType == EPdfDataType::Number || eDataType == EPdfDataType::Real;
}

bool PdfObject::IsString() const
{
    return GetDataType() == EPdfDataType::String;
}

bool PdfObject::IsName() const
{
    return GetDataType() == EPdfDataType::Name;
}

bool PdfObject::IsArray() const
{
    return GetDataType() == EPdfDataType::Array;
}

bool PdfObject::IsDictionary() const
{
    return GetDataType() == EPdfDataType::Dictionary;
}

bool PdfObject::IsRawData() const
{
    return GetDataType() == EPdfDataType::RawData;
}

bool PdfObject::IsNull() const
{
    return GetDataType() == EPdfDataType::Null;
}

bool PdfObject::IsReference() const
{
    return GetDataType() == EPdfDataType::Reference;
}

void PdfObject::AssertMutable() const
{
    if (m_bImmutable)
        PODOFO_RAISE_ERROR(EPdfError::ChangeOnImmutable);
}

bool PdfObject::operator<(const PdfObject& rhs) const
{
    DelayedLoad();
    rhs.DelayedLoad();
    return m_reference < rhs.m_reference;
}

// REWRITE-ME: The equality operator is pure shit
// Check owner,reference and pdfvariant value. Easy
bool PdfObject::operator==(const PdfObject& rhs) const
{
    DelayedLoad();
    rhs.DelayedLoad();
    return m_reference == rhs.m_reference;
}

bool PdfObject::operator!=(const PdfObject& rhs) const
{
    DelayedLoad();
    rhs.DelayedLoad();
    return !(*this == rhs);
}
