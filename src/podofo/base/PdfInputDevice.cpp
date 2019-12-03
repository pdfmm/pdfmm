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

#include "PdfInputDevice.h"

#include <cstdarg>
#include <fstream>
#include <sstream>
#include "PdfDefinesPrivate.h"

using namespace PoDoFo;

PdfInputDevice::PdfInputDevice()
{
    this->Init();
}

PdfInputDevice::PdfInputDevice( const char* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );

    try
    {
        m_pFile = fopen(pszFilename, "rb");
        if( !m_pFile)
            PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );

        m_StreamOwned = true;
    }
    catch(...)
    {
        // should probably check the exact error, but for now it's a good error
        PODOFO_RAISE_ERROR_INFO( ePdfError_FileNotFound, pszFilename );
    }
}

#ifdef _WIN32
PdfInputDevice::PdfInputDevice( const wchar_t* pszFilename )
{
    this->Init();

    if( !pszFilename ) 
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );

    try
    {
        // James McGill 16.02.2011 Fix wide character filename loading in windows
        m_pFile = _wfopen(pszFilename, L"rb");
        if( !m_pFile)
        {
            PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
            e.SetErrorInformation( pszFilename );
            throw e;
        }
        m_StreamOwned = true;
    }
    catch(...)
    {
        // should probably check the exact error, but for now it's a good error
        PdfError e( ePdfError_FileNotFound, __FILE__, __LINE__ );
        e.SetErrorInformation( pszFilename );
        throw e;
    }
}
#endif // _WIN32

PdfInputDevice::PdfInputDevice( const char* pBuffer, size_t lLen )
{
    this->Init();

    if( !pBuffer ) 
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );

    try
    {
        m_pStream = static_cast< std::istream* >( 
            new std::istringstream( std::string( pBuffer, lLen ), std::ios::binary ) );
        if( !m_pStream || !m_pStream->good() )
            PODOFO_RAISE_ERROR( ePdfError_FileNotFound );

        m_StreamOwned = true;
    }
    catch(...)
    {
        // should probably check the exact error, but for now it's a good error
        PODOFO_RAISE_ERROR( ePdfError_FileNotFound );
    }
    PdfLocaleImbue(*m_pStream);
}

PdfInputDevice::PdfInputDevice( const std::istream* pInStream )
{
    this->Init();

    m_pStream = const_cast< std::istream* >( pInStream );
    if( !m_pStream->good() )
        PODOFO_RAISE_ERROR( ePdfError_FileNotFound );

    PdfLocaleImbue(*m_pStream);
}

PdfInputDevice::~PdfInputDevice()
{
    this->Close();

    if ( m_StreamOwned ) 
    {
        if (m_pStream)
            delete m_pStream;
        
        if (m_pFile)
            fclose(m_pFile);
    }
}

void PdfInputDevice::Init()
{
    m_pStream = nullptr;
    m_pFile = 0;
    m_StreamOwned = false;
    m_bIsSeekable = true;
}

void PdfInputDevice::Close()
{
    // nothing to do here, but maybe necessary for inheriting classes
}

int PdfInputDevice::GetChar() const
{
	if (m_pStream)
    {
        int ch = m_pStream->get();
        if (m_pStream->fail() && !m_pStream->eof())
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to read the current character");

        return ch;
    }
    
	if (m_pFile)
    {
        int ch = fgetc(m_pFile);
        if (ferror(m_pFile))
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to read the current character");

        return ch;
    }
    
	return 0;
}

int PdfInputDevice::Look() const 
{
    if (m_pStream)
    {
        int ch = m_pStream->peek();
        if (m_pStream->fail() && !m_pStream->eof())
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to peek current character");

        return ch;
    }
    
    if (m_pFile)
    {
        ssize_t lOffset = ftello( m_pFile );
        if( lOffset == -1)
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Failed to read the current file position" );

        int ch = fgetc(m_pFile);
        if (ferror(m_pFile))
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to read the current character");

        if(fseeko( m_pFile, lOffset, SEEK_SET ) == -1)
            PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Failed to seek back to the previous position" );
        
        return ch;
    }

    return 0;
}

size_t PdfInputDevice::Tell() const
{
	if (m_pStream)
    {
        std::streamoff ret = m_pStream->tellg();
        if (m_pStream->fail())
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to get current position in the stream");

        return (size_t)ret;
    }
    
	if (m_pFile)
    {
        ssize_t lOffset = ftello(m_pFile);
        if (lOffset == -1)
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to read the current file position");

        return (size_t)lOffset;
    }
    
	return 0;
}

void PdfInputDevice::Seek( std::streamoff off, std::ios_base::seekdir dir )
{
    if (m_bIsSeekable)
    {
        if (m_pStream)
        {
            m_pStream->seekg( off, dir );
            if (m_pStream->fail())
                PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to seek to given position in the stream");
        }

        if (m_pFile)
        {
            int whence;

            if( dir == std::ios_base::beg )
                whence = SEEK_SET;
            else if( dir == std::ios_base::cur )
                whence = SEEK_CUR;
            else // if( dir == std::ios_base::end )
                whence = SEEK_END;
            
            if( fseeko( m_pFile, (ssize_t)off, whence ) == -1)
                PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Failed to seek to given position in the file" );
        }
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDeviceOperation, "Tried to seek an unseekable input device." );
    }
}


size_t PdfInputDevice::Read( char* pBuffer, size_t lLen )
{
	if (m_pStream)
    {
        m_pStream->read( pBuffer, lLen );
        if (m_pStream->fail() && !m_pStream->eof())
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to read the amount of bytes requested");

        return (size_t)m_pStream->gcount();
	}
	else 
	{
		size_t ret = fread(pBuffer, 1, (size_t)lLen, m_pFile);
        if (ferror(m_pFile))
            PODOFO_RAISE_ERROR_INFO(ePdfError_InvalidDeviceOperation, "Failed to read the amount of bytes requested");

        return ret;
	}
}
