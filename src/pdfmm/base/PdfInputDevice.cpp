/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfInputDevice.h"

#include <fstream>
#include <pdfmm/private/istringviewstream.h>

#include "PdfObjectStream.h"

using namespace std;
using namespace cmn;
using namespace mm;

PdfInputDevice::~PdfInputDevice() { }

char PdfInputDevice::ReadChar()
{
    char ch;
    if (!readCharImpl(ch))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Reached EOF while reading from the stream");

    return ch;
}

bool PdfInputDevice::Read(char& ch)
{
    return readCharImpl(ch);
}

PdfFileInputDevice::PdfFileInputDevice(const string_view& filename)
{
    if (filename.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto stream = new ifstream(utls::open_ifstream(filename, ios_base::in | ios_base::binary));
    if (stream->fail())
    {
        delete stream;
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, "File not found: {}", filename);
    }

    SetStream(stream, true);
}

PdfStreamInputDevice::PdfStreamInputDevice(istream& stream)
    : m_StreamOwned(false)
{
    if (!stream.good())
        PDFMM_RAISE_ERROR(PdfErrorCode::FileNotFound);

    m_Stream = &stream;
}

PdfStreamInputDevice::~PdfStreamInputDevice()
{
    if (m_StreamOwned)
        delete m_Stream;
}

void PdfInputDevice::Close()
{
    close();
}

void PdfInputDevice::Seek(streamoff off, ios_base::seekdir dir)
{
    if (!CanSeek())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Tried to seek an unseekable input device");

    seek(off, dir);
}

size_t PdfInputDevice::Read(char* buffer, size_t size)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return readBufferImpl(buffer, size);
}

bool PdfInputDevice::CanSeek() const
{
    return false;
}

void PdfInputDevice::seek(streamoff off, ios_base::seekdir dir)
{
    (void)off;
    (void)dir;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfInputDevice::close()
{
    // Do nothing
}

bool PdfInputDevice::readCharImpl(char& ch)
{
    ch = '\0';
    return readBufferImpl(&ch, 1) == 1;
}

size_t PdfInputDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    size_t ret = readBufferImpl(buffer, size);
    eof = ret == 0;
    return ret;
}

bool PdfInputDevice::readChar(char& ch, bool& eof)
{
    bool success = readCharImpl(ch);
    eof = !success;
    return success;
}

PdfStreamInputDevice::PdfStreamInputDevice()
    : m_Stream(nullptr), m_StreamOwned(false) { }

int PdfStreamInputDevice::Peek()
{
    // NOTE: We don't want a peek() call to set failbit
    if (m_Stream->eof())
        return char_traits<char>::eof();

    int ch = m_Stream->peek();
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to peek current character");

    return ch;
}

size_t PdfStreamInputDevice::GetPosition()
{
    streamoff ret;
    if (m_Stream->eof())
    {
        // tellg() will set failbit when called on a stream that
        // is eof. Reset eofbit and restore it after calling tellg()
        // https://stackoverflow.com/q/18490576/213871
        PDFMM_ASSERT(!m_Stream->fail());
        m_Stream->clear();
        ret = m_Stream->tellg();
        m_Stream->clear(ios_base::eofbit);
    }
    else
    {
        ret = m_Stream->tellg();
    }

    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to get current position in the stream");

    return (size_t)ret;
}

void PdfStreamInputDevice::seek(streamoff off, ios_base::seekdir dir)
{
    // NOTE: Some c++ libraries don't reset eofbit prior seeking
    m_Stream->clear(m_Stream->rdstate() & ~ios_base::eofbit);
    m_Stream->seekg(off, dir);
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to seek to given position in the stream");
}

size_t PdfStreamInputDevice::readBufferImpl(char* buffer, size_t size)
{
    if (m_Stream->eof())
        return 0;

    return utls::ReadBuffer(*m_Stream, buffer, size);
}

bool PdfStreamInputDevice::readCharImpl(char& ch)
{
    if (m_Stream->eof())
    {
        ch = '\0';
        return false;
    }

    return utls::ReadChar(*m_Stream, ch);
}

void PdfStreamInputDevice::SetStream(istream* stream, bool streamOwned)
{
    m_Stream = stream;
    m_StreamOwned = streamOwned;
}

bool PdfStreamInputDevice::Eof() const
{
    return m_Stream->eof();
}

bool PdfStreamInputDevice::CanSeek() const
{
    return true;
}

PdfMemoryInputDevice::PdfMemoryInputDevice(const char* buffer, size_t size)
    : m_buffer(buffer), m_Length(size), m_Position(0)
{
    if (size != 0 && buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

PdfMemoryInputDevice::PdfMemoryInputDevice(const bufferview& buffer)
    : PdfMemoryInputDevice(buffer.data(), buffer.size()) { }

PdfMemoryInputDevice::PdfMemoryInputDevice(const string_view& view)
    : PdfMemoryInputDevice(view.data(), view.size()) { }

PdfMemoryInputDevice::PdfMemoryInputDevice(const string& str)
    : PdfMemoryInputDevice(str.data(), str.size()) { }

PdfMemoryInputDevice::PdfMemoryInputDevice(const char* str)
    : PdfMemoryInputDevice(str, char_traits<char>::length(str)) { }

size_t PdfMemoryInputDevice::GetPosition()
{
    return m_Position;
}

int PdfMemoryInputDevice::Peek()
{
    if (m_Position == m_Length)
        return char_traits<char>::eof();

    return (unsigned char)m_buffer[m_Position];
}

bool PdfMemoryInputDevice::Eof() const
{
    return m_Position == m_Length;
}

bool PdfMemoryInputDevice::CanSeek() const
{
    return true;
}

void PdfMemoryInputDevice::seek(streamoff off, ios_base::seekdir dir)
{
    switch (dir)
    {
        case ios_base::beg:
        {
            if (off < 0)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid negative seek");
            else if (off > m_Length)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");

            m_Position = (size_t)off;
            break;
        }
        case ios_base::cur:
        {
            if (off == 0)
            {
                // No modification
                break;
            }
            else if (off > 0)
            {
                if (off > m_Length - m_Position)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");
            }
            else
            {
                if (-off > m_Position)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");
            }

            m_Position += (size_t)off;
            break;
        }
        case ios_base::end:
        {
            if (off > 0)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Invalid negative seek");
            else if (-off > m_Length)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Invalid seek out of bounds");

            m_Position = (size_t)(m_Length + off);
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }
    }
}

size_t PdfMemoryInputDevice::readBufferImpl(char* buffer, size_t size)
{
    size_t readCount = std::min(size, m_Length - m_Position);
    std::memcpy(buffer, m_buffer + m_Position, readCount);
    m_Position += readCount;
    return readCount;
}

bool PdfMemoryInputDevice::readCharImpl(char& ch)
{
    if (m_Position == m_Length)
    {
        ch = '\0';
        return false;
    }

    ch = m_buffer[m_Position];
    m_Position++;
    return true;
}
