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

#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"
#include "PdfRefCountedBuffer.h"
#include "PdfDefinesPrivate.h"

#include <stdlib.h>
#include <string.h>

using namespace std;
using namespace PoDoFo;

void PdfOutputStream::Write(const std::string_view& view)
{
    Write(view.data(), view.length());
}

void PdfOutputStream::Write(const char* buffer, size_t len)
{
    if (len == 0)
        return;

    WriteImpl(buffer, len);
}

PdfMemoryOutputStream::PdfMemoryOutputStream(size_t lInitial )
    : m_lLen( 0 ), m_bOwnBuffer( true )
{
    m_lSize   = lInitial;
    m_pBuffer = static_cast<char*>(podofo_calloc( m_lSize, sizeof(char) ));
    
    if( !m_pBuffer ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }
}

PdfMemoryOutputStream::PdfMemoryOutputStream( char* pBuffer, size_t lLen )
    : m_lLen( 0 ), m_bOwnBuffer( false )
{
    if( !pBuffer )
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    m_lSize   = lLen;
    m_pBuffer = pBuffer;
}

PdfMemoryOutputStream::~PdfMemoryOutputStream()
{
    if( m_bOwnBuffer )
        podofo_free( m_pBuffer );
}

void PdfMemoryOutputStream::WriteImpl(const char* data, size_t len)
{
    if( m_lLen + len > m_lSize )
    {
        if( m_bOwnBuffer )
        {
            // a reallocation is required
            m_lSize = std::max(m_lLen + len, m_lSize << 1);
            m_pBuffer = static_cast<char*>(podofo_realloc( m_pBuffer, m_lSize ));
            if( !m_pBuffer ) 
                PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
        }
        else
        {
            PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
        }
    }

    memcpy( m_pBuffer + m_lLen, data, len);
    m_lLen += len;
}

void PdfMemoryOutputStream::Close()
{
}

char* PdfMemoryOutputStream::TakeBuffer()
{
    char* pBuffer = m_pBuffer;
    m_pBuffer = nullptr;
    return pBuffer;
}

PdfDeviceOutputStream::PdfDeviceOutputStream( PdfOutputDevice* pDevice )
    : m_pDevice( pDevice )
{
}

void PdfDeviceOutputStream::WriteImpl(const char* data, size_t len)
{
    m_pDevice->Write(data, len);
}

void PdfDeviceOutputStream::Close()
{
}

PdfBufferOutputStream::PdfBufferOutputStream(PdfRefCountedBuffer* pBuffer)
    : m_pBuffer(pBuffer), m_lLength(pBuffer->GetSize())
{
}

void PdfBufferOutputStream::WriteImpl(const char* data, size_t len)
{
    if( m_lLength + len >= m_pBuffer->GetSize())
        m_pBuffer->Resize( m_lLength + len);

    memcpy( m_pBuffer->GetBuffer() + m_lLength, data, len);
    m_lLength += len;
}

void PdfBufferOutputStream::Close()
{
}
