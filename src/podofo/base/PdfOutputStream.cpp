/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"
#include "PdfRefCountedBuffer.h"

using namespace std;
using namespace PoDoFo;

void PdfOutputStream::Write(const string_view& view)
{
    Write(view.data(), view.length());
}

PdfOutputStream::~PdfOutputStream()
{
}

void PdfOutputStream::Write(const char* buffer, size_t len)
{
    if (len == 0)
        return;

    WriteImpl(buffer, len);
}

PdfMemoryOutputStream::PdfMemoryOutputStream(size_t initialSize)
    : m_Len(0), m_OwnBuffer(true)
{
    m_Size = initialSize;
    m_Buffer = static_cast<char*>(podofo_calloc(m_Size, sizeof(char)));

    if (m_Buffer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
}

PdfMemoryOutputStream::PdfMemoryOutputStream(char* buffer, size_t size)
    : m_Len(0), m_OwnBuffer(false)
{
    if (buffer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_Size = size;
    m_Buffer = buffer;
}

PdfMemoryOutputStream::~PdfMemoryOutputStream()
{
    if (m_OwnBuffer)
        podofo_free(m_Buffer);
}

void PdfMemoryOutputStream::WriteImpl(const char* data, size_t len)
{
    if (m_Len + len > m_Size)
    {
        if (m_OwnBuffer)
        {
            // a reallocation is required
            m_Size = std::max(m_Len + len, m_Size << 1);
            m_Buffer = static_cast<char*>(podofo_realloc(m_Buffer, m_Size));
            if (m_Buffer == nullptr)
                PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
        }
        else
        {
            PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
        }
    }

    memcpy(m_Buffer + m_Len, data, len);
    m_Len += len;
}

void PdfMemoryOutputStream::Close()
{
}

char* PdfMemoryOutputStream::TakeBuffer()
{
    char* buffer = m_Buffer;
    m_Buffer = nullptr;
    return buffer;
}

PdfDeviceOutputStream::PdfDeviceOutputStream(PdfOutputDevice& device)
    : m_Device(&device)
{
}

void PdfDeviceOutputStream::WriteImpl(const char* data, size_t len)
{
    m_Device->Write(data, len);
}

void PdfDeviceOutputStream::Close()
{
}

PdfBufferOutputStream::PdfBufferOutputStream(PdfRefCountedBuffer& buffer)
    : m_Buffer(&buffer), m_Length(buffer.GetSize())
{
}

void PdfBufferOutputStream::WriteImpl(const char* data, size_t len)
{
    if (m_Length + len >= m_Buffer->GetSize())
        m_Buffer->Resize(m_Length + len);

    memcpy(m_Buffer->GetBuffer() + m_Length, data, len);
    m_Length += len;
}

void PdfBufferOutputStream::Close()
{
}
