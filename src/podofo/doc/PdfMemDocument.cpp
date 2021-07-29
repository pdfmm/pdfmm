/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfMemDocument.h"

#include <algorithm>
#include <deque>
#include <iostream>

#include "base/PdfParser.h"
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfObject.h"
#include "base/PdfParserObject.h"
#include "base/PdfStream.h"
#include "base/PdfVecObjects.h"

#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNamesTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"

using namespace std;

using namespace PoDoFo;

PdfMemDocument::PdfMemDocument()
    : PdfDocument(), m_bSoureHasXRefStream(false), m_lPrevXRefOffset(-1)
{
    m_eVersion = PdfVersionDefault;
    m_eWriteMode = PdfWriteModeDefault;
    m_bLinearized = false;
    m_eSourceVersion = m_eVersion;
}

PdfMemDocument::PdfMemDocument(bool bOnlyTrailer)
    : PdfDocument(bOnlyTrailer), m_bSoureHasXRefStream(false), m_lPrevXRefOffset(-1)
{
    m_eVersion = PdfVersionDefault;
    m_eWriteMode = PdfWriteModeDefault;
    m_bLinearized = false;
    m_eSourceVersion = m_eVersion;
}

PdfMemDocument::PdfMemDocument(const string_view& filename)
    : PdfDocument(), m_bSoureHasXRefStream(false), m_lPrevXRefOffset(-1)
{
    this->Load(filename);
}

PdfMemDocument::~PdfMemDocument()
{
    this->Clear();
}

void PdfMemDocument::Clear()
{
    m_pEncrypt = nullptr;
    m_eWriteMode = PdfWriteModeDefault;

    m_bSoureHasXRefStream = false;
    m_lPrevXRefOffset = -1;

    GetObjects().SetCanReuseObjectNumbers(true);

    PdfDocument::Clear();
}

void PdfMemDocument::InitFromParser(PdfParser* pParser)
{
    m_eVersion = pParser->GetPdfVersion();
    m_bLinearized = pParser->IsLinearized();
    m_eSourceVersion = m_eVersion;
    m_bSoureHasXRefStream = pParser->HasXRefStream();
    m_lPrevXRefOffset = pParser->GetXRefOffset();

    auto pTrailer = std::make_unique<PdfObject>(pParser->GetTrailer());
    this->SetTrailer(pTrailer); // Set immediately as trailer
                                   // so that pTrailer has an owner
                                   // and GetIndirectKey will work

    if (PdfError::DebugEnabled())
    {
        PdfRefCountedBuffer buf;
        PdfOutputDevice debug(buf);
        GetTrailer().GetVariant().Write(debug, m_eWriteMode, nullptr);
        debug.Write("\n", 1);
        size_t siz = buf.GetSize();
        char* ptr = buf.GetBuffer();
        PdfError::LogMessage(LogSeverity::Information, "%.*s", siz, ptr);
    }

    auto catalog = GetTrailer().GetIndirectKey("Root");
    if (!catalog)
        PODOFO_RAISE_ERROR_INFO(EPdfError::NoObject, "Catalog object not found!");

    this->SetCatalog(catalog);

    auto pInfoObj = GetTrailer().GetIndirectKey("Info");
    unique_ptr<PdfInfo> info;
    if (pInfoObj == nullptr)
    {
        info.reset(new PdfInfo(*this));
        GetTrailer().GetDictionary().AddKey("Info", info->GetObject().GetIndirectReference());
    }
    else
    {
        info.reset(new PdfInfo(*pInfoObj));
    }

    this->SetInfo(info);

    if (pParser->IsEncrypted())
    {
        // All PdfParser instances have a pointer to a PdfEncrypt object.
        // So we have to take ownership of it (command the parser to give it).
        m_pEncrypt = pParser->TakeEncrypt();
    }

    InitPagesTree();
}

void PdfMemDocument::Load(const string_view& filename, const string_view& password)
{
    if (filename.length() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.ParseFile(filename.data(), true);
    InitFromParser(&parser);
}

void PdfMemDocument::LoadFromBuffer(const string_view& buffer, const string_view& password)
{
    if (buffer.length() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.ParseBuffer(buffer, true);
    InitFromParser(&parser);
}

void PdfMemDocument::LoadFromDevice(const PdfRefCountedInputDevice& rDevice, const string_view& password)
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.Parse(rDevice, true);
    InitFromParser(&parser);
}

void PdfMemDocument::AddPdfExtension(const char* ns, int64_t level)
{
    if (!this->HasPdfExtension(ns, level))
    {

        PdfObject* pExtensions = this->GetCatalog().GetIndirectKey("Extensions");
        PdfDictionary newExtension;

        newExtension.AddKey("BaseVersion", PdfName(s_szPdfVersionNums[(unsigned)m_eVersion]));
        newExtension.AddKey("ExtensionLevel", PdfVariant(level));

        if (pExtensions && pExtensions->IsDictionary())
        {
            pExtensions->GetDictionary().AddKey(ns, newExtension);
        }
        else
        {
            PdfDictionary extensions;
            extensions.AddKey(ns, newExtension);
            this->GetCatalog().GetDictionary().AddKey("Extensions", extensions);
        }
    }
}

bool PdfMemDocument::HasPdfExtension(const char* ns, int64_t level) const {

    PdfObject* pExtensions = this->GetCatalog().GetIndirectKey("Extensions");

    if (pExtensions != nullptr)
    {
        PdfObject* pExtension = pExtensions->GetIndirectKey(ns);

        if (pExtension != nullptr)
        {
            PdfObject* pLevel = pExtension->GetIndirectKey("ExtensionLevel");

            if (pLevel != nullptr && pLevel->IsNumber() && pLevel->GetNumber() == level)
                return true;
        }
    }

    return false;
}

/** Return the list of all vendor-specific extensions to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
vector<PdfExtension> PdfMemDocument::GetPdfExtensions() const
{
    vector<PdfExtension> ret;
    PdfObject* extensions = this->GetCatalog().GetIndirectKey("Extensions");
    if (extensions == nullptr)
        return ret;

    // Loop through all declared extensions
    for (auto& pair : extensions->GetDictionary())
    {
        PdfObject* bv = pair.second.GetIndirectKey("BaseVersion");
        PdfObject* el = pair.second.GetIndirectKey("ExtensionLevel");

        if (bv && el && bv->IsName() && el->IsNumber()) {

            // Convert BaseVersion name to EPdfVersion
            for (unsigned i = 0; i <= MAX_PDF_VERSION_STRING_INDEX; i++)
            {
                if (bv->GetName().GetString() == s_szPdfVersionNums[i])
                    ret.push_back(PdfExtension(pair.first.GetString().c_str(), static_cast<PdfVersion>(i), el->GetNumber()));
            }
        }
    }
    return ret;
}
    

    
/** Remove a vendor-specific extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
void PdfMemDocument::RemovePdfExtension(const char* ns, int64_t level)
{
    if (this->HasPdfExtension(ns, level))
        this->GetCatalog().GetIndirectKey("Extensions")->GetDictionary().RemoveKey("ns");
}

void PdfMemDocument::Write(const string_view& filename, PdfSaveOptions options)
{
    PdfOutputDevice device(filename);
    this->Write(device, options);
}

void PdfMemDocument::Write(PdfOutputDevice& device, PdfSaveOptions options)
{
    // makes sure pending subset-fonts are embedded
    GetFontCache().EmbedSubsetFonts();

    PdfWriter writer(this->GetObjects(), this->GetTrailer());
    writer.SetPdfVersion(this->GetPdfVersion());
    writer.SetSaveOptions(options);
    writer.SetWriteMode(m_eWriteMode);

    if (m_pEncrypt)
        writer.SetEncrypted(*m_pEncrypt);

    writer.Write(device);
}

void PdfMemDocument::WriteUpdate(const string_view& filename, PdfSaveOptions options)
{
    PdfOutputDevice device(filename, false);
    this->WriteUpdate(device, options);
}

void PdfMemDocument::WriteUpdate(PdfOutputDevice& device, PdfSaveOptions options)
{
    // makes sure pending subset-fonts are embedded
    GetFontCache().EmbedSubsetFonts();
    PdfWriter writer(this->GetObjects(), this->GetTrailer());
    writer.SetSaveOptions(options);
    writer.SetPdfVersion(this->GetPdfVersion());
    writer.SetWriteMode(m_eWriteMode);
    writer.SetPrevXRefOffset(m_lPrevXRefOffset);
    writer.SetUseXRefStream(m_bSoureHasXRefStream);
    writer.SetIncrementalUpdate(m_bLinearized);

    if (m_pEncrypt != nullptr)
        writer.SetEncrypted(*m_pEncrypt);

    PdfObject* catalog;
    if (m_eSourceVersion < this->GetPdfVersion() && (catalog = this->getCatalog()) && catalog->IsDictionary())
    {
        if (this->GetPdfVersion() < PdfVersion::V1_0 || this->GetPdfVersion() > PdfVersion::V1_7)
            PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);

        catalog->GetDictionary().AddKey("Version", PdfName(s_szPdfVersionNums[(unsigned)this->GetPdfVersion()]));
    }

    try
    {
        writer.Write(device);
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__);
        throw e;
    }
}

PdfObject* PdfMemDocument::GetNamedObjectFromCatalog(const char* pszName) const
{
    return this->GetCatalog().GetIndirectKey(pszName);
}

void PdfMemDocument::DeletePages(unsigned atIndex, unsigned pageCount)
{
    for (unsigned i = 0; i < pageCount; i++)
        this->GetPageTree().DeletePage(atIndex);
}

const PdfMemDocument& PdfMemDocument::InsertPages(const PdfMemDocument& doc, unsigned atIndex, unsigned pageCount)
{
    /*
      This function works a bit different than one might expect.
      Rather than copying one page at a time - we copy the ENTIRE document
      and then delete the pages we aren't interested in.

      We do this because
      1) SIGNIFICANTLY simplifies the process
      2) Guarantees that shared objects aren't copied multiple times
      3) offers MUCH faster performance for the common cases

      HOWEVER: because PoDoFo doesn't currently do any sort of "object garbage collection" during
      a Write() - we will end up with larger documents, since the data from unused pages
      will also be in there.
    */

    // calculate preliminary "left" and "right" page ranges to delete
    // then offset them based on where the pages were inserted
    // NOTE: some of this will change if/when we support insertion at locations
    //       OTHER than the end of the document!
    unsigned leftStartPage = 0;
    unsigned leftCount = atIndex;
    unsigned rightStartPage = atIndex + pageCount;
    unsigned rightCount = doc.GetPageTree().GetPageCount() - rightStartPage;
    unsigned pageOffset = this->GetPageTree().GetPageCount();

    leftStartPage += pageOffset;
    rightStartPage += pageOffset;

    // append in the whole document
    this->Append(doc);

    // delete
    if (rightCount > 0)
        this->DeletePages(rightStartPage, rightCount);
    if (leftCount > 0)
        this->DeletePages(leftStartPage, leftCount);

    return *this;
}

void PdfMemDocument::SetEncrypted(const string& userPassword, const string& ownerPassword,
    EPdfPermissions protection, EPdfEncryptAlgorithm eAlgorithm,
    EPdfKeyLength eKeyLength)
{
    m_pEncrypt = PdfEncrypt::CreatePdfEncrypt(userPassword, ownerPassword, protection, eAlgorithm, eKeyLength);
}

void PdfMemDocument::SetEncrypted(const PdfEncrypt& pEncrypt)
{
    m_pEncrypt = PdfEncrypt::CreatePdfEncrypt(pEncrypt);
}

PdfObject* PdfMemDocument::GetStructTreeRoot() const
{
    return GetNamedObjectFromCatalog("StructTreeRoot");
}

PdfObject* PdfMemDocument::GetMetadata() const
{
    return GetNamedObjectFromCatalog("Metadata");
}

PdfObject* PdfMemDocument::GetMarkInfo() const
{
    return GetNamedObjectFromCatalog("MarkInfo");
}

PdfObject* PdfMemDocument::GetLanguage() const
{
    return GetNamedObjectFromCatalog("Lang");
}

void PdfMemDocument::FreeObjectMemory(const PdfReference& ref, bool force)
{
    FreeObjectMemory(this->GetObjects().GetObject(ref), force);
}

void PdfMemDocument::FreeObjectMemory(PdfObject* obj, bool force)
{
    if (obj == nullptr)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    PdfParserObject* pParserObject = dynamic_cast<PdfParserObject*>(obj);
    if (pParserObject == nullptr)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle,
            "FreeObjectMemory works only on classes of type PdfParserObject.");
    }

    pParserObject->FreeObjectMemory(force);
}

bool PdfMemDocument::IsPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsPrintAllowed() : true;
}

bool PdfMemDocument::IsEditAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditAllowed() : true;
}

bool PdfMemDocument::IsCopyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsCopyAllowed() : true;
}

bool PdfMemDocument::IsEditNotesAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditNotesAllowed() : true;
}

bool PdfMemDocument::IsFillAndSignAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsFillAndSignAllowed() : true;
}

bool PdfMemDocument::IsAccessibilityAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsAccessibilityAllowed() : true;
}

bool PdfMemDocument::IsDocAssemblyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsDocAssemblyAllowed() : true;
}

bool PdfMemDocument::IsHighPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsHighPrintAllowed() : true;
}
