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
    : PdfStream(parent), m_Length(0)
{
}

PdfMemStream::~PdfMemStream()
{
    EnsureAppendClosed();
}

void PdfMemStream::BeginAppendImpl(const PdfFilterList& filters)
{
    m_buffer = PdfRefCountedBuffer();
    m_Length = 0;

    if (filters.size() != 0)
    {
        m_BufferStream = unique_ptr<PdfBufferOutputStream>(new PdfBufferOutputStream(m_buffer));
        m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_BufferStream);
    }
    else
        m_Stream = unique_ptr<PdfBufferOutputStream>(new PdfBufferOutputStream(m_buffer));
}

void PdfMemStream::AppendImpl(const char* data, size_t len)
{
    m_Stream->Write(data, len);
}

void PdfMemStream::EndAppendImpl()
{
    if (m_Stream != nullptr)
    {
        m_Stream->Close();

        if (m_BufferStream == nullptr)
        {
            PdfBufferOutputStream* bufferOutputStream = dynamic_cast<PdfBufferOutputStream*>(m_Stream.get());
            if (bufferOutputStream != nullptr)
                m_Length = bufferOutputStream->GetLength();
        }

        m_Stream = nullptr;
    }

    if (m_BufferStream != nullptr)
    {
        m_BufferStream->Close();
        m_Length = m_BufferStream->GetLength();
        m_BufferStream = nullptr;
    }
}

void PdfMemStream::GetCopy(char** buffer, size_t* len) const
{
    if (buffer == nullptr || len == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    *buffer = static_cast<char*>(pdfmm_calloc(m_Length, sizeof(char)));
    *len = m_Length;

    if (*buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::OutOfMemory);

    memcpy(*buffer, m_buffer.GetBuffer(), m_Length);
}


void PdfMemStream::GetCopy(PdfOutputStream& stream) const
{
    stream.Write(m_buffer.GetBuffer(), m_Length);
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
    m_Length = rhs.GetLength();
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
    return m_buffer.GetBuffer();
}

const char* PdfMemStream::GetInternalBuffer() const
{
    return m_buffer.GetBuffer();
}

size_t PdfMemStream::GetInternalBufferSize() const
{
    return m_Length;
}

size_t PdfMemStream::GetLength() const
{
    return m_Length;
}
