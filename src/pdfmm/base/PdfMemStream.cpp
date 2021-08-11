/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfMemStream.h"

#include "PdfArray.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfVariant.h"

using namespace std;
using namespace mm;

PdfMemStream::PdfMemStream(PdfObject& parent)
    : PdfStream(parent)
{
}

PdfMemStream::~PdfMemStream()
{
    EnsureAppendClosed();
}

void PdfMemStream::BeginAppendImpl(const PdfFilterList& filters)
{
    m_buffer.clear();
    if (filters.size() != 0)
    {
        m_BufferStream = unique_ptr<PdfCharsOutputStream>(new PdfCharsOutputStream(m_buffer));
        m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_BufferStream);
    }
    else
    {
        m_Stream = unique_ptr<PdfCharsOutputStream>(new PdfCharsOutputStream(m_buffer));
    }
}

void PdfMemStream::AppendImpl(const char* data, size_t len)
{
    m_Stream->Write(data, len);
}

void PdfMemStream::EndAppendImpl()
{
    m_Stream->Close();

    if (m_BufferStream != nullptr)
    {
        m_BufferStream->Close();
        m_BufferStream = nullptr;
    }

    m_Stream = nullptr;
}

void PdfMemStream::GetCopy(unique_ptr<char[]>& buffer, size_t& len) const
{
    buffer.reset(new char[m_buffer.size()]);
    len = m_buffer.size();
    std::memcpy(buffer.get(), m_buffer.data(), m_buffer.size());
}

void PdfMemStream::GetCopy(PdfOutputStream& stream) const
{
    stream.Write(m_buffer.data(), m_buffer.size());
}

const PdfMemStream& PdfMemStream::operator=(const PdfStream& rhs)
{
    CopyFrom(rhs);
    return (*this);
}

const PdfMemStream& PdfMemStream::operator=(const PdfMemStream& rhs)
{
    CopyFrom(rhs);
    return (*this);
}

void PdfMemStream::CopyFrom(const PdfStream& rhs)
{
    const PdfMemStream* memstream = dynamic_cast<const PdfMemStream*>(&rhs);
    if (memstream == nullptr)
    {
        PdfStream::operator=(rhs);
        return;
    }

    copyFrom(*memstream);
}

void PdfMemStream::copyFrom(const PdfMemStream& rhs)
{
    m_buffer = rhs.m_buffer;
}

void PdfMemStream::Write(PdfOutputDevice& device, const PdfEncrypt* encrypt)
{
    device.Print("stream\n");
    if (encrypt == nullptr)
    {
        device.Write(this->Get(), this->GetLength());
    }
    else
    {
        string encrypted;
        encrypt->Encrypt({ this->Get(), this->GetLength() }, encrypted);
        device.Write(encrypted.data(), encrypted.size());
    }

    device.Print("\nendstream\n");
}

const char* PdfMemStream::Get() const
{
    return m_buffer.data();
}

const char* PdfMemStream::GetInternalBuffer() const
{
    return m_buffer.data();
}

size_t PdfMemStream::GetInternalBufferSize() const
{
    return m_buffer.size();
}

size_t PdfMemStream::GetLength() const
{
    return m_buffer.size();
}
