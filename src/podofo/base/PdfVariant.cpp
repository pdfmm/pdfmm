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

#include "PdfVariant.h"

#include "PdfArray.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"
#include "PdfDefinesPrivate.h"

#include <sstream>

#include <string.h>

using namespace PoDoFo;
using namespace std;

PdfVariant PdfVariant::NullValue;

void PdfVariant::Init()
{
    memset(&m_Data, 0, sizeof(UVariant));
    m_bDelayedLoadDone = true;
    m_bDirty = false;
    m_eDataType = EPdfDataType::Unknown;
    m_bImmutable = false;
}

PdfVariant::PdfVariant()
{
    Init();
    m_eDataType = EPdfDataType::Null;
}

PdfVariant::PdfVariant( bool b )
{
    Init();
    m_eDataType       = EPdfDataType::Bool;
    m_Data.bBoolValue = b;
}

PdfVariant::PdfVariant( int64_t l )
{
    Init();
    m_eDataType       = EPdfDataType::Number;
    m_Data.nNumber    = l;
}

PdfVariant::PdfVariant( double d )
{
    Init();
    m_eDataType       = EPdfDataType::Real;
    m_Data.dNumber    = d;    
}

PdfVariant::PdfVariant( const PdfString & rsString )
{
    Init();
    m_eDataType = EPdfDataType::String;
    m_Data.pData = new PdfString( rsString );
}

PdfVariant::PdfVariant( const PdfName & rName )
{
    Init();
    m_eDataType  = EPdfDataType::Name;
    m_Data.pData = new PdfName( rName );
}

PdfVariant::PdfVariant( const PdfReference & rRef )
{
    Init();
    m_eDataType  = EPdfDataType::Reference;
    m_Data.pData = new PdfReference( rRef );
}

PdfVariant::PdfVariant( const PdfArray & rArray )
{
    Init();
    m_eDataType  = EPdfDataType::Array;
    m_Data.pData = new PdfArray( rArray );
}

PdfVariant::PdfVariant( const PdfDictionary & rObj )
{
    Init();
    m_eDataType  = EPdfDataType::Dictionary;
    m_Data.pData = new PdfDictionary( rObj );
}

PdfVariant::PdfVariant( const PdfData & rData )
{
    Init();
    m_eDataType  = EPdfDataType::RawData;
    m_Data.pData = new PdfData( rData );
}

PdfVariant::PdfVariant( const PdfVariant & rhs )
{
    Init();
    this->operator=(rhs);

    SetDirty( false );
}

PdfVariant::~PdfVariant()
{
    Clear();
}

void PdfVariant::Clear()
{
    switch( m_eDataType ) 
    {
        case EPdfDataType::Array:
        case EPdfDataType::Reference:
        case EPdfDataType::Dictionary:
        case EPdfDataType::Name:
        case EPdfDataType::String:
        case EPdfDataType::RawData:
        {
            if( m_Data.pData )
                delete m_Data.pData;
            break;
        }
            
        case EPdfDataType::Bool:
        case EPdfDataType::Null:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
        case EPdfDataType::Unknown:
        default:
            break;
            
    }

    Init();
}

void PdfVariant::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt ) const
{
    this->Write( pDevice, eWriteMode, pEncrypt, PdfName::KeyNull );
}

void PdfVariant::Write( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    DelayedLoad(); 

    switch( m_eDataType ) 
    {
        case EPdfDataType::Bool:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
            {
                pDevice->Write( " ", 1 ); // Write space before true or false
            }

            if( m_Data.bBoolValue )
                pDevice->Write( "true", 4 );
            else
                pDevice->Write( "false", 5 );
            break;
        }
        case EPdfDataType::Number:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
            {
                pDevice->Write( " ", 1 ); // Write space before numbers
            }

            pDevice->Print( "%" PDF_FORMAT_INT64, m_Data.nNumber );
            break;
        }
        case EPdfDataType::Real:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
            {
                pDevice->Write( " ", 1 ); // Write space before numbers
            }

            // Use ostringstream, so that locale does not matter
            // NOTE: Don't use printf() formatting! It may write the number
            // way that is incompatible in PDF
            std::ostringstream oss;
            PdfLocaleImbue(oss);
            oss << std::fixed << m_Data.dNumber;
            std::string copy = oss.str();
            size_t len = copy.size();

            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact && 
                copy.find('.') != string::npos )
            {
                const char *str = copy.c_str();
                while( str[len - 1] == '0' )
                    --len;
                if( str[len - 1] == '.' )
                    --len;
                if( len == 0 )
                {
                    pDevice->Write( "0", 1 );
                    break;
                }
            }

            pDevice->Write( copy.c_str(), len );
            break;
        }
        case EPdfDataType::Reference:
            m_Data.pData->Write(pDevice, eWriteMode, pEncrypt);
            break;
        case EPdfDataType::String:
        case EPdfDataType::Name:
        case EPdfDataType::Array:
        case EPdfDataType::RawData:
            checkHandle();
            m_Data.pData->Write( pDevice, eWriteMode, pEncrypt );
            break;
        case EPdfDataType::Dictionary:
            checkHandle();
            static_cast<PdfDictionary*>(m_Data.pData)->Write( pDevice, eWriteMode, pEncrypt, keyStop );
            break;
        case EPdfDataType::Null:
        {
            if( (eWriteMode & EPdfWriteMode::Compact) == EPdfWriteMode::Compact ) 
            {
                pDevice->Write( " ", 1 ); // Write space before null
            }

            pDevice->Print( "null" );
            break;
        }
        case EPdfDataType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
            break;
        }
    };
}

void PdfVariant::DelayedLoad() const
{
    if (m_bDelayedLoadDone)
        return;

    const_cast<PdfVariant*>(this)->DelayedLoadImpl();
    m_bDelayedLoadDone = true;
    const_cast<PdfVariant*>(this)->AfterDelayedLoad();
}

void PdfVariant::DelayedLoadImpl()
{
    // Default implementation of virtual void DelayedLoadImpl() throws, since delayed
    // loading should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

void PdfVariant::ToString( std::string & rsData, EPdfWriteMode eWriteMode ) const
{
    ostringstream   out;
    // We don't need to this stream with the safe PDF locale because
    // PdfOutputDevice will do so for us.
    PdfOutputDevice device( &out );

    this->Write( &device, eWriteMode, NULL );
    
    rsData = out.str();
}

const PdfVariant & PdfVariant::operator=( const PdfVariant & rhs )
{
    Clear();

    rhs.DelayedLoad();

    m_eDataType = rhs.m_eDataType;
    
    switch( m_eDataType ) 
    {
        case EPdfDataType::Array:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfArray(*static_cast<PdfArray*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Reference:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfReference(*static_cast<PdfReference*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Dictionary:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfDictionary(*static_cast<PdfDictionary*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Name:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfName(*static_cast<PdfName*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::String:
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfString(*static_cast<PdfString*>(rhs.m_Data.pData));
            break;
        }
            
        case EPdfDataType::RawData: 
        {
            if( rhs.m_Data.pData ) 
                m_Data.pData = new PdfData(*static_cast<PdfData*>(rhs.m_Data.pData));
            break;
        }
        case EPdfDataType::Bool:
        case EPdfDataType::Null:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
            m_Data = rhs.m_Data;
            break;
            
        case EPdfDataType::Unknown:
        default:
            break;
    };

    SetDirty( true ); 
    return *this;
}

const char * PdfVariant::GetDataTypeString() const
{
    switch(GetDataType())
    {
        case EPdfDataType::Bool: return "Bool";
        case EPdfDataType::Number: return "Number";
        case EPdfDataType::Real: return "Real";
        case EPdfDataType::String: return "String";
        case EPdfDataType::Name: return "Name";
        case EPdfDataType::Array: return "Array";
        case EPdfDataType::Dictionary: return "Dictionary";
        case EPdfDataType::Null: return "Null";
        case EPdfDataType::Reference: return "Reference";
        case EPdfDataType::RawData: return "RawData";
        case EPdfDataType::Unknown: return "Unknown";
    }
    return "INVALID_TYPE_ENUM";
}

// REWRITE-ME: The equality operator is pure shit
// test on type first then get.
// Unkwnown -> return false always
// Rawdata: easy check on bytes

//
// This is rather slow:
//    - We set up to catch an exception
//    - We throw & catch an exception whenever there's a type mismatch
//
bool PdfVariant::operator==( const PdfVariant & rhs ) const
{
    DelayedLoad();
    try {
        switch (m_eDataType) {
            case EPdfDataType::Bool: return GetBool() == rhs.GetBool();
            case EPdfDataType::Number: return GetNumber() == rhs.GetNumber();
            case EPdfDataType::Real: return GetRealStrict() == rhs.GetRealStrict();
            case EPdfDataType::String: return GetString() == rhs.GetString();
            case EPdfDataType::Name: return GetName() == rhs.GetName();
            case EPdfDataType::Array: return GetArray() == rhs.GetArray();
            case EPdfDataType::Dictionary: return GetDictionary() == rhs.GetDictionary();
            case EPdfDataType::Null: return rhs.IsNull();
            case EPdfDataType::Reference: return GetReference() == rhs.GetReference();
            case EPdfDataType::RawData: /* fall through to end of func */ break;
            case EPdfDataType::Unknown: /* fall through to end of func */ break;
        }
    }
    catch ( PdfError& e )
    {
        if (e.GetError() == EPdfError::InvalidDataType)
            return false;
        else
            throw e;
    }
    PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Tried to compare unknown/raw variant" );
}


bool PdfVariant::operator!=(const PdfVariant& rhs) const
{
    return !(*this == rhs);
}

EPdfDataType PdfVariant::GetDataType() const
{
    DelayedLoad();
    return m_eDataType;
}

bool PdfVariant::GetBool() const
{
    bool ret;
    if (!TryGetBool(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetBool(bool& value) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Bool)
    {
        value = false;
        return false;
    }

    value = m_Data.bBoolValue;
    return true;
}

int64_t PdfVariant::GetNumberLenient() const
{
    int64_t ret;
    if (!TryGetNumberLenient(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetNumberLenient(int64_t& value) const
{
    DelayedLoad();
    if (!(m_eDataType == EPdfDataType::Number
        || m_eDataType == EPdfDataType::Real))
    {
        value = 0;
        return false;
    }

    if (m_eDataType == EPdfDataType::Real)
        value = static_cast<int64_t>(std::round(m_Data.dNumber));
    else
        value = m_Data.nNumber;

    return true;
}

int64_t PdfVariant::GetNumber() const
{
    int64_t ret;
    if (!TryGetNumber(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return m_Data.nNumber;
}

bool PdfVariant::TryGetNumber(int64_t& value) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Number)
    {
        value = 0;
        return false;
    }

    value = m_Data.nNumber;
    return true;
}

double PdfVariant::GetReal() const
{
    double ret;
    if (!TryGetReal(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReal(double& value) const
{
    DelayedLoad();
    if (!(m_eDataType == EPdfDataType::Real
        || m_eDataType == EPdfDataType::Number))
    {
        value = 0;
        return false;
    }

    if (m_eDataType == EPdfDataType::Number)
        value = static_cast<double>(m_Data.nNumber);
    else
        value = m_Data.dNumber;

    return true;
}

double PdfVariant::GetRealStrict() const
{
    double ret;
    if (!TryGetRealStrict(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return m_Data.dNumber;
}

bool PdfVariant::TryGetRealStrict(double& value) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Real)
    {
        value = 0;
        return false;
    }

    value = m_Data.dNumber;
    return true;
}

const PdfString & PdfVariant::GetString() const
{
    const PdfString* ret;
    if (!TryGetString(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetString(const PdfString*& str) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::String)
    {
        str = nullptr;
        return false;
    }

    str = (PdfString*)m_Data.pData;
    return true;
}

const PdfName & PdfVariant::GetName() const
{
    const PdfName* ret;
    if (!TryGetName(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetName(const PdfName*& name) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Name)
    {
        name = nullptr;
        return false;
    }

    name = (PdfName*)m_Data.pData;
    return true;
}

PdfReference PdfVariant::GetReference() const
{
    PdfReference ret;
    if (!TryGetReference(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReference(PdfReference& ref) const
{
    DelayedLoad();

    if (m_eDataType != EPdfDataType::Reference)
    {
        ref = PdfReference();
        return false;
    }

    ref = *(PdfReference*)m_Data.pData;
    return true;
}

const PdfData& PdfVariant::GetRawData() const
{
    const PdfData* ret;
    if (!TryGetRawData(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetRawData(const PdfData*& data) const
{
    DelayedLoad();

    if (m_eDataType != EPdfDataType::RawData)
    {
        data = nullptr;
        return false;
    }

    data = (PdfData*)m_Data.pData;
    return true;
}

const PdfArray & PdfVariant::GetArray() const
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfArray & PdfVariant::GetArray()
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetArray(const PdfArray*& arr) const
{
    return tryGetArray(const_cast<PdfArray*&>(arr));
}

bool PdfVariant::TryGetArray(PdfArray*& arr)
{
    return tryGetArray(arr);
}

const PdfDictionary & PdfVariant::GetDictionary() const
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfDictionary & PdfVariant::GetDictionary()
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetDictionary(const PdfDictionary*& dict) const
{
    return tryGetDictionary(const_cast<PdfDictionary*&>(dict));
}

bool PdfVariant::TryGetDictionary(PdfDictionary*& dict)
{
    return tryGetDictionary(dict);
}

bool PdfVariant::tryGetDictionary(PdfDictionary*& dict) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Dictionary)
    {
        dict = nullptr;
        return false;
    }

    dict = (PdfDictionary*)m_Data.pData;
    return true;
}

bool PdfVariant::tryGetArray(PdfArray*& arr) const
{
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Array)
    {
        arr = nullptr;
        return false;
    }

    arr = (PdfArray*)m_Data.pData;
    return true;
}

PdfArray& PdfVariant::GetArrayInternal()
{
    return *(PdfArray*)m_Data.pData;
}

PdfDictionary & PdfVariant::GetDictionaryInternal()
{
    return *(PdfDictionary*)m_Data.pData;
}

void PdfVariant::SetBool(bool b)
{
    AssertMutable();
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Bool)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_Data.bBoolValue = b;
    SetDirty(true);
}

void PdfVariant::SetNumber(int64_t l)
{
    AssertMutable();
    DelayedLoad();
    if (!(m_eDataType == EPdfDataType::Number
        || m_eDataType == EPdfDataType::Real))
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if (m_eDataType == EPdfDataType::Real)
        m_Data.dNumber = static_cast<double>(l);
    else
        m_Data.nNumber = l;
    SetDirty(true);
}

void PdfVariant::SetReal(double d)
{
    AssertMutable();
    DelayedLoad();
    if (!(m_eDataType == EPdfDataType::Real
        || m_eDataType == EPdfDataType::Number))
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if (m_eDataType == EPdfDataType::Number)
        m_Data.nNumber = static_cast<int64_t>(std::round(d));
    else
        m_Data.dNumber = d;

    SetDirty(true);
}

void PdfVariant::SetName(const PdfName &name)
{
    AssertMutable();
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Name)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfName*)m_Data.pData) = name;
    SetDirty(true);
}

void PdfVariant::SetString(const PdfString &str)
{
    AssertMutable();
    DelayedLoad();
    if (m_eDataType != EPdfDataType::String)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfString*)m_Data.pData) = str;
    SetDirty(true);
}

void PdfVariant::SetReference(const PdfReference &ref)
{
    AssertMutable();
    DelayedLoad();
    if (m_eDataType != EPdfDataType::Reference)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfReference*)m_Data.pData) = ref;
    SetDirty(true);
}

// TODO: IsDirty in a container should be modified automatically by its children??? YES! And stop on first parent not dirty
bool PdfVariant::IsDirty() const
{
    // If this is a object with
    // stream, the streams dirty
    // flag might be set.
    if (m_bDirty)
        return m_bDirty;

    switch (m_eDataType)
    {
        case EPdfDataType::Array:
        case EPdfDataType::Dictionary:
            // Arrays and Dictionaries
            // handle dirty status by themselfes
            return static_cast<PdfContainerDataType &>(*m_Data.pData).IsDirty();

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

void PdfVariant::SetDirty(bool bDirty)
{
    m_bDirty = bDirty;

    if (!m_bDirty)
    {
        // Propogate new dirty state to subclasses
        switch (m_eDataType)
        {
            case EPdfDataType::Array:
            case EPdfDataType::Dictionary:
                // Arrays and Dictionaries
                // handle dirty status by themselfes
                static_cast<PdfContainerDataType&>(*m_Data.pData).SetDirty();
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

void PdfVariant::SetImmutable(bool bImmutable)
{
    m_bImmutable = bImmutable;

    switch (m_eDataType)
    {
        case EPdfDataType::Array:
        case EPdfDataType::Dictionary:
            // Arrays and Dictionaries
            // handle dirty status by themselfes
            static_cast<PdfContainerDataType&>(*m_Data.pData).SetImmutable(bImmutable);
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

void PdfVariant::checkHandle() const
{
    if (m_Data.pData == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
}

void PdfVariant::AssertMutable() const
{
    if (m_bImmutable)
        PODOFO_RAISE_ERROR(EPdfError::ChangeOnImmutable);
}

void PdfVariant::AfterDelayedLoad()
{
    // Do nothing
}

bool PdfVariant::IsBool() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Bool;
}

bool PdfVariant::IsNumber() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Number;
}

bool PdfVariant::IsRealStrict() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Real;
}

bool PdfVariant::IsNumberOrReal() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Number || m_eDataType == EPdfDataType::Real;
}

bool PdfVariant::IsString() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::String;
}

bool PdfVariant::IsName() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Name;
}

bool PdfVariant::IsArray() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Array;
}

bool PdfVariant::IsDictionary() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Dictionary;
}

bool PdfVariant::IsRawData() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::RawData;
}

bool PdfVariant::IsNull() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Null;
}

bool PdfVariant::IsReference() const
{
    DelayedLoad();
    return m_eDataType == EPdfDataType::Reference;
}
