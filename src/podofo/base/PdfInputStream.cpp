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

#include "PdfInputStream.h"

#include "PdfInputDevice.h"
#include "PdfDefinesPrivate.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

using namespace std;
using namespace PoDoFo;

size_t PdfInputStream::Read(char* buffer, size_t len)
{
    if (len == 0)
        return 0;

    if (buffer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    return ReadImpl(buffer, len);
}

PdfFileInputStream::PdfFileInputStream(const string_view& filename)
    : m_stream(io::open_ifstream(filename, ios_base::in | ios_base::binary))
{
    if(m_stream.fail())
        PODOFO_RAISE_ERROR_INFO( EPdfError::FileNotFound, filename.data());
}

PdfFileInputStream::~PdfFileInputStream() { }

size_t PdfFileInputStream::ReadImpl( char* pBuffer, size_t lLen)
{
    return io::Read(m_stream, pBuffer, lLen);
}

PdfMemoryInputStream::PdfMemoryInputStream( const char* pBuffer, size_t lBufferLen )
    : m_pBuffer( pBuffer ), m_pCur( pBuffer ), m_lBufferLen( lBufferLen )
{
}

PdfMemoryInputStream::~PdfMemoryInputStream()
{
}

size_t PdfMemoryInputStream::ReadImpl( char* pBuffer, size_t lLen)
{
    size_t lRead = m_pCur - m_pBuffer;

    // return zero if EOF is reached
    if( lRead == m_lBufferLen ) 
        return 0;

    lLen = ( lRead + lLen <= m_lBufferLen ? lLen : m_lBufferLen - lRead );
    memcpy( pBuffer, m_pCur, lLen );
    m_pCur += lLen;
    
    return lLen;
}

PdfDeviceInputStream::PdfDeviceInputStream( PdfInputDevice* pDevice )
    : m_pDevice( pDevice )
{
}

size_t PdfDeviceInputStream::ReadImpl( char* pBuffer, size_t lLen)
{
    return (size_t)m_pDevice->Read( pBuffer, lLen );
}
