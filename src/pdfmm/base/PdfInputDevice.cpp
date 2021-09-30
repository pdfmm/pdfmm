/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfInputDevice.h"

#include <cstdarg>
#include <fstream>
#include <sstream>
#include "PdfStream.h"

using namespace std;
using namespace mm;

PdfInputDevice::PdfInputDevice() :
    m_Stream(nullptr),
    m_StreamOwned(false)
{
}

PdfInputDevice::PdfInputDevice(const string_view& filename)
    : PdfInputDevice()
{
    if (filename.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_Stream = new ifstream(io::open_ifstream(filename, ios_base::in | ios_base::binary));
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, filename);

    m_StreamOwned = true;
}

PdfInputDevice::PdfInputDevice(const char* buffer, size_t len)
    : PdfInputDevice()
{
    if (len != 0 && buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    // TODO: Optimize me, offer a version that does not copy the buffer
    m_Stream = new istringstream(string(buffer, len), ios::binary);
    m_StreamOwned = true;
}

PdfInputDevice::PdfInputDevice(istream& stream)
    : PdfInputDevice()
{
    m_Stream = &stream;
    if (!m_Stream->good())
        PDFMM_RAISE_ERROR(PdfErrorCode::FileNotFound);
}

PdfInputDevice::PdfInputDevice(const PdfStream& stream)
{
    string buffer;
    PdfStringOutputStream outputStream(buffer);
    stream.GetFilteredCopy(outputStream);
    // TODO: Optimize me, offer a version that does not copy the buffer
    m_Stream = new istringstream(buffer, ios::binary);
    m_StreamOwned = true;
}

PdfInputDevice::~PdfInputDevice()
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

bool PdfInputDevice::TryGetChar(char& ch)
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

int PdfInputDevice::Look()
{
    // NOTE: We don't want a peek() call to set failbit
    if (m_Stream->eof())
        return -1;

    int ch = m_Stream->peek();
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to peek current character");

    return ch;
}

size_t PdfInputDevice::Tell()
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

void PdfInputDevice::Seek(streamoff off, ios_base::seekdir dir)
{
    return seek(off, dir);
}

void PdfInputDevice::seek(streamoff off, ios_base::seekdir dir)
{
    if (!IsSeekable())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Tried to seek an unseekable input device");

    // NOTE: Some c++ libraries don't reset eofbit prior seeking
    m_Stream->clear(m_Stream->rdstate() & ~ios_base::eofbit);
    m_Stream->seekg(off, dir);
    if (m_Stream->fail())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to seek to given position in the stream");
}

size_t PdfInputDevice::Read(char* buffer, size_t size)
{
    return io::Read(*m_Stream, buffer, size);
}

bool PdfInputDevice::Eof() const
{
    return m_Stream->eof();
}
