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
#include <sstream>
#include "PdfObjectStream.h"

using namespace std;
using namespace mm;

PdfInputDevice::PdfInputDevice() { }

PdfInputDevice::~PdfInputDevice() { }

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

PdfMemoryInputDevice::PdfMemoryInputDevice(const char* buffer, size_t len)
{
    if (len != 0 && buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    // TODO: Optimize me, offer a version that does not copy the buffer
    SetStream(new istringstream(string(buffer, len), ios::binary), true);
}

PdfMemoryInputDevice::PdfMemoryInputDevice(const cspan<char>& buffer)
    : PdfMemoryInputDevice(buffer.data(), buffer.size()) { }

PdfMemoryInputDevice::PdfMemoryInputDevice(const PdfObjectStream& stream)
{
    string buffer;
    PdfStringOutputStream outputStream(buffer);
    stream.GetFilteredCopy(outputStream);
    // TODO: Optimize me, offer a version that does not copy the buffer
    SetStream(new istringstream(buffer, ios::binary), true);
}

PdfStreamInputDevice::PdfStreamInputDevice()
    : m_Stream(nullptr), m_StreamOwned(false) { }

PdfStreamInputDevice::PdfStreamInputDevice(istream& stream)
    : m_StreamOwned(false)
{
    if (!stream.good())
        PDFMM_RAISE_ERROR(PdfErrorCode::FileNotFound);

    m_Stream = &stream;
}

PdfStreamInputDevice::~PdfStreamInputDevice()
{
    this->Close();
    if (m_StreamOwned)
        delete m_Stream;
}

void PdfInputDevice::Close()
{
    // nothing to do here, but maybe necessary for inheriting classes
}

// TODO: Convert to char type
int PdfInputDevice::GetChar()
{
    char ch;
    if (TryGetChar(ch))
        return -1;
    else
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to read the current character");
}

void PdfInputDevice::Seek(streamoff off, ios_base::seekdir dir)
{
    if (!IsSeekable())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Tried to seek an unseekable input device");

    return seek(off, dir);
}

void PdfInputDevice::seek(streamoff off, ios_base::seekdir dir)
{
    (void)off;
    (void)dir;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfStreamInputDevice::TryGetChar(char& ch)
{
    if (m_Stream->eof())
    {
        ch = '\0';
        return false;
    }

    int intch = m_Stream->peek();
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to read the current character");

    if (intch == char_traits<char>::eof())
    {
        intch = '\0';
        return false;
    }

    ch = (char)(intch & 255);
    // Consume the character
    (void)m_Stream->get();
    return true;
}

int PdfStreamInputDevice::Look()
{
    // NOTE: We don't want a peek() call to set failbit
    if (m_Stream->eof())
        return -1;

    int ch = m_Stream->peek();
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to peek current character");

    return ch;
}

size_t PdfStreamInputDevice::Tell()
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

void PdfStreamInputDevice::SetStream(istream* stream, bool streamOwned)
{
    m_Stream = stream;
    m_StreamOwned = streamOwned;
}

size_t PdfStreamInputDevice::Read(char* buffer, size_t size)
{
    return utls::Read(*m_Stream, buffer, size);
}

bool PdfStreamInputDevice::Eof() const
{
    return m_Stream->eof();
}
