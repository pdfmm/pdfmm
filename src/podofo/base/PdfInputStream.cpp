/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfInputStream.h"

#include "PdfInputDevice.h"

using namespace std;
using namespace PoDoFo;

PdfInputStream::PdfInputStream() :
    m_eof(false)
{
}

size_t PdfInputStream::Read(char* buffer, size_t len, bool& eof)
{
    if (m_eof)
    {
        eof = true;
        return 0;
    }

    if (len == 0)
        return 0;

    if (buffer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    size_t ret = ReadImpl(buffer, len, m_eof);
    eof = m_eof;
    return ret;
}

PdfFileInputStream::PdfFileInputStream(const string_view& filename)
    : m_stream(io::open_ifstream(filename, ios_base::in | ios_base::binary))
{
    if (m_stream.fail())
        PODOFO_RAISE_ERROR_INFO(EPdfError::FileNotFound, filename.data());
}

PdfFileInputStream::~PdfFileInputStream() { }

size_t PdfFileInputStream::ReadImpl(char* pBuffer, size_t lLen, bool& eof)
{
    size_t ret = io::Read(m_stream, pBuffer, lLen);
    eof = m_stream.eof();
    return ret;
}

PdfMemoryInputStream::PdfMemoryInputStream(const char* pBuffer, size_t lBufferLen)
    : m_pBuffer(pBuffer), m_lBufferLen(lBufferLen)
{
}

PdfMemoryInputStream::~PdfMemoryInputStream()
{
}

size_t PdfMemoryInputStream::ReadImpl(char* pBuffer, size_t lLen, bool& eof)
{
    lLen = std::min(m_lBufferLen, lLen);
    memcpy(pBuffer, m_pBuffer, lLen);
    m_lBufferLen -= lLen;
    m_pBuffer += lLen;
    eof = m_lBufferLen == 0;
    return lLen;
}

PdfDeviceInputStream::PdfDeviceInputStream(PdfInputDevice* pDevice)
    : m_pDevice(pDevice)
{
}

size_t PdfDeviceInputStream::ReadImpl(char* pBuffer, size_t lLen, bool& eof)
{
    size_t ret = m_pDevice->Read(pBuffer, lLen);
    eof = m_pDevice->Eof();
    return ret;

}
