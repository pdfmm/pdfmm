/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfOutputDevice.h"

#include <fstream>
#include <sstream>

using namespace std;
using namespace mm;

PdfOutputDevice::PdfOutputDevice()
    : m_Length(0), m_Position(0)
{
}

PdfOutputDevice::~PdfOutputDevice() { }

size_t PdfOutputDevice::Read(char* buffer, size_t len)
{
    size_t readCount = read(buffer, len);
    m_Position += readCount;
    return readCount;
}

void PdfOutputDevice::Write(const string_view& view)
{
    write(view.data(), view.size());
    m_Position += view.size();
    if (m_Position > m_Length)
        m_Length = m_Position;
}

void PdfOutputDevice::Put(char ch)
{
    Write({ &ch, 1 });
}

void PdfOutputDevice::Seek(size_t offset)
{
    seek(offset);

    // NOTE: Seek should not change the length of the device
    m_Position = offset;
}

void PdfOutputDevice::Flush()
{
    flush();
}

void PdfOutputDevice::SetLength(size_t length)
{
    m_Length = length;
}

void PdfOutputDevice::SetPosition(size_t position)
{
    m_Position = position;
}

PdfFileOutputDevice::PdfFileOutputDevice(const string_view& filename, bool truncate)
    : PdfFileOutputDevice(getFileStream(filename, truncate))
{
    if (!truncate)
    {
        GetStream().seekp(0, ios_base::end);
        size_t position = (size_t)GetStream().tellp();
        SetLength(position);
        SetPosition(position);
    }
}

PdfFileOutputDevice::PdfFileOutputDevice(fstream* stream)
    : PdfStreamOutputDevice(stream, stream, true)
{
}

fstream* PdfFileOutputDevice::getFileStream(const std::string_view& filename, bool truncate)
{
    ios_base::openmode openmode = fstream::binary | ios_base::in | ios_base::out;
    if (truncate)
        openmode |= ios_base::trunc;

    auto stream = new fstream(io::open_fstream(filename, openmode));
    if (stream->fail())
    {
        delete stream;
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, filename);
    }

    return stream;
}

PdfStreamOutputDevice::PdfStreamOutputDevice(ostream& stream)
    : PdfStreamOutputDevice(&stream, nullptr, false)
{
}

PdfStreamOutputDevice::PdfStreamOutputDevice(iostream& stream)
    : PdfStreamOutputDevice(&stream, &stream, false)
{
}

PdfStreamOutputDevice::~PdfStreamOutputDevice()
{
    if (m_StreamOwned)
        delete m_Stream;
}

PdfStreamOutputDevice::PdfStreamOutputDevice(ostream* out, istream* in, bool streamOwned)
    : m_Stream(out), m_ReadStream(in), m_StreamOwned(streamOwned)
{
}

void PdfStreamOutputDevice::write(const char* buffer, size_t len)
{
    m_Stream->write(buffer, len);
}

void PdfStreamOutputDevice::seek(size_t offset)
{
    m_Stream->seekp(offset, std::ios_base::beg);
}

size_t PdfStreamOutputDevice::read(char* buffer, size_t len)
{
    if (m_ReadStream == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported read operation");

    size_t position = Tell();
    m_ReadStream->read(buffer, len);
    if (m_ReadStream->fail() && !m_ReadStream->eof())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);

    size_t readCount = (size_t)m_ReadStream->tellg();
    readCount -= position;
    return readCount;
}

void PdfStreamOutputDevice::flush()
{
    if (m_Stream != nullptr)
        m_Stream->flush();
}

PdfMemoryOutputDevice::PdfMemoryOutputDevice(char* buffer, size_t len)
{
    if (buffer == nullptr && len != 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_BufferLen = len;
    m_Buffer = buffer;
}

void PdfMemoryOutputDevice::write(const char* buffer, size_t len)
{
    size_t position = Tell();
    if (position + len > m_BufferLen)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "Allocated buffer to small for PdfOutputDevice. Cannot write!");

    std::memcpy(m_Buffer + position, buffer, len);
}

void PdfMemoryOutputDevice::seek(size_t offset)
{
    if (offset >= m_BufferLen)
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
}

size_t PdfMemoryOutputDevice::read(char* buffer, size_t len)
{
    return Read(m_Buffer, m_BufferLen, buffer, len);
}

void PdfMemoryOutputDevice::flush()
{
    // Do nothing
}

PdfNullOutputDevice::PdfNullOutputDevice()
{
}

void PdfNullOutputDevice::write(const char* buffer, size_t len)
{
    // Do nothing
    (void)buffer;
    (void)len;
}

void PdfNullOutputDevice::seek(size_t offset)
{
    // Do nothing
    (void)offset;
}

size_t PdfNullOutputDevice::read(char* buffer, size_t len)
{
    // Do nothing
    (void)buffer;
    (void)len;
    return 0;
}

void PdfNullOutputDevice::flush()
{
    // Do nothing
}

size_t PdfOutputDevice::Read(const char* src, size_t srcSize, char* dst, size_t dstLen)
{
    size_t position = Tell();
    size_t readCount = std::min(dstLen, srcSize - position);
    std::memcpy(dst, src + position, readCount);
    return readCount;
}
