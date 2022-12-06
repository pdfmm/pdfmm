/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfStreamedObjectStream.h"

#include "PdfDocument.h"
#include "PdfEncrypt.h"
#include "PdfFilter.h"
#include "PdfOutputDevice.h"
#include "PdfObject.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

class PdfStreamedObjectStream::ObjectOutputStream : public OutputStream
{
public:
    ObjectOutputStream(PdfStreamedObjectStream& stream, OutputStreamDevice& outputStream, size_t initialLength) :
        m_objectStream(&stream),
        m_outputStream(&outputStream),
        m_initialLength(initialLength)
    {
    }

    ObjectOutputStream(PdfStreamedObjectStream& stream, unique_ptr<OutputStream> outputStream, size_t initialLength) :
        m_objectStream(&stream),
        m_outputStream(outputStream.get()),
        m_outputStreamStore(std::move(outputStream)),
        m_initialLength(initialLength)
    {
    }

    ~ObjectOutputStream()
    {
        Flush(*m_outputStream);
        m_objectStream->FinishOutput(m_initialLength);
    }

protected:
    virtual void writeBuffer(const char* buffer, size_t size)
    {
        WriteBuffer(*m_outputStream, buffer, size);
    }

    virtual void flush()
    {
        Flush(*m_outputStream);
    }

private:
    PdfStreamedObjectStream* m_objectStream;
    OutputStream* m_outputStream;
    std::unique_ptr<OutputStream> m_outputStreamStore;
    size_t m_initialLength;
};

PdfStreamedObjectStream::PdfStreamedObjectStream(PdfObject& parent, OutputStreamDevice& device) :
    PdfObjectStream(parent),
    m_Device(&device),
    m_CurrEncrypt(nullptr),
    m_Length(0)
{
    m_LengthObj = parent.GetDocument()->GetObjects().CreateObject(static_cast<int64_t>(0));
    GetParent().GetDictionary().AddKey(PdfName::KeyLength, m_LengthObj->GetIndirectReference());
}

PdfStreamedObjectStream::~PdfStreamedObjectStream()
{
    EnsureClosed();
}

void PdfStreamedObjectStream::Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt)
{
    (void)stream;
    (void)encrypt;
    // Do nothing
}

unique_ptr<InputStream> PdfStreamedObjectStream::getInputStream()
{
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

unique_ptr<OutputStream> PdfStreamedObjectStream::getOutputStream()
{
    GetParent().GetDocument()->GetObjects().WriteObject(GetParent());
    if (m_CurrEncrypt == nullptr)
    {
        return std::make_unique<ObjectOutputStream>(*this, *m_Device, m_Device->GetLength());
    }
    else
    {
        return std::make_unique<ObjectOutputStream>(*this,
            m_CurrEncrypt->CreateEncryptionOutputStream(*m_Device, GetParent().GetIndirectReference()),
            m_Device->GetLength());
    }
}

void PdfStreamedObjectStream::FinishOutput(size_t initialLength)
{
    m_Length = m_Device->GetLength() - initialLength;
    if (m_CurrEncrypt != nullptr)
        m_Length = m_CurrEncrypt->CalculateStreamLength(m_Length);

    m_LengthObj->SetNumber(static_cast<int64_t>(m_Length));
}

void PdfStreamedObjectStream::SetEncrypted(PdfEncrypt& encrypt)
{
    m_CurrEncrypt = &encrypt;
}

size_t PdfStreamedObjectStream::GetLength() const
{
    return m_Length;
}

PdfStreamedObjectStream& PdfStreamedObjectStream::operator=(const PdfStreamedObjectStream& rhs)
{
    CopyDataFrom(rhs);
    return *this;
}
