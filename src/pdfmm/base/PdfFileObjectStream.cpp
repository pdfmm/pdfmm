/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFileObjectStream.h"

#include "PdfDocument.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfFileObjectStream::PdfFileObjectStream(PdfObject& parent, OutputStreamDevice& device)
    : PdfObjectStream(parent), m_Device(&device),
    m_initialLength(0), m_Length(0), m_CurrEncrypt(nullptr)
{
    m_LengthObj = parent.GetDocument()->GetObjects().CreateObject(static_cast<int64_t>(0));
    GetParent().GetDictionary().AddKey(PdfName::KeyLength, m_LengthObj->GetIndirectReference());
}

PdfFileObjectStream::~PdfFileObjectStream()
{
    EnsureAppendClosed();
}

void PdfFileObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt)
{
    (void)stream;
    (void)encrypt;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unique_ptr<InputStream> PdfFileObjectStream::GetInputStream() const
{
    // TODO
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfFileObjectStream::BeginAppendImpl(const PdfFilterList& filters)
{
    GetParent().GetDocument()->GetObjects().WriteObject(GetParent());

    m_initialLength = m_Device->GetLength();

    if (filters.size() != 0)
    {
        if (m_CurrEncrypt != nullptr)
        {
            m_EncryptStream = m_CurrEncrypt->CreateEncryptionOutputStream(*m_Device, GetParent().GetIndirectReference());
            m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_EncryptStream);
        }
        else
        {
            m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_Device);
        }
    }
    else
    {
        if (m_CurrEncrypt != nullptr)
            m_Stream = m_CurrEncrypt->CreateEncryptionOutputStream(*m_Device, GetParent().GetIndirectReference());
    }
}

void PdfFileObjectStream::AppendImpl(const char* data, size_t len)
{
    if (m_Stream == nullptr)
        m_Device->Write(data, len);
    else
        m_Stream->Write(data, len);
}

void PdfFileObjectStream::EndAppendImpl()
{
    if (m_Stream != nullptr)
    {
        m_Stream->Flush();
        m_Stream = nullptr;
    }

    if (m_EncryptStream != nullptr)
    {
        m_EncryptStream->Flush();
        m_EncryptStream = nullptr;
    }

    m_Length = m_Device->GetLength() - m_initialLength;
    if (m_CurrEncrypt != nullptr)
        m_Length = m_CurrEncrypt->CalculateStreamLength(m_Length);

    m_LengthObj->SetNumber(static_cast<int64_t>(m_Length));
}

void PdfFileObjectStream::CopyTo(OutputStream&) const
{
    PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

void PdfFileObjectStream::SetEncrypted(PdfEncrypt* encrypt)
{
    m_CurrEncrypt = encrypt;
}

size_t PdfFileObjectStream::GetLength() const
{
    return m_Length;
}

PdfFileObjectStream& PdfFileObjectStream::operator=(const PdfFileObjectStream& rhs)
{
    CopyFrom(rhs);
    return *this;
}
