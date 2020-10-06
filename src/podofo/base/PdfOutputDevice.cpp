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

#include "PdfOutputDevice.h"
#include "PdfRefCountedBuffer.h"
#include "PdfDefinesPrivate.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace PoDoFo;

PdfOutputDevice::PdfOutputDevice()
{
    this->Init();
}

PdfOutputDevice::PdfOutputDevice(const std::string_view& filename, bool bTruncate )
{
    this->Init();

    std::ios_base::openmode openmode = std::fstream::binary | std::ios_base::in | std::ios_base::out;
    if (bTruncate)
        openmode |= std::ios_base::trunc;

    auto pStream = new fstream(io::open_fstream(filename, openmode));
    if( pStream->fail() )
    {
        delete pStream;
        PODOFO_RAISE_ERROR_INFO( EPdfError::FileNotFound, (std::string)filename);
    }

    m_pStream = pStream;
    m_pReadStream = pStream;

    if( !bTruncate )
    {
        m_pStream->seekp( 0, std::ios_base::end );
        m_ulPosition = (size_t)m_pStream->tellp();
        m_ulLength = m_ulPosition;
    }
}

PdfOutputDevice::PdfOutputDevice( char* pBuffer, size_t lLen )
{
    this->Init();

    if( !pBuffer )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_lBufferLen = lLen;
    m_pBuffer    = pBuffer;
}

PdfOutputDevice::PdfOutputDevice( const std::ostream* pOutStream )
{
    this->Init();

    m_pStream = const_cast< std::ostream* >( pOutStream );
    m_pStreamOwned = false;
}

PdfOutputDevice::PdfOutputDevice( PdfRefCountedBuffer* pOutBuffer )
{
    this->Init();
    m_pRefCountedBuffer = pOutBuffer;
}

PdfOutputDevice::PdfOutputDevice( std::iostream* pStream )
{
    this->Init();

    m_pStream = pStream;
    m_pReadStream = pStream;
    m_pStreamOwned = false;
}

PdfOutputDevice::~PdfOutputDevice()
{
    if( m_pStreamOwned ) 
        delete m_pStream;
}

void PdfOutputDevice::Init()
{
    m_ulLength          = 0;
    m_pBuffer           = nullptr;
    m_pStream           = nullptr;
	m_pReadStream       = nullptr;
    m_pRefCountedBuffer = nullptr;
    m_lBufferLen        = 0;
    m_ulPosition        = 0;
    m_pStreamOwned      = true;
}

void PdfOutputDevice::Print( const char* pszFormat, ... )
{
    va_list args;
	va_start( args, pszFormat );
	Print(pszFormat, args);
	va_end( args );
}

void PdfOutputDevice::Print(const char* pszFormat, va_list args)
{
    int rc = compat::vsnprintf(nullptr, 0, pszFormat, args);
    if (rc < 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    PrintV(pszFormat, (size_t)rc, args);
}

void PdfOutputDevice::PrintV( const char* pszFormat, size_t lBytes, va_list args )
{
    if( !pszFormat )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if( m_pBuffer )
    {
        if( m_ulPosition + lBytes <= m_lBufferLen )
        {
            compat::vsnprintf( m_pBuffer + m_ulPosition, m_lBufferLen - m_ulPosition, pszFormat, args );
        }
        else
        {
            PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
        }
    }
    else if( m_pStream || m_pRefCountedBuffer )
    {
        m_printBuffer.resize( lBytes );
        compat::vsnprintf(m_printBuffer.data(), lBytes + 1, pszFormat, args );

        if( m_pStream ) 
        {
            m_pStream->write(m_printBuffer.data(), lBytes);
            if (m_pStream->fail())
                PODOFO_RAISE_ERROR(EPdfError::InvalidDeviceOperation);
        }
        else // if( m_pRefCountedBuffer ) 
        {
            if( m_ulPosition + lBytes > m_pRefCountedBuffer->GetSize())
                m_pRefCountedBuffer->Resize( m_ulPosition + lBytes );

            memcpy( m_pRefCountedBuffer->GetBuffer() + m_ulPosition, m_printBuffer.data(), lBytes );
        }
    }

    m_ulPosition += lBytes;
    if (m_ulPosition > m_ulLength)
        m_ulLength = m_ulPosition;
}

size_t PdfOutputDevice::Read( char* pBuffer, size_t lLen )
{
	size_t numRead = 0;
    if( m_pBuffer )
    {		
        if( m_ulPosition <= m_ulLength )
        {
			numRead = std::min(lLen, m_ulLength-m_ulPosition);
            memcpy( pBuffer, m_pBuffer + m_ulPosition, numRead);
        }
    }
    else if( m_pReadStream )
    {
		size_t iPos = (size_t)m_pReadStream->tellg();
		m_pReadStream->read( pBuffer, lLen );
		if(m_pReadStream->fail() && !m_pReadStream->eof())
			PODOFO_RAISE_ERROR( EPdfError::InvalidDeviceOperation );

		numRead = (size_t)m_pReadStream->tellg();
		numRead -= iPos;
    }
    else if( m_pRefCountedBuffer ) 
    {
        if( m_ulPosition <= m_ulLength )
		{
			numRead = std::min(lLen, m_ulLength-m_ulPosition);
            memcpy( pBuffer, m_pRefCountedBuffer->GetBuffer() + m_ulPosition, numRead );
		}
    }

    m_ulPosition += static_cast<size_t>(numRead);
	return numRead;
}

void PdfOutputDevice::Write( const char* pBuffer, size_t lLen )
{
    if( m_pBuffer )
    {
        if( m_ulPosition + lLen <= m_lBufferLen )
        {
            memcpy( m_pBuffer + m_ulPosition, pBuffer, lLen );
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::OutOfMemory, "Allocated buffer to small for PdfOutputDevice. Cannot write!"  );
        }
    }
    else if( m_pStream )
    {
        m_pStream->write( pBuffer, lLen );
    }
    else if( m_pRefCountedBuffer ) 
    {
        if( m_ulPosition + lLen > m_pRefCountedBuffer->GetSize() )
            m_pRefCountedBuffer->Resize( m_ulPosition + lLen );

        memcpy( m_pRefCountedBuffer->GetBuffer() + m_ulPosition, pBuffer, lLen );
    }

    m_ulPosition += static_cast<size_t>(lLen);
	if(m_ulPosition>m_ulLength) m_ulLength = m_ulPosition;
}

void PdfOutputDevice::Seek( size_t offset )
{
    if( m_pBuffer )
    {
        if( offset >= m_lBufferLen )
        {
            PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );
        }
    }
    else if( m_pStream )
    {
        m_pStream->seekp( offset, std::ios_base::beg );
    }
    else if( m_pRefCountedBuffer ) 
    {
        m_ulPosition = offset;
    }

    m_ulPosition = offset;
    // Seek should not change the length of the device
    // m_ulLength = offset;
}

void PdfOutputDevice::Flush()
{
    if( m_pStream )
        m_pStream->flush();
}
