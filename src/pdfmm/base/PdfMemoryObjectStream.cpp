/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfMemoryObjectStream.h"

#include "PdfArray.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfObject.h"
#include "PdfStreamDevice.h"

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

unique_ptr<InputStream> PdfMemoryObjectStream::GetInputStream() const
{
    return std::unique_ptr<InputStream>(new SpanStreamDevice(m_buffer));
}

void PdfMemoryObjectStream::BeginAppendImpl(const PdfFilterList& filters)
{
    m_buffer.clear();
    if (filters.size() == 0)
    {
        m_Stream = unique_ptr<BufferStreamDevice>(new BufferStreamDevice(m_buffer));
    }
    else
    {
        m_BufferStream = unique_ptr<BufferStreamDevice>(new BufferStreamDevice(m_buffer));
        m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_BufferStream);
    }
}

void PdfMemoryObjectStream::AppendImpl(const char* data, size_t len)
{
    m_Stream->Write(data, len);
}

void PdfMemoryObjectStream::EndAppendImpl()
{
    m_Stream->Flush();
    m_Stream = nullptr;

    if (m_BufferStream != nullptr)
    {
        m_BufferStream->Flush();
        m_BufferStream = nullptr;
    }
}

void PdfMemoryObjectStream::CopyTo(OutputStream& stream) const
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

void PdfMemoryObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt)
{
    stream.Write("stream\n");
    if (encrypt.HasEncrypt())
    {
        charbuff encrypted;
        encrypt.EncryptTo(encrypted, { this->Get(), this->GetLength() });
        stream.Write(encrypted);
    }
    else
    {
        stream.Write(string_view(this->Get(), this->GetLength()));
    }

    stream.Write("\nendstream\n");
    stream.Flush();
}

const char* PdfMemoryObjectStream::Get() const
{
    return m_buffer.data();
}

size_t PdfMemoryObjectStream::GetLength() const
{
    return m_buffer.size();
}

PdfMemoryObjectStream& PdfMemoryObjectStream::operator=(const PdfMemoryObjectStream& rhs)
{
    CopyFrom(rhs);
    return *this;
}
