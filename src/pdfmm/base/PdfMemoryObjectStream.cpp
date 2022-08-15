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
    EnsureClosed();
}

unique_ptr<InputStream> PdfMemoryObjectStream::getInputStream()
{
    return unique_ptr<InputStream>(new SpanStreamDevice(m_buffer));
}

unique_ptr<OutputStream> PdfMemoryObjectStream::getOutputStream()
{
    m_buffer.clear();
    return unique_ptr<OutputStream>(new StringStreamDevice(m_buffer));
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
