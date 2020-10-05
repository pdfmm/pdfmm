/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

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
		PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

	pStream->Write(m_buffer.GetBuffer(), m_lLength);
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
