/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfMemoryObjectStream.h"

#include "PdfArray.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfVariant.h"

using namespace std;
using namespace mm;

PdfMemoryObjectStream::PdfMemoryObjectStream(PdfObject& parent)
    : PdfObjectStream(parent)
{
}

PdfMemoryObjectStream::~PdfMemoryObjectStream()
{
    EnsureAppendClosed();
}

void PdfMemoryObjectStream::BeginAppendImpl(const PdfFilterList& filters)
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

void PdfMemoryObjectStream::AppendImpl(const char* data, size_t len)
{
    m_Stream->Write(data, len);
}

void PdfMemoryObjectStream::EndAppendImpl()
{
    m_Stream->Close();

    if (m_BufferStream != nullptr)
    {
        m_BufferStream->Close();
        m_BufferStream = nullptr;
    }

    m_Stream = nullptr;
}

void PdfMemoryObjectStream::GetCopy(unique_ptr<char[]>& buffer, size_t& len) const
{
    buffer.reset(new char[m_buffer.size()]);
    len = m_buffer.size();
    std::memcpy(buffer.get(), m_buffer.data(), m_buffer.size());
}

void PdfMemoryObjectStream::GetCopy(PdfOutputStream& stream) const
{
    stream.Write(m_buffer.data(), m_buffer.size());
}

void PdfMemoryObjectStream::CopyFrom(const PdfObjectStream& rhs)
{
    const PdfMemoryObjectStream* memstream = dynamic_cast<const PdfMemoryObjectStream*>(&rhs);
    if (memstream == nullptr)
    {
        PdfObjectStream::operator=(rhs);
        return;
    }

    copyFrom(*memstream);
}

void PdfMemoryObjectStream::copyFrom(const PdfMemoryObjectStream& rhs)
{
    m_buffer = rhs.m_buffer;
}

void PdfMemoryObjectStream::Write(PdfOutputDevice& device, const PdfEncrypt* encrypt)
{
    device.Write("stream\n");
    if (encrypt == nullptr)
    {
        device.Write(string_view(this->Get(), this->GetLength()));
    }
    else
    {
        string encrypted;
        encrypt->Encrypt({ this->Get(), this->GetLength() }, encrypted);
        device.Write(encrypted);
    }

    device.Write("\nendstream\n");
}

const char* PdfMemoryObjectStream::Get() const
{
    return m_buffer.data();
}

const char* PdfMemoryObjectStream::GetInternalBuffer() const
{
    return m_buffer.data();
}

size_t PdfMemoryObjectStream::GetInternalBufferSize() const
{
    return m_buffer.size();
}

size_t PdfMemoryObjectStream::GetLength() const
{
    return m_buffer.size();
}
