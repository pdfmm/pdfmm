/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

void PdfOutputStream::Write(const string_view& view)
{
    Write(view.data(), view.length());
}

PdfOutputStream::~PdfOutputStream() { }

void PdfOutputStream::Write(const char* buffer, size_t len)
{
    if (len == 0)
        return;

    WriteImpl(buffer, len);
}

PdfMemoryOutputStream::PdfMemoryOutputStream(size_t initialSize)
    : m_Length(0), m_OwnBuffer(true)
{
    m_Capacity = initialSize;
    m_Buffer = new char[m_Capacity];
}

PdfMemoryOutputStream::PdfMemoryOutputStream(char* buffer, size_t size)
    : m_Length(0), m_OwnBuffer(false)
{
    if (buffer == nullptr && size != 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_Capacity = size;
    m_Buffer = buffer;
}

PdfMemoryOutputStream::~PdfMemoryOutputStream()
{
    if (m_OwnBuffer)
        delete[] m_Buffer;
}

void PdfMemoryOutputStream::WriteImpl(const char* data, size_t len)
{
    if (m_Length + len > m_Capacity)
    {
        if (m_OwnBuffer)
        {
            // A reallocation is required
            m_Capacity = std::max(m_Length + len, m_Capacity << 1);
            auto newbuffer = new char[m_Capacity];
            std::memcpy(newbuffer, m_Buffer, m_Length);
            delete m_Buffer;
            m_Buffer = newbuffer;
        }
        else
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::OutOfMemory);
        }
    }

    std::memcpy(m_Buffer + m_Length, data, len);
    m_Length += len;
}

void PdfMemoryOutputStream::Close()
{
    // Do nothing
}

unique_ptr<char[]> PdfMemoryOutputStream::TakeBuffer(size_t& length)
{
    if (!m_OwnBuffer)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported");

    length = m_Length;
    char* buffer = m_Buffer;
    m_Buffer = new char[0];
    m_Length = 0;
    m_Capacity = 0;
    return unique_ptr<char[]>(buffer);
}

PdfDeviceOutputStream::PdfDeviceOutputStream(PdfOutputDevice& device)
    : m_Device(&device) { }

void PdfDeviceOutputStream::WriteImpl(const char* data, size_t len)
{
    m_Device->Write(string_view(data, len));
}

void PdfDeviceOutputStream::Close()
{
    // Do nothing
}
