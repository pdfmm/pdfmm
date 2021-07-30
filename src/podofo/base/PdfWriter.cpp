/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
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

PdfWriter::PdfWriter(PdfVecObjects* pVecObjects, const PdfObject& pTrailer, PdfVersion version) :
    m_vecObjects(pVecObjects),
    m_Trailer(pTrailer),
    m_eVersion(version),
    m_UseXRefStream(false),
    m_pEncryptObj(nullptr),
    m_saveOptions(PdfSaveOptions::None),
    m_eWriteMode(PdfWriteMode::Compact),
    m_lPrevXRefOffset(0),
    m_bIncrementalUpdate(false),
    m_rewriteXRefTable(false),
    m_lFirstInXRef(0),
    m_lLinearizedOffset(0),
    m_lLinearizedLastOffset(0),
    m_lTrailerOffset(0)
{
}

PdfWriter::PdfWriter( PdfVecObjects& pVecObjects, const PdfObject& trailer )
    : PdfWriter(&pVecObjects, trailer, PdfVersionDefault)
{
}

PdfWriter::PdfWriter(PdfVecObjects& pVecObjects)
    : PdfWriter(&pVecObjects, PdfObject(), PdfVersionDefault)
{
}

void PdfWriter::SetIncrementalUpdate(bool rewriteXRefTable)
{
    m_bIncrementalUpdate = true;
    m_rewriteXRefTable = rewriteXRefTable;
}

const char* PdfWriter::GetPdfVersionString() const
{
    return s_szPdfVersionNums[static_cast<int>(m_eVersion)];
}

PdfWriter::~PdfWriter()
{
    m_vecObjects = nullptr;
}

void PdfWriter::Write(PdfOutputDevice& device)
{
    CreateFileIdentifier( m_identifier, m_Trailer, &m_originalIdentifier );

    // setup encrypt dictionary
    if( m_pEncrypt )
    {
        m_pEncrypt->GenerateEncryptionKey( m_identifier );

        // Add our own Encryption dictionary
        m_pEncryptObj = m_vecObjects->CreateDictionaryObject();
        m_pEncrypt->CreateEncryptionDictionary( m_pEncryptObj->GetDictionary() );
    }

    unique_ptr<PdfXRef> pXRef;
    if (m_UseXRefStream)
        pXRef.reset(new PdfXRefStream(*this, *m_vecObjects));
    else
        pXRef.reset(new PdfXRef(*this));

    try
    {
        if( !m_bIncrementalUpdate )
            WritePdfHeader(device);

        WritePdfObjects(device, *m_vecObjects, *pXRef);

        if ( m_bIncrementalUpdate )
            pXRef->SetFirstEmptyBlock();

        pXRef->Write(device);
    }
    catch( PdfError & e )
    {   
        // P.Zent: Delete Encryption dictionary (cannot be reused)
        if(m_pEncryptObj)
        {
            m_vecObjects->RemoveObject(m_pEncryptObj->GetIndirectReference());
            m_pEncryptObj = nullptr;
        }
        
        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }
    
    // P.Zent: Delete Encryption dictionary (cannot be reused)
    if(m_pEncryptObj)
    {
        m_vecObjects->RemoveObject(m_pEncryptObj->GetIndirectReference());
        m_pEncryptObj = nullptr;
    }
}

void PdfWriter::WritePdfHeader(PdfOutputDevice& device)
{
    device.Print( "%s\n%%%s", s_szPdfVersions[static_cast<int>(m_eVersion)], PDF_MAGIC );
}

void PdfWriter::WritePdfObjects(PdfOutputDevice& device, const PdfVecObjects& vecObjects, PdfXRef& xref)
{
    for(PdfObject* pObject : vecObjects )
    {
	    if( m_bIncrementalUpdate )
        {
            if(!pObject->IsDirty())
            {
                if (m_rewriteXRefTable)
                {
                    PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(pObject);
                    if (parserObject != nullptr)
                    {
                        // Try to see if we can just write the reference to previous entry
                        // without rewriting the entry

                        // the reference looks like "0 0 R", while the object identifier like "0 0 obj", thus add two letters
                        size_t objRefLength = pObject->GetIndirectReference().ToString().length() + 2;

                        // the offset points just after the "0 0 obj" string
                        if (parserObject->GetOffset() - objRefLength > 0)
                        {
                            xref.AddInUseObject(pObject->GetIndirectReference(), parserObject->GetOffset() - objRefLength);
                            continue;
                        }
                    }
                }
                else
                {
                    // The object will not be output in the XRef entries but it will be
                    // counted in trailer's /Size
                    xref.AddInUseObject(pObject->GetIndirectReference(), std::nullopt);
                    continue;
                }
            }
        }

        xref.AddInUseObject( pObject->GetIndirectReference(), device.Tell());

        if (!xref.ShouldSkipWrite(pObject->GetIndirectReference()))
        {
            // Also make sure that we do not encrypt the encryption dictionary!
            pObject->Write(device, m_eWriteMode, pObject == m_pEncryptObj ? nullptr : m_pEncrypt.get());
        }
    }

    for(auto& freeObjectRef : vecObjects.GetFreeObjects())
    {
        xref.AddFreeObject(freeObjectRef);
    }
}

void PdfWriter::FillTrailerObject(PdfObject& trailer, size_t lSize, bool bOnlySizeKey) const
{
    trailer.GetDictionary().AddKey(PdfName::KeySize, static_cast<int64_t>(lSize));

    if (!bOnlySizeKey)
    {
        if (m_Trailer.GetDictionary().HasKey("Root"))
            trailer.GetDictionary().AddKey("Root", *m_Trailer.GetDictionary().GetKey("Root"));
        /*
          DominikS: It makes no sense to simple copy an encryption key
                    Either we have no encryption or we encrypt again by ourselves
        if( m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )
            pTrailer->GetDictionary().AddKey( "Encrypt", m_pTrailer->GetDictionary().GetKey( "Encrypt" ) );
        */
        if (m_Trailer.GetDictionary().HasKey("Info"))
            trailer.GetDictionary().AddKey("Info", *m_Trailer.GetDictionary().GetKey("Info"));


        if (m_pEncryptObj)
            trailer.GetDictionary().AddKey(PdfName("Encrypt"), m_pEncryptObj->GetIndirectReference());

        PdfArray array;
        // The ID is the same unless the PDF was incrementally updated
        if (m_bIncrementalUpdate && m_originalIdentifier.GetLength() > 0)
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

        if (!m_rewriteXRefTable && m_lPrevXRefOffset > 0)
        {
            PdfVariant value(m_lPrevXRefOffset);
            trailer.GetDictionary().AddKey("Prev", value);
        }
    }
}

void PdfWriter::CreateFileIdentifier(PdfString& identifier, const PdfObject& pTrailer, PdfString* pOriginalIdentifier) const
{
    PdfOutputDevice length;
    unique_ptr<PdfObject> pInfo;
    bool bOriginalIdentifierFound = false;

    if (pOriginalIdentifier && pTrailer.GetDictionary().HasKey("ID"))
    {
        const PdfObject* idObj = pTrailer.GetDictionary().GetKey("ID");
        // The PDF spec, section 7.5.5, implies that the ID may be
        // indirect as long as the PDF is not encrypted. Handle that
        // case.
        if (idObj->IsReference()) {
            idObj = m_vecObjects->GetObject(idObj->GetReference());
        }

        auto it = idObj->GetArray().begin();
        PdfString str;
        if (it != idObj->GetArray().end() && it->TryGetString(str) && str.IsHex())
        {
            *pOriginalIdentifier = it->GetString();
            bOriginalIdentifierFound = true;
        }
    }

    // create a dictionary with some unique information.
    // This dictionary is based on the PDF files information
    // dictionary if it exists.
    auto pObjInfo = pTrailer.GetDictionary().GetKey("Info");;
    if (pObjInfo == nullptr)
    {
        PdfDate date;
        PdfString dateString = date.ToString();

        pInfo.reset(new PdfObject());
        pInfo->GetDictionary().AddKey("CreationDate", dateString);
        pInfo->GetDictionary().AddKey("Creator", PdfString("PoDoFo"));
        pInfo->GetDictionary().AddKey("Producer", PdfString("PoDoFo"));
    }
    else
    {
        PdfReference ref;
        if(pObjInfo->TryGetReference(ref))
        {
            pObjInfo = m_vecObjects->GetObject(ref);

            if (pObjInfo == nullptr)
            {
                std::ostringstream oss;
                oss << "Error while retrieving info dictionary: "
                    << ref.ObjectNumber() << " "
                    << ref.GenerationNumber() << " R" << std::endl;
                PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, oss.str().c_str());
            }
            else
            {
                pInfo.reset(new PdfObject(*pObjInfo));
            }
        }
        else if (pObjInfo->IsDictionary())
        {
            // NOTE: While Table 15, ISO 32000-1:2008, states that Info should be an
            // indirect reference, we found Pdfs, for example produced
            // by pdfjs v0.4.1 (github.com/rkusa/pdfjs) that do otherwise.
            // As usual, Acroat Pro Syntax checker doesn't care about this,
            // so let's just read it
            pInfo.reset(new PdfObject(*pObjInfo));
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Invalid ");
        }
    }

    pInfo->GetDictionary().AddKey("Location", PdfString("SOMEFILENAME"));
    pInfo->Write(length, m_eWriteMode, nullptr);

    PdfRefCountedBuffer buffer(length.GetLength());
    PdfOutputDevice device(buffer);
    pInfo->Write(device, m_eWriteMode, nullptr);

    // calculate the MD5 Sum
    identifier = PdfEncryptMD5Base::GetMD5String(reinterpret_cast<unsigned char*>(buffer.GetBuffer()),
        static_cast<unsigned>(length.GetLength()));

    if (pOriginalIdentifier && !bOriginalIdentifierFound)
        *pOriginalIdentifier = identifier;
}

void PdfWriter::SetEncryptObj(PdfObject* obj)
{
    m_pEncryptObj = obj;
}

void PdfWriter::SetEncrypted( const PdfEncrypt & rEncrypt )
{
	m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( rEncrypt );
}

void PdfWriter::SetUseXRefStream(bool bStream)
{
    if (bStream && m_eVersion < PdfVersion::V1_5)
        this->SetPdfVersion(PdfVersion::V1_5);

    m_UseXRefStream = bStream;
}
