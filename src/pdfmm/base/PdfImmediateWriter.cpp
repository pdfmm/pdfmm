/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfImmediateWriter.h"

#include "PdfFileStream.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"

using namespace mm;

PdfImmediateWriter::PdfImmediateWriter(PdfIndirectObjectList& objects, const PdfObject& trailer,
    PdfOutputDevice& device, PdfVersion version, PdfEncrypt* encrypt, PdfWriteMode writeMode) :
    PdfWriter(objects, trailer),
    m_attached(true),
    m_Device(&device),
    m_Last(nullptr),
    m_OpenStream(false)
{
    // register as observer for PdfIndirectObjectList
    GetObjects().Attach(this);
    // register as stream factory for PdfIndirectObjectList
    GetObjects().SetStreamFactory(this);

    PdfString identifier;
    this->CreateFileIdentifier(identifier, trailer);
    SetIdentifier(identifier);

    // setup encryption
    if (encrypt != nullptr)
    {
        this->SetEncrypted(*encrypt);
        encrypt->GenerateEncryptionKey(GetIdentifier());
    }

    // start with writing the header
    this->SetPdfVersion(version);
    this->SetWriteMode(writeMode);
    this->WritePdfHeader(*m_Device);

    m_xRef.reset(GetUseXRefStream() ? new PdfXRefStream(*this) : new PdfXRef(*this));
}

PdfImmediateWriter::~PdfImmediateWriter()
{
    if (m_attached)
        GetObjects().Detach(this);
}

PdfWriteMode PdfImmediateWriter::GetWriteMode() const
{
    return PdfWriter::GetWriteMode();
}

PdfVersion PdfImmediateWriter::GetPdfVersion() const
{
    return PdfWriter::GetPdfVersion();
}

void PdfImmediateWriter::WriteObject(const PdfObject& obj)
{
    const int endObjLenght = 7;

    this->FinishLastObject();

    m_xRef->AddInUseObject(obj.GetIndirectReference(), m_Device->Tell());
    obj.Write(*m_Device, this->GetWriteMode(), GetEncrypt());
    // Make sure, no one will add keys now to the object
    const_cast<PdfObject&>(obj).SetImmutable(true);

    // Let's cheat a bit:
    // obj has written an "endobj\n" as last data to the file.
    // we simply overwrite this string with "stream\n" which 
    // has excatly the same length.
    m_Device->Seek(m_Device->Tell() - endObjLenght);
    m_Device->Write("stream\n");
    m_Last = const_cast<PdfObject*>(&obj);
}

void PdfImmediateWriter::Finish()
{
    // write all objects which are still in RAM
    this->FinishLastObject();

    // setup encrypt dictionary
    if (GetEncrypt() != nullptr)
    {
        // Add our own Encryption dictionary
        SetEncryptObj(GetObjects().CreateDictionaryObject());
        GetEncrypt()->CreateEncryptionDictionary(GetEncryptObj()->GetDictionary());
    }

    this->WritePdfObjects(*m_Device, GetObjects(), *m_xRef);

    // write the XRef
    uint64_t lXRefOffset = static_cast<uint64_t>(m_Device->Tell());
    m_xRef->Write(*m_Device);

    // FIX-ME: The following is already done by PdfXRef now
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);

    // XRef streams contain the trailer in the XRef
    if (!GetUseXRefStream())
    {
        PdfObject trailer;

        // if we have a dummy offset we write also a prev entry to the trailer
        FillTrailerObject(trailer, m_xRef->GetSize(), false);

        m_Device->Write("trailer\n");
        trailer.Write(*m_Device, this->GetWriteMode(), nullptr);
    }

    m_Device->Write(PDFMM_FORMAT("startxref\n{}\n%%EOF\n", lXRefOffset));
    m_Device->Flush();

    // we are done now
    GetObjects().Detach(this);
    m_attached = false;
}

PdfStream* PdfImmediateWriter::CreateStream(PdfObject& parent)
{
    return m_OpenStream ?
        static_cast<PdfStream*>(new PdfMemStream(parent)) :
        static_cast<PdfStream*>(new PdfFileStream(parent, *m_Device));
}

void PdfImmediateWriter::FinishLastObject()
{
    if (m_Last != nullptr)
    {
        m_Device->Write("\nendstream\n");
        m_Device->Write("endobj\n");

        GetObjects().RemoveObject(m_Last->GetIndirectReference(), false);
        m_Last = nullptr;
    }
}

void PdfImmediateWriter::BeginAppendStream(const PdfStream& stream)
{
    auto fileStream = dynamic_cast<const PdfFileStream*>(&stream);
    if (fileStream != nullptr)
    {
        // Only one open file stream is allowed at a time
        PDFMM_ASSERT(!m_OpenStream);
        m_OpenStream = true;

        if (GetEncrypt() != nullptr)
            const_cast<PdfFileStream*>(fileStream)->SetEncrypted(GetEncrypt());
    }
}

void PdfImmediateWriter::EndAppendStream(const PdfStream& stream)
{
    auto fileStream = dynamic_cast<const PdfFileStream*>(&stream);
    if (fileStream != nullptr)
    {
        // A PdfFileStream has to be opened before
        PDFMM_ASSERT(m_OpenStream);
        m_OpenStream = false;
    }
}
