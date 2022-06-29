/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfOutputDevice.h"

#include <fstream>

using namespace std;
using namespace mm;

enum class PdfStreamDir
{
    Current,
    Begin,
    End
};

template <typename TStream>
size_t getPosition(TStream& stream)
{
    streampos ret;
    if (stream.eof())
    {
        // tellg() will set failbit when called on a stream that
        // is eof. Reset eofbit and restore it after calling tellg()
        // https://stackoverflow.com/q/18490576/213871
        PDFMM_ASSERT(!stream.fail());
        stream.clear();
        ret = utls::stream_helper<TStream>::tell(stream);
        stream.clear(ios_base::eofbit);
    }
    else
    {
        ret = utls::stream_helper<TStream>::tell(stream);
    }

    return (size_t)ret;
}

template <typename TStream>
size_t getLength(TStream& stream)
{
    streampos currpos;
    if (stream.eof())
    {
        // tellg() will set failbit when called on a stream that
        // is eof. Reset eofbit and restore it after calling tellg()
        // https://stackoverflow.com/q/18490576/213871
        PDFMM_ASSERT(!stream.fail());
        stream.clear();
        currpos = utls::stream_helper<TStream>::tell(stream);
        stream.clear(ios_base::eofbit);
    }
    else
    {
        streampos prevpos = utls::stream_helper<TStream>::tell(stream);
        (void)utls::stream_helper<TStream>::seek(stream, 0, ios_base::end);
        currpos = utls::stream_helper<TStream>::tell(stream);
        if (currpos != prevpos)
            (void)utls::stream_helper<TStream>::seek(stream, prevpos);
    }

    return (size_t)currpos;
}

template <typename TStream>
void seek(TStream& stream, size_t pos)
{
    (void)utls::stream_helper<TStream>::seek(stream, (std::streampos)pos);
}

PdfOutputDevice::~PdfOutputDevice() { }

size_t PdfOutputDevice::Read(char* buffer, size_t size)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return readBuffer(buffer, size);
}

void PdfOutputDevice::Seek(size_t offset)
{
    if (!CanSeek())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Tried to seek an unseekable input device");

    seek(offset);
}

void PdfOutputDevice::Flush()
{
    flush();
}

void PdfOutputDevice::Close()
{
    close();
}

bool PdfOutputDevice::CanSeek() const
{
    return false;
}

void PdfOutputDevice::close()
{
    // Do nothing
}

PdfFileOutputDevice::PdfFileOutputDevice(const string_view& filename, bool truncate)
    : PdfStreamOutputDevice(getFileStream(filename, truncate), true)
{
    if (GetStream().fail())
        PDFMM_RAISE_ERROR(PdfErrorCode::FileNotFound);
}

void PdfFileOutputDevice::close()
{
    dynamic_cast<fstream&>(GetStream()).close();
}

fstream* PdfFileOutputDevice::getFileStream(const string_view& filename, bool truncate)
{
    ios_base::openmode openmode = fstream::binary | ios_base::in | ios_base::out;
    if (truncate)
        openmode |= ios_base::trunc;

    auto stream = new fstream(utls::open_fstream(filename, openmode));
    if (!truncate)
        stream->seekp(0, ios_base::end);

    if (stream->fail())
    {
        delete stream;
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, filename);
    }

    return stream;
}

PdfStreamOutputDevice::PdfStreamOutputDevice(ostream& stream)
    : PdfStreamOutputDevice(&stream, false)
{
    if (stream.fail())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);
}

PdfStreamOutputDevice::PdfStreamOutputDevice(istream& stream)
    : PdfStreamOutputDevice(&stream, false)
{
    if (stream.fail())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);
}

PdfStreamOutputDevice::PdfStreamOutputDevice(iostream& stream)
    : PdfStreamOutputDevice(&stream, false)
{
    if (stream.fail())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDeviceOperation);

    if (stream.tellp() != stream.tellg())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation,
            "Unsupported mistmatch between read and read position in stream");
}

PdfStreamOutputDevice::~PdfStreamOutputDevice()
{
    if (m_StreamOwned)
        delete m_Stream;
}

size_t PdfStreamOutputDevice::GetLength() const
{
    size_t ret;
    if (m_istream == nullptr)
        ret = getLength(*m_ostream);
    else
        ret = getLength(*m_istream);

    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to retrieve length for this stream");

    return ret;
}

size_t PdfStreamOutputDevice::GetPosition() const
{
    size_t ret;
    if (m_istream == nullptr)
        ret = getPosition(*m_ostream);
    else
        ret = getPosition(*m_istream);

    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to get current position in the stream");

    return ret;
}

bool PdfStreamOutputDevice::CanSeek() const
{
    return true;
}

bool PdfStreamOutputDevice::Eof() const
{
    return m_Stream->eof();
}

PdfStreamOutputDevice::PdfStreamOutputDevice(ios_base* stream, bool streamOwned) :
    m_Stream(stream),
    m_istream(dynamic_cast<istream*>(stream)),
    m_ostream(dynamic_cast<ostream*>(stream)),
    m_StreamOwned(streamOwned)
{
    PDFMM_ASSERT(stream != nullptr);

    // TODO1: check stream is either istream/ostream/iostream
}

void PdfStreamOutputDevice::writeBuffer(const char* buffer, size_t size)
{
    if (m_ostream == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Unsupported write operation");

    m_ostream->write(buffer, size);
    if (m_ostream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to write the given buffer");
}

void PdfStreamOutputDevice::seek(size_t offset)
{
    // NOTE: Some c++ libraries don't reset eofbit prior seeking
    m_Stream->clear(m_Stream->rdstate() & ~ios_base::eofbit);
    if (m_istream != nullptr)
        ::seek(*m_istream, offset);

    if (m_ostream != nullptr)
        ::seek(*m_ostream, offset);

    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to seek to given position in the stream");
}

size_t PdfStreamOutputDevice::readBuffer(char* buffer, size_t size)
{
    if (m_istream == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Unsupported read operation");

    if (m_istream->eof())
        return 0;

    return utls::ReadBuffer(*m_istream, buffer, size);
}

void PdfStreamOutputDevice::flush()
{
    if (m_ostream == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Unsupported flush operation");

    m_ostream->flush();
}

PdfNullOutputDevice::PdfNullOutputDevice()
    : m_Length(0), m_Position(0)
{
}

size_t PdfNullOutputDevice::GetLength() const
{
    return m_Length;
}

size_t PdfNullOutputDevice::GetPosition() const
{
    return m_Position;
}

bool PdfNullOutputDevice::Eof() const
{
    return m_Position == m_Length;
}

void PdfNullOutputDevice::writeBuffer(const char* buffer, size_t size)
{
    (void)buffer;
    m_Position = m_Position + size;
    if (m_Position > m_Length)
        m_Length = m_Position;
}

void PdfNullOutputDevice::seek(size_t offset)
{
    if (offset > m_Length)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Seeking out of bounds");

    m_Position = offset;
}

size_t PdfNullOutputDevice::readBuffer(char* buffer, size_t size)
{
    (void)buffer;
    size_t prevpos = m_Position;
    m_Position = std::min(m_Length, m_Position + size);
    return m_Position - prevpos;
}
