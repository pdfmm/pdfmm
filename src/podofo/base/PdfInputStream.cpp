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

size_t PdfFileInputStream::ReadImpl(char* buffer, size_t len, bool& eof)
{
    size_t ret = io::Read(m_stream, buffer, len);
    eof = m_stream.eof();
    return ret;
}

PdfMemoryInputStream::PdfMemoryInputStream(const char* buffer, size_t lBufferLen)
    : m_Buffer(buffer), m_BufferLen(lBufferLen)
{
}

PdfMemoryInputStream::~PdfMemoryInputStream()
{
}

size_t PdfMemoryInputStream::ReadImpl(char* buffer, size_t len, bool& eof)
{
    len = std::min(m_BufferLen, len);
    memcpy(buffer, m_Buffer, len);
    m_BufferLen -= len;
    m_Buffer += len;
    eof = m_BufferLen == 0;
    return len;
}

PdfDeviceInputStream::PdfDeviceInputStream(PdfInputDevice* device)
    : m_Device(device)
{
}

size_t PdfDeviceInputStream::ReadImpl(char* buffer, size_t len, bool& eof)
{
    size_t ret = m_Device->Read(buffer, len);
    eof = m_Device->Eof();
    return ret;
}
