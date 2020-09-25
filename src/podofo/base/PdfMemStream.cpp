/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfMemStream.h"

#include "PdfArray.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"

#include <cstdlib>

using namespace std;
using namespace PoDoFo;

PdfMemStream::PdfMemStream( PdfObject* pParent )
    : PdfStream( pParent ), m_lLength( 0 )
{
}

void PdfMemStream::BeginAppendImpl( const TVecFilters & vecFilters )
{
    m_buffer  = PdfRefCountedBuffer();
	m_lLength = 0;

    if( vecFilters.size() )
    {
        m_pBufferStream = unique_ptr<PdfBufferOutputStream>(new PdfBufferOutputStream( &m_buffer ));
        m_pStream = PdfFilterFactory::CreateEncodeStream( vecFilters, *m_pBufferStream );
    }
    else 
        m_pStream = unique_ptr<PdfBufferOutputStream>(new PdfBufferOutputStream( &m_buffer ));

}

void PdfMemStream::AppendImpl( const char* pszString, size_t lLen )
{
    m_pStream->Write( pszString, lLen );
}

void PdfMemStream::EndAppendImpl()
{
    if( m_pStream ) 
    {
        m_pStream->Close();

        if( !m_pBufferStream )
        {
            PdfBufferOutputStream* pBufferOutputStream = dynamic_cast<PdfBufferOutputStream*>(m_pStream.get());
            if( pBufferOutputStream )
                m_lLength = pBufferOutputStream->GetLength();
        }

        m_pStream = nullptr;
    }

    if( m_pBufferStream ) 
    {
        m_pBufferStream->Close();
        m_lLength = m_pBufferStream->GetLength();
        m_pBufferStream = nullptr;
    }
}

void PdfMemStream::GetCopy( char** pBuffer, size_t* lLen ) const
{
    if( !pBuffer || !lLen )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    *pBuffer = static_cast<char*>(podofo_calloc( m_lLength, sizeof(char) ));
    *lLen = m_lLength;
    
    if( !*pBuffer )
    {
        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }
    
    memcpy( *pBuffer, m_buffer.GetBuffer(), m_lLength );
}


void PdfMemStream::GetCopy(PdfOutputStream * pStream) const
{
	if( !pStream)
	{
		PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
	}
	pStream->Write(m_buffer.GetBuffer(), m_lLength);
}

void PdfMemStream::FlateCompress()
{
    PdfObject*        pObj;
    PdfVariant        vFilter( PdfName("FlateDecode" ) );
    PdfVariant        vFilterList;
    PdfArray          tFilters;

    PdfArray::const_iterator tciFilters;
    
    if( !m_lLength )
        return; // EPdfError::ErrOk

    // TODO: Handle DecodeParms
    if( m_pParent->GetDictionary().HasKey( "Filter" ) )
    {
        pObj = m_pParent->GetIndirectKey( "Filter" );

        if( pObj->IsName() )
        {
            if( pObj->GetName() != "DCTDecode" && pObj->GetName() != "FlateDecode" )
            {
                tFilters.push_back( vFilter );
                tFilters.push_back( *pObj );
            }
        }
        else if( pObj->IsArray() )
        {
            tciFilters = pObj->GetArray().begin();

            while( tciFilters != pObj->GetArray().end() )
            {
                if( (*tciFilters).IsName() )
                {
                    // do not compress DCTDecoded are already FlateDecoded streams again
                    if( (*tciFilters).GetName() == "DCTDecode" || (*tciFilters).GetName() == "FlateDecode" )
                    {
                        return;
                    }
                }

                ++tciFilters;
            }

            tFilters.push_back( vFilter );

            tciFilters = pObj->GetArray().begin();

            while( tciFilters != pObj->GetArray().end() )
            {
                tFilters.push_back( (*tciFilters) );
                
                ++tciFilters;
            }
        }
        else
            return;

        vFilterList = PdfVariant( tFilters );
        m_pParent->GetDictionary().AddKey( "Filter", vFilterList );

        FlateCompressStreamData(); // throws an exception on error
    }
    else
    {
        m_pParent->GetDictionary().AddKey( "Filter", PdfName( "FlateDecode" ) );
        FlateCompressStreamData();
    }
}

void PdfMemStream::Uncompress()
{
    size_t lLen;
    char * pBuffer = NULL;
    
    TVecFilters  vecEmpty;

    if( m_pParent && m_pParent->IsDictionary() && m_pParent->GetDictionary().HasKey( "Filter" ) && m_lLength )
    {
        try {
            this->GetFilteredCopy( &pBuffer, &lLen );
        } catch( PdfError & e ) {
            if( pBuffer )
                podofo_free( pBuffer );

            throw e;
        }

        this->Set( pBuffer, lLen, vecEmpty );
        // free the memory allocated by GetFilteredCopy again.
        podofo_free( pBuffer );

        m_pParent->GetDictionary().RemoveKey( "Filter" ); 
        if( m_pParent->GetDictionary().HasKey( "DecodeParms" ) ) 
        {
            m_pParent->GetDictionary().RemoveKey( "DecodeParms" ); 
        }
    }
}

const PdfMemStream & PdfMemStream::operator=(const PdfStream & rhs)
{
    if (&rhs == this)
        return *this;

    CopyFrom(rhs);
    return (*this);
}

const PdfMemStream & PdfMemStream::operator=(const PdfMemStream & rhs)
{
    if (&rhs == this)
        return *this;

    CopyFrom(rhs);
    return (*this);
}

void PdfMemStream::FlateCompressStreamData()
{
    char * pBuffer;
    size_t lLen;

    if( !m_lLength )
        return;

    std::unique_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( EPdfFilter::FlateDecode );
    if( pFilter.get() )
    {
        pFilter->Encode( m_buffer.GetBuffer(), m_buffer.GetSize(), &pBuffer, &lLen );
        this->Set( pBuffer, lLen );
    }
    else
    {
        PODOFO_RAISE_ERROR( EPdfError::UnsupportedFilter );
    }
}

void PdfMemStream::CopyFrom(const PdfStream & rhs)
{
    const PdfMemStream* memstream = dynamic_cast<const PdfMemStream*>(&rhs);
    if (memstream == nullptr)
    {
        PdfStream::operator=(rhs);
        return;
    }

    copyFrom(*memstream);
}

void PdfMemStream::copyFrom(const PdfMemStream &rhs)
{
    m_buffer = rhs.m_buffer;
    m_lLength = rhs.GetLength();
}

void PdfMemStream::Write(PdfOutputDevice& pDevice, const PdfEncrypt* pEncrypt)
{
    pDevice.Print( "stream\n" );
    if( pEncrypt ) 
    {
        size_t lLen = this->GetLength();

        size_t nOutputLen = pEncrypt->CalculateStreamLength(lLen);

        char *pOutputBuffer = new char[nOutputLen];

        pEncrypt->Encrypt( reinterpret_cast<const unsigned char*>(this->Get()), lLen,
                          reinterpret_cast<unsigned char*>(pOutputBuffer), nOutputLen);
        pDevice.Write( pOutputBuffer, nOutputLen );

        delete[] pOutputBuffer;
    }
    else
    {
        pDevice.Write( this->Get(), this->GetLength() );
    }
    pDevice.Print( "\nendstream\n" );
}

const char* PdfMemStream::Get() const
{
    return m_buffer.GetBuffer();
}

const char* PdfMemStream::GetInternalBuffer() const
{
    return m_buffer.GetBuffer();
}

size_t PdfMemStream::GetInternalBufferSize() const
{
    return m_lLength;
}

size_t PdfMemStream::GetLength() const
{
    return m_lLength;
}
