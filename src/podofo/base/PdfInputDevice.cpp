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

using namespace std;
using namespace PoDoFo;

PdfInputDevice::PdfInputDevice(bool isSeeakable) :
    m_pStream(nullptr),
    m_StreamOwned(false),
    m_bIsSeekable(isSeeakable)
{
}

PdfInputDevice::PdfInputDevice(const string_view& filename)
    : PdfInputDevice(true)
{
    if (filename.length() == 0)
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    m_pStream = new ifstream(io::open_ifstream(filename, ios_base::in | ios_base::binary));
    if (m_pStream->fail())
        PODOFO_RAISE_ERROR_INFO(EPdfError::FileNotFound, filename.data());

    m_StreamOwned = true;
}

PdfInputDevice::PdfInputDevice( const char* pBuffer, size_t lLen )
    : PdfInputDevice(true)
{
    if( !pBuffer ) 
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    try
    {
        m_pStream = new std::istringstream( std::string( pBuffer, lLen ), std::ios::binary );
        m_StreamOwned = true;
    }
    catch(...)
    {
        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }
}

PdfInputDevice::PdfInputDevice( const std::istream* pInStream )
    : PdfInputDevice(true)
{
    m_pStream = const_cast< std::istream* >( pInStream );
    if( !m_pStream->good() )
        PODOFO_RAISE_ERROR( EPdfError::FileNotFound );
}

PdfInputDevice::~PdfInputDevice()
{
    this->Close();
    if (m_StreamOwned)
        delete m_pStream;
}

void PdfInputDevice::Close()
{
    // nothing to do here, but maybe necessary for inheriting classes
}

int PdfInputDevice::GetChar() const
{
    int ch;
    if (TryGetChar(ch))
        return ch;
    else
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to read the current character");
}

bool PdfInputDevice::TryGetChar(int &ch) const
{
    if (m_pStream->eof())
    {
        ch = -1;
        return false;
    }

    ch = m_pStream->get();
    if (m_pStream->fail())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to read the current character");

    return true;
}

int PdfInputDevice::Look() const 
{
    // NOTE: We don't want a peek() call to set failbit
    if (m_pStream->eof())
        return -1;

    int ch = m_pStream->peek();
    if (m_pStream->fail())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to peek current character");

    return ch;
}

size_t PdfInputDevice::Tell() const
{
    streamoff ret;
    if (m_pStream->eof())
    {
        // tellg() will set failbit when called on a stream that
        // is eof. Reset eofbit and restore it after calling tellg()
        // https://stackoverflow.com/q/18490576/213871
        PODOFO_ASSERT(!m_pStream->fail());
        m_pStream->clear();
        ret = m_pStream->tellg();
        m_pStream->clear(ios_base::eofbit);
    }
    else
    {
        ret = m_pStream->tellg();
    }
    if (m_pStream->fail())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to get current position in the stream");
    
    return (size_t)ret;
}

void PdfInputDevice::Seek( std::streamoff off, std::ios_base::seekdir dir )
{
    if (!m_bIsSeekable)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Tried to seek an unseekable input device.");

    m_pStream->seekg( off, dir );
    if (m_pStream->fail())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to seek to given position in the stream");
}

size_t PdfInputDevice::Read(char* pBuffer, size_t lLen)
{
    return io::Read(*m_pStream, pBuffer, lLen);
}

bool PdfInputDevice::Eof() const
{
    return m_pStream->eof();
}
