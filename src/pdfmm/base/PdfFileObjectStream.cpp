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

PdfFileObjectStream::PdfFileObjectStream(PdfObject& parent, PdfOutputDevice& device)
    : PdfObjectStream(parent), m_Device(&device),
    m_initialLength(0), m_Length(0), m_CurrEncrypt(nullptr)
{
    m_LengthObj = parent.GetDocument()->GetObjects().CreateObject(PdfVariant(static_cast<int64_t>(0)));
    m_Parent->GetDictionary().AddKey(PdfName::KeyLength, m_LengthObj->GetIndirectReference());
}

PdfFileObjectStream::~PdfFileObjectStream()
{
    EnsureAppendClosed();
}

void PdfFileObjectStream::Write(PdfOutputDevice&, const PdfEncrypt*)
{
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

void PdfFileObjectStream::BeginAppendImpl(const PdfFilterList& filters)
{
    m_Parent->GetDocument()->GetObjects().WriteObject(*m_Parent);

    m_initialLength = m_Device->GetLength();

    if (filters.size() != 0)
    {
        m_DeviceStream = unique_ptr<PdfDeviceOutputStream>(new PdfDeviceOutputStream(*m_Device));
        if (m_CurrEncrypt != nullptr)
        {
            m_EncryptStream = m_CurrEncrypt->CreateEncryptionOutputStream(*m_DeviceStream);
            m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_EncryptStream);
        }
        else
            m_Stream = PdfFilterFactory::CreateEncodeStream(filters, *m_DeviceStream);
    }
    else
    {
        if (m_CurrEncrypt != nullptr)
        {
            m_DeviceStream = unique_ptr<PdfDeviceOutputStream>(new PdfDeviceOutputStream(*m_Device));
            m_Stream = m_CurrEncrypt->CreateEncryptionOutputStream(*m_DeviceStream);
        }
        else
            m_Stream = unique_ptr<PdfDeviceOutputStream>(new PdfDeviceOutputStream(*m_Device));
    }
}

void PdfFileObjectStream::AppendImpl(const char* data, size_t len)
{
    m_Stream->Write(data, len);
}

void PdfFileObjectStream::EndAppendImpl()
{
    if (m_Stream != nullptr)
    {
        m_Stream->Close();
        m_Stream = nullptr;
    }

    if (m_EncryptStream != nullptr)
    {
        m_EncryptStream->Close();
        m_EncryptStream = nullptr;
    }

    if (m_DeviceStream != nullptr)
    {
        m_DeviceStream->Close();
        m_DeviceStream = nullptr;
    }

    m_Length = m_Device->GetLength() - m_initialLength;
    if (m_CurrEncrypt != nullptr)
        m_Length = m_CurrEncrypt->CalculateStreamLength(m_Length);

    m_LengthObj->SetNumber(static_cast<int64_t>(m_Length));
}

void PdfFileObjectStream::GetCopy(unique_ptr<char[]>& buffer, size_t& len) const
{
    (void)buffer;
    (void)len;
    PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

void PdfFileObjectStream::GetCopy(PdfOutputStream&) const
{
    PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

void PdfFileObjectStream::SetEncrypted(PdfEncrypt* encrypt)
{
    m_CurrEncrypt = encrypt;
    if (m_CurrEncrypt != nullptr)
        m_CurrEncrypt->SetCurrentReference(m_Parent->GetIndirectReference());
}

size_t PdfFileObjectStream::GetLength() const
{
    return m_Length;
}

const char* PdfFileObjectStream::GetInternalBuffer() const
{
    return nullptr;
}

size_t PdfFileObjectStream::GetInternalBufferSize() const
{
    return 0;
}

PdfFileObjectStream& PdfFileObjectStream::operator=(const PdfFileObjectStream& rhs)
{
    CopyFrom(rhs);
    return *this;
}
