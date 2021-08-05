/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfFileStream.h"

#include <doc/PdfDocument.h>
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfOutputDevice.h"
#include "PdfOutputStream.h"
#include "PdfObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

PdfFileStream::PdfFileStream(PdfObject& parent, PdfOutputDevice& device)
    : PdfStream(parent), m_Device(&device),
    m_initialLength(0), m_Length(0), m_CurrEncrypt(nullptr)
{
    m_LengthObj = parent.GetDocument()->GetObjects().CreateObject(PdfVariant(static_cast<int64_t>(0)));
    m_Parent->GetDictionary().AddKey(PdfName::KeyLength, m_LengthObj->GetIndirectReference());
}

PdfFileStream::~PdfFileStream()
{
    EnsureAppendClosed();
}

void PdfFileStream::Write(PdfOutputDevice&, const PdfEncrypt*)
{
    PODOFO_RAISE_ERROR(EPdfError::NotImplemented);
}

void PdfFileStream::BeginAppendImpl(const PdfFilterList& filters)
{
    m_Parent->GetDocument()->GetObjects().WriteObject(*m_Parent);

    m_initialLength = m_Device->GetLength();

    if (filters.size())
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

void PdfFileStream::AppendImpl(const char* data, size_t len)
{
    m_Stream->Write(data, len);
}

void PdfFileStream::EndAppendImpl()
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

void PdfFileStream::GetCopy(char**, size_t*) const
{
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

void PdfFileStream::GetCopy(PdfOutputStream&) const
{
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

void PdfFileStream::SetEncrypted(PdfEncrypt* encrypt)
{
    m_CurrEncrypt = encrypt;
    if (m_CurrEncrypt != nullptr)
        m_CurrEncrypt->SetCurrentReference(m_Parent->GetIndirectReference());
}

size_t PdfFileStream::GetLength() const
{
    return m_Length;
}

const char* PdfFileStream::GetInternalBuffer() const
{
    return nullptr;
}

size_t PdfFileStream::GetInternalBufferSize() const
{
    return 0;
}
