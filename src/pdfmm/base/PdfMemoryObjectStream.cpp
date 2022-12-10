/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

void PdfMemoryObjectStream::CopyDataFrom(const PdfObjectStream& rhs)
{
    const PdfMemoryObjectStream* memstream = dynamic_cast<const PdfMemoryObjectStream*>(&rhs);
    if (memstream == nullptr)
    {
        PdfObjectStream::operator=(rhs);
        return;
    }

    m_buffer = memstream->m_buffer;
    CopyFrom(rhs);
}

void PdfMemoryObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt)
{
    stream.Write("stream\n");
    if (encrypt.HasEncrypt())
    {
        charbuff encrypted;
        encrypt.EncryptTo(encrypted, { m_buffer.data(), m_buffer.size() });
        stream.Write(encrypted);
    }
    else
    {
        stream.Write(string_view(m_buffer.data(), m_buffer.size()));
    }

    stream.Write("\nendstream\n");
    stream.Flush();
}

const char* PdfMemoryObjectStream::GetData() const
{
    return m_buffer.data();
}

size_t PdfMemoryObjectStream::GetLength() const
{
    return m_buffer.size();
}

PdfMemoryObjectStream& PdfMemoryObjectStream::operator=(const PdfMemoryObjectStream& rhs)
{
    CopyDataFrom(rhs);
    return *this;
}
