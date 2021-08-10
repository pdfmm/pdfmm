/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfOutputDevice.h"
#include "PdfSharedBuffer.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace mm;

PdfOutputDevice::PdfOutputDevice()
{
    this->Init();
}

PdfOutputDevice::PdfOutputDevice(const string_view& filename, bool truncate)
{
    this->Init();

    ios_base::openmode openmode = fstream::binary | ios_base::in | ios_base::out;
    if (truncate)
        openmode |= std::ios_base::trunc;

    auto stream = new fstream(io::open_fstream(filename, openmode));
    if (stream->fail())
    {
        delete stream;
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, (std::string)filename);
    }

    m_Stream = stream;
    m_ReadStream = stream;

    if (!truncate)
    {
        m_Stream->seekp(0, std::ios_base::end);
        m_Position = (size_t)m_Stream->tellp();
        m_Length = m_Position;
    }
}

PdfOutputDevice::PdfOutputDevice(char* buffer, size_t len)
{
    this->Init();

    if (buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_BufferLen = len;
    m_Buffer = buffer;
}

PdfOutputDevice::PdfOutputDevice(ostream& stream)
{
    this->Init();

    m_Stream = &stream;
    m_StreamOwned = false;
}

PdfOutputDevice::PdfOutputDevice(PdfSharedBuffer& buffer)
{
    this->Init();
    m_RefCountedBuffer = &buffer;
}

PdfOutputDevice::PdfOutputDevice(iostream& stream)
{
    this->Init();

    m_Stream = &stream;
    m_ReadStream = &stream;
    m_StreamOwned = false;
}

PdfOutputDevice::~PdfOutputDevice()
{
    if (m_StreamOwned)
        delete m_Stream;
}

void PdfOutputDevice::Init()
{
    m_Length = 0;
    m_Buffer = nullptr;
    m_Stream = nullptr;
    m_ReadStream = nullptr;
    m_RefCountedBuffer = nullptr;
    m_BufferLen = 0;
    m_Position = 0;
    m_StreamOwned = true;
}

void PdfOutputDevice::Print(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    try
    {
        Print(format, args);
    }
    catch (...)
    {
        va_end(args);
        throw;
    }
    va_end(args);
}

void PdfOutputDevice::Print(const char* format, va_list args)
{
    // NOTE: The first vsnprintf call will modify the va_list in GCC
    // so we must copy it first. Yes, it sucks. Yes, it's a shame
    va_list argscopy;
    va_copy(argscopy, args);
    int rc = compat::vsnprintf(nullptr, 0, format, args);
    if (rc < 0)
    {
        va_end(argscopy);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    try
    {
        PrintV(format, (size_t)rc, argscopy);
    }
    catch (...)
    {
        va_end(argscopy);
        throw;
    }

    va_end(argscopy);
}

void PdfOutputDevice::PrintV(const char* format, size_t size, va_list args)
{
    if (format == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (m_Buffer != nullptr)
    {
        if (m_Position + size <= m_BufferLen)
        {
            compat::vsnprintf(m_Buffer + m_Position, m_BufferLen - m_Position, format, args);
        }
        else
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::OutOfMemory);
        }
    }
    else if (m_Stream != nullptr || m_RefCountedBuffer != nullptr)
    {
        m_printBuffer.resize(size);
        compat::vsnprintf(m_printBuffer.data(), size + 1, format, args);

        if (m_Stream != nullptr)
        {
            m_Stream->write(m_printBuffer.data(), size);
            if (m_Stream->fail())
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);
        }
        else // m_RefCountedBuffer
        {
            if (m_Position + size > m_RefCountedBuffer->GetSize())
                m_RefCountedBuffer->Resize(m_Position + size);

            memcpy(m_RefCountedBuffer->GetBuffer() + m_Position, m_printBuffer.data(), size);
        }
    }

    m_Position += size;
    if (m_Position > m_Length)
        m_Length = m_Position;
}

size_t PdfOutputDevice::Read(char* buffer, size_t len)
{
    size_t numRead = 0;
    if (m_Buffer != nullptr)
    {
        if (m_Position <= m_Length)
        {
            numRead = std::min(len, m_Length - m_Position);
            memcpy(buffer, m_Buffer + m_Position, numRead);
        }
    }
    else if (m_ReadStream != nullptr)
    {
        size_t iPos = (size_t)m_ReadStream->tellg();
        m_ReadStream->read(buffer, len);
        if (m_ReadStream->fail() && !m_ReadStream->eof())
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);

        numRead = (size_t)m_ReadStream->tellg();
        numRead -= iPos;
    }
    else if (m_RefCountedBuffer != nullptr)
    {
        if (m_Position <= m_Length)
        {
            numRead = std::min(len, m_Length - m_Position);
            memcpy(buffer, m_RefCountedBuffer->GetBuffer() + m_Position, numRead);
        }
    }

    m_Position += static_cast<size_t>(numRead);
    return numRead;
}

void PdfOutputDevice::Write(const char* buffer, size_t len)
{
    if (m_Buffer != nullptr)
    {
        if (m_Position + len <= m_BufferLen)
        {
            memcpy(m_Buffer + m_Position, buffer, len);
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "Allocated buffer to small for PdfOutputDevice. Cannot write!");
        }
    }
    else if (m_Stream != nullptr)
    {
        m_Stream->write(buffer, len);
    }
    else if (m_RefCountedBuffer != nullptr)
    {
        if (m_Position + len > m_RefCountedBuffer->GetSize())
            m_RefCountedBuffer->Resize(m_Position + len);

        memcpy(m_RefCountedBuffer->GetBuffer() + m_Position, buffer, len);
    }

    m_Position += static_cast<size_t>(len);
    if (m_Position > m_Length) m_Length = m_Position;
}

void PdfOutputDevice::Put(char ch)
{
    Write(&ch, 1);
}

void PdfOutputDevice::Seek(size_t offset)
{
    if (m_Buffer != nullptr)
    {
        if (offset >= m_BufferLen)
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
        }
    }
    else if (m_Stream != nullptr)
    {
        m_Stream->seekp(offset, std::ios_base::beg);
    }
    else if (m_RefCountedBuffer != nullptr)
    {
        m_Position = offset;
    }

    // NOTE: Seek should not change the length of the device
    m_Position = offset;
}

void PdfOutputDevice::Flush()
{
    if (m_Stream != nullptr)
        m_Stream->flush();
}
