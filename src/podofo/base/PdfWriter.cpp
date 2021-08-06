/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfWriter.h"

#include <iostream>

#include "PdfData.h"
#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"

#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
// 10 spaces
#define LINEARIZATION_PADDING "          "

using namespace std;
using namespace PoDoFo;

PdfWriter::PdfWriter(PdfVecObjects* objects, const PdfObject& trailer, PdfVersion version) :
    m_Objects(objects),
    m_Trailer(trailer),
    m_Version(version),
    m_UseXRefStream(false),
    m_EncryptObj(nullptr),
    m_saveOptions(PdfSaveOptions::None),
    m_WriteMode(PdfWriteMode::Compact),
    m_PrevXRefOffset(0),
    m_IncrementalUpdate(false),
    m_rewriteXRefTable(false),
    m_FirstInXRef(0),
    m_LinearizedOffset(0),
    m_LinearizedLastOffset(0),
    m_TrailerOffset(0)
{
}

PdfWriter::PdfWriter(PdfVecObjects& objects, const PdfObject& trailer)
    : PdfWriter(&objects, trailer, PdfVersionDefault)
{
}

PdfWriter::PdfWriter(PdfVecObjects& objects)
    : PdfWriter(&objects, PdfObject(), PdfVersionDefault)
{
}

void PdfWriter::SetIncrementalUpdate(bool rewriteXRefTable)
{
    m_IncrementalUpdate = true;
    m_rewriteXRefTable = rewriteXRefTable;
}

const char* PdfWriter::GetPdfVersionString() const
{
    return s_PdfVersionNums[static_cast<int>(m_Version)];
}

PdfWriter::~PdfWriter()
{
    m_Objects = nullptr;
}

void PdfWriter::Write(PdfOutputDevice& device)
{
    CreateFileIdentifier(m_identifier, m_Trailer, &m_originalIdentifier);

    // setup encrypt dictionary
    if (m_Encrypt)
    {
        m_Encrypt->GenerateEncryptionKey(m_identifier);

        // Add our own Encryption dictionary
        m_EncryptObj = m_Objects->CreateDictionaryObject();
        m_Encrypt->CreateEncryptionDictionary(m_EncryptObj->GetDictionary());
    }

    unique_ptr<PdfXRef> pXRef;
    if (m_UseXRefStream)
        pXRef.reset(new PdfXRefStream(*this, *m_Objects));
    else
        pXRef.reset(new PdfXRef(*this));

    try
    {
        if (!m_IncrementalUpdate)
            WritePdfHeader(device);

        WritePdfObjects(device, *m_Objects, *pXRef);

        if (m_IncrementalUpdate)
            pXRef->SetFirstEmptyBlock();

        pXRef->Write(device);
    }
    catch (PdfError& e)
    {
        // P.Zent: Delete Encryption dictionary (cannot be reused)
        if (m_EncryptObj)
        {
            m_Objects->RemoveObject(m_EncryptObj->GetIndirectReference());
            m_EncryptObj = nullptr;
        }

        e.AddToCallstack(__FILE__, __LINE__);
        throw e;
    }

    // P.Zent: Delete Encryption dictionary (cannot be reused)
    if (m_EncryptObj)
    {
        m_Objects->RemoveObject(m_EncryptObj->GetIndirectReference());
        m_EncryptObj = nullptr;
    }
}

void PdfWriter::WritePdfHeader(PdfOutputDevice& device)
{
    device.Print("%s\n%%%s", s_PdfVersions[static_cast<int>(m_Version)], PDF_MAGIC);
}

void PdfWriter::WritePdfObjects(PdfOutputDevice& device, const PdfVecObjects& objects, PdfXRef& xref)
{
    for (PdfObject* obj : objects)
    {
        if (m_IncrementalUpdate)
        {
            if (!obj->IsDirty())
            {
                if (m_rewriteXRefTable)
                {
                    PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(obj);
                    if (parserObject != nullptr)
                    {
                        // Try to see if we can just write the reference to previous entry
                        // without rewriting the entry

                        // the reference looks like "0 0 R", while the object identifier like "0 0 obj", thus add two letters
                        size_t objRefLength = obj->GetIndirectReference().ToString().length() + 2;

                        // the offset points just after the "0 0 obj" string
                        if (parserObject->GetOffset() - objRefLength > 0)
                        {
                            xref.AddInUseObject(obj->GetIndirectReference(), parserObject->GetOffset() - objRefLength);
                            continue;
                        }
                    }
                }
                else
                {
                    // The object will not be output in the XRef entries but it will be
                    // counted in trailer's /Size
                    xref.AddInUseObject(obj->GetIndirectReference(), std::nullopt);
                    continue;
                }
            }
        }

        xref.AddInUseObject(obj->GetIndirectReference(), device.Tell());

        if (!xref.ShouldSkipWrite(obj->GetIndirectReference()))
        {
            // Also make sure that we do not encrypt the encryption dictionary!
            obj->Write(device, m_WriteMode, obj == m_EncryptObj ? nullptr : m_Encrypt.get());
        }
    }

    for (auto& freeObjectRef : objects.GetFreeObjects())
    {
        xref.AddFreeObject(freeObjectRef);
    }
}

void PdfWriter::FillTrailerObject(PdfObject& trailer, size_t size, bool onlySizeKey) const
{
    trailer.GetDictionary().AddKey(PdfName::KeySize, static_cast<int64_t>(size));

    if (!onlySizeKey)
    {
        if (m_Trailer.GetDictionary().HasKey("Root"))
            trailer.GetDictionary().AddKey("Root", *m_Trailer.GetDictionary().GetKey("Root"));
        // It makes no sense to simple copy an encryption key
        // Either we have no encryption or we encrypt again by ourselves
        if (m_Trailer.GetDictionary().HasKey("Info"))
            trailer.GetDictionary().AddKey("Info", *m_Trailer.GetDictionary().GetKey("Info"));

        if (m_EncryptObj != nullptr)
            trailer.GetDictionary().AddKey(PdfName("Encrypt"), m_EncryptObj->GetIndirectReference());

        PdfArray array;
        // The ID is the same unless the PDF was incrementally updated
        if (m_IncrementalUpdate && m_originalIdentifier.GetLength() > 0)
        {
            array.push_back(m_originalIdentifier);
        }
        else
        {
            array.push_back(m_identifier);
        }
        array.push_back(m_identifier);

        // finally add the key to the trailer dictionary
        trailer.GetDictionary().AddKey("ID", array);

        if (!m_rewriteXRefTable && m_PrevXRefOffset > 0)
        {
            PdfVariant value(m_PrevXRefOffset);
            trailer.GetDictionary().AddKey("Prev", value);
        }
    }
}

void PdfWriter::CreateFileIdentifier(PdfString& identifier, const PdfObject& trailer, PdfString* originalIdentifier) const
{
    PdfOutputDevice length;
    unique_ptr<PdfObject> info;
    bool originalIdentifierFound = false;

    if (originalIdentifier && trailer.GetDictionary().HasKey("ID"))
    {
        const PdfObject* idObj = trailer.GetDictionary().GetKey("ID");
        // The PDF spec, section 7.5.5, implies that the ID may be
        // indirect as long as the PDF is not encrypted. Handle that
        // case.
        if (idObj->IsReference())
            idObj = &m_Objects->MustGetObject(idObj->GetReference());

        auto it = idObj->GetArray().begin();
        PdfString str;
        if (it != idObj->GetArray().end() && it->TryGetString(str) && str.IsHex())
        {
            *originalIdentifier = it->GetString();
            originalIdentifierFound = true;
        }
    }

    // create a dictionary with some unique information.
    // This dictionary is based on the PDF files information
    // dictionary if it exists.
    auto infoObj = trailer.GetDictionary().GetKey("Info");;
    if (infoObj == nullptr)
    {
        PdfDate date;
        PdfString dateString = date.ToString();

        info.reset(new PdfObject());
        info->GetDictionary().AddKey("CreationDate", dateString);
        info->GetDictionary().AddKey("Creator", PdfString("PoDoFo"));
        info->GetDictionary().AddKey("Producer", PdfString("PoDoFo"));
    }
    else
    {
        PdfReference ref;
        if (infoObj->TryGetReference(ref))
        {
            infoObj = m_Objects->GetObject(ref);

            if (infoObj == nullptr)
            {
                ostringstream oss;
                oss << "Error while retrieving info dictionary: "
                    << ref.ObjectNumber() << " "
                    << ref.GenerationNumber() << " R" << std::endl;
                PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, oss.str().c_str());
            }
            else
            {
                info.reset(new PdfObject(*infoObj));
            }
        }
        else if (infoObj->IsDictionary())
        {
            // NOTE: While Table 15, ISO 32000-1:2008, states that Info should be an
            // indirect reference, we found Pdfs, for example produced
            // by pdfjs v0.4.1 (github.com/rkusa/pdfjs) that do otherwise.
            // As usual, Acroat Pro Syntax checker doesn't care about this,
            // so let's just read it
            info.reset(new PdfObject(*infoObj));
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Invalid ");
        }
    }

    info->GetDictionary().AddKey("Location", PdfString("SOMEFILENAME"));
    info->Write(length, m_WriteMode, nullptr);

    PdfRefCountedBuffer buffer(length.GetLength());
    PdfOutputDevice device(buffer);
    info->Write(device, m_WriteMode, nullptr);

    // calculate the MD5 Sum
    identifier = PdfEncryptMD5Base::GetMD5String(reinterpret_cast<unsigned char*>(buffer.GetBuffer()),
        static_cast<unsigned>(length.GetLength()));

    if (originalIdentifier && !originalIdentifierFound)
        *originalIdentifier = identifier;
}

void PdfWriter::SetEncryptObj(PdfObject* obj)
{
    m_EncryptObj = obj;
}

void PdfWriter::SetEncrypted(const PdfEncrypt& rEncrypt)
{
    m_Encrypt = PdfEncrypt::CreatePdfEncrypt(rEncrypt);
}

void PdfWriter::SetUseXRefStream(bool bStream)
{
    if (bStream && m_Version < PdfVersion::V1_5)
        this->SetPdfVersion(PdfVersion::V1_5);

    m_UseXRefStream = bStream;
}
