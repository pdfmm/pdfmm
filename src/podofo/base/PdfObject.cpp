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

#include "PdfVecObjects.h"
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
    : PdfVariant( PdfDictionary() )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfReference & rRef, const char* pszType )
    : PdfVariant( PdfDictionary() ), m_reference( rRef )
{
    InitPdfObject();

    if( pszType )
        this->GetDictionary().AddKey( PdfName::KeyType, PdfName( pszType ) );
}

PdfObject::PdfObject( const PdfVariant & var )
    : PdfVariant( var )
{
    InitPdfObject();
}

PdfObject::PdfObject( bool b )
    : PdfVariant( b )
{
    InitPdfObject();
}

PdfObject::PdfObject( int64_t l )
    : PdfVariant( l )
{
    InitPdfObject();
}

PdfObject::PdfObject( double d )
    : PdfVariant( d )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfString & rsString )
    : PdfVariant( rsString )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfName & rName )
    : PdfVariant( rName )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfReference & rRef )
    : PdfVariant( rRef )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfArray & tList )
    : PdfVariant( tList )
{
    InitPdfObject();
}

PdfObject::PdfObject( const PdfDictionary & rDict )
    : PdfVariant( rDict )
{
    InitPdfObject();
}

// NOTE: Don't copy owner. Copied objects must be always detached.
// Ownership will be set automatically elsewhere. Also don't copy
// reference
PdfObject::PdfObject( const PdfObject & rhs ) 
    : PdfVariant( rhs )
{
    InitPdfObject();
    copyFrom(rhs);
}

void PdfObject::ForceCreateStream()
{
    DelayedLoadStream();
    forceCreateStream();
}

void PdfObject::SetOwner(PdfVecObjects &pVecObjects)
{
    if ( m_pOwner == &pVecObjects )
    {
        // The inner owner for variant data objects is guaranteed to be same
        return;
    }

    m_pOwner = &pVecObjects;
    SetVariantOwner();
}

void PdfObject::AfterDelayedLoad()
{
    SetVariantOwner();
}

void PdfObject::SetVariantOwner()
{
    auto eDataType = GetDataType_NoDL();
    switch ( eDataType )
    {
        case EPdfDataType::Dictionary:
            static_cast<PdfOwnedDataType &>( GetDictionary_NoDL() ).SetOwner( this );
            break;
        case EPdfDataType::Array:
            static_cast<PdfOwnedDataType &>( GetArray_NoDL() ).SetOwner( this );
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
    m_pStream = nullptr;
    m_pOwner = nullptr;
    m_DelayedLoadStreamDone = true;
    SetVariantOwner();
}

void PdfObject::WriteObject( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode,
                             PdfEncrypt* pEncrypt, const PdfName & keyStop ) const
{
    DelayedLoadStream();

    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( m_reference.IsIndirect() )
    {
        // CHECK-ME We want to make this in all the cases for PDF/A Compatibility
        //if( (eWriteMode & ePdfWriteMode_Clean) == ePdfWriteMode_Clean )
        {
            pDevice->Print( "%i %i obj\n", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        }
        //else
        //{
        //    pDevice->Print( "%i %i obj", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        //}
    }

    if( pEncrypt ) 
    {
        pEncrypt->SetCurrentReference( m_reference );
    }

    if( pEncrypt && m_pStream != nullptr )
    {
        // Set length if it is a key
        PdfFileStream* pFileStream = dynamic_cast<PdfFileStream*>(m_pStream.get());
        if( !pFileStream )
        {
            // PdfFileStream handles encryption internally
            pdf_long lLength = pEncrypt->CalculateStreamLength(m_pStream->GetLength());
            *(const_cast<PdfObject*>(this)->GetIndirectKey( PdfName::KeyLength )) = PdfObject(static_cast<int64_t>(lLength));
        }
    }

    this->Write( pDevice, eWriteMode, pEncrypt, keyStop );
    pDevice->Print( "\n" );

    if( m_pStream )
    {
        m_pStream->Write( pDevice, pEncrypt );
    }

    if( m_reference.IsIndirect() )
    {
        pDevice->Print( "endobj\n" );
    }
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
        PODOFO_RAISE_ERROR(ePdfError_NoObject);
    return obj;
}

pdf_long PdfObject::GetObjectLength( EPdfWriteMode eWriteMode )
{
    PdfOutputDevice device;

    this->WriteObject( &device, eWriteMode, NULL  );

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

    if (GetDataType() != EPdfDataType::Dictionary)
    {
        PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDataType, "Tried to get stream of non-dictionary object");
    }

    if (m_pOwner == nullptr)
        m_pStream.reset(new PdfMemStream(this));
    else
        m_pStream.reset(m_pOwner->CreateStream(this));
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

    /*
    if( m_pStream )
        m_pStream->FlateCompress();
    */
}

const PdfObject & PdfObject::operator=(const PdfObject & rhs)
{
    if (&rhs == this)
        return *this;

    PdfVariant::operator=(rhs);
    copyFrom(rhs);
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

pdf_long PdfObject::GetByteOffset( const char* pszKey, EPdfWriteMode eWriteMode )
{
    PdfOutputDevice device;

    if( !pszKey )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if( !this->GetDictionary().HasKey( pszKey ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidKey );
    }

    this->Write( &device, eWriteMode, NULL, pszKey );
    
    return device.GetLength();
}

void PdfObject::EnableDelayedLoadingStream()
{
    m_DelayedLoadStreamDone = false;
}

void PdfObject::DelayedLoadStreamImpl()
{
    // Default implementation throws, since delayed loading of
    // steams should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(ePdfError_InternalLogic);
}
