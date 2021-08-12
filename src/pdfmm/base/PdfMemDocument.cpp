/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfMemDocument.h"

#include <algorithm>
#include <deque>
#include <iostream>

#include "PdfParser.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfImmediateWriter.h"
#include "PdfObject.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfIndirectObjectList.h"
#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNameTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPageTree.h"

using namespace std;
using namespace mm;

PdfMemDocument::PdfMemDocument()
    : PdfDocument(), m_SoureHasXRefStream(false), m_PrevXRefOffset(-1)
{
    m_Version = PdfVersionDefault;
    m_WriteMode = PdfWriteModeDefault;
    m_Linearized = false;
    m_SourceVersion = m_Version;
}

PdfMemDocument::PdfMemDocument(bool onlyTrailer)
    : PdfDocument(onlyTrailer), m_SoureHasXRefStream(false), m_PrevXRefOffset(-1)
{
    m_Version = PdfVersionDefault;
    m_WriteMode = PdfWriteModeDefault;
    m_Linearized = false;
    m_SourceVersion = m_Version;
}

PdfMemDocument::PdfMemDocument(const string_view& filename)
    : PdfDocument(), m_SoureHasXRefStream(false), m_PrevXRefOffset(-1)
{
    this->Load(filename);
}

PdfMemDocument::~PdfMemDocument()
{
    this->Clear();
}

void PdfMemDocument::Clear()
{
    m_Encrypt = nullptr;
    m_WriteMode = PdfWriteModeDefault;

    m_SoureHasXRefStream = false;
    m_PrevXRefOffset = -1;

    GetObjects().SetCanReuseObjectNumbers(true);

    PdfDocument::Clear();
}

void PdfMemDocument::InitFromParser(PdfParser* parser)
{
    m_Version = parser->GetPdfVersion();
    m_Linearized = parser->IsLinearized();
    m_SourceVersion = m_Version;
    m_SoureHasXRefStream = parser->HasXRefStream();
    m_PrevXRefOffset = parser->GetXRefOffset();

    auto trailer = std::make_unique<PdfObject>(parser->GetTrailer());
    this->SetTrailer(trailer); // Set immediately as trailer
                                // so that trailer has an owner

    if (PdfError::DebugEnabled())
    {
        string buf;
        PdfStringOutputDevice debug(buf);
        GetTrailer().GetVariant().Write(debug, m_WriteMode, nullptr);
        debug.Write("\n", 1);
        PdfError::LogMessage(LogSeverity::Information, "%.*s", buf.size(), buf.data());
    }

    auto catalog = GetTrailer().GetDictionary().FindKey("Root");
    if (catalog == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Catalog object not found!");

    this->SetCatalog(catalog);

    auto infoObj = GetTrailer().GetDictionary().FindKey("Info");
    unique_ptr<PdfInfo> info;
    if (infoObj == nullptr)
    {
        info.reset(new PdfInfo(*this));
        GetTrailer().GetDictionary().AddKey("Info", info->GetObject().GetIndirectReference());
    }
    else
    {
        info.reset(new PdfInfo(*infoObj));
    }

    this->SetInfo(info);

    if (parser->IsEncrypted())
    {
        // All PdfParser instances have a pointer to a PdfEncrypt object.
        // So we have to take ownership of it (command the parser to give it).
        m_Encrypt = parser->TakeEncrypt();
    }

    InitPagesTree();
}

void PdfMemDocument::Load(const string_view& filename, const string_view& password)
{
    if (filename.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_Parser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.ParseFile(filename.data(), true);
    InitFromParser(&parser);
}

void PdfMemDocument::LoadFromBuffer(const string_view& buffer, const string_view& password)
{
    if (buffer.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_Parser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.ParseBuffer(buffer, true);
    InitFromParser(&parser);
}

void PdfMemDocument::LoadFromDevice(const std::shared_ptr<PdfInputDevice>& device, const string_view& password)
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_Parser is initialized for encrypted documents
    PdfParser parser(PdfDocument::GetObjects());
    parser.SetPassword(password);
    parser.Parse(device, true);
    InitFromParser(&parser);
}

void PdfMemDocument::AddPdfExtension(const PdfName& ns, int64_t level)
{
    if (!this->HasPdfExtension(ns, level))
    {

        PdfObject* extensions = this->GetCatalog().GetDictionary().FindKey("Extensions");
        PdfDictionary newExtension;

        newExtension.AddKey("BaseVersion", PdfName(s_PdfVersionNums[(unsigned)m_Version]));
        newExtension.AddKey("ExtensionLevel", PdfVariant(level));

        if (extensions != nullptr && extensions->IsDictionary())
        {
            extensions->GetDictionary().AddKey(ns, newExtension);
        }
        else
        {
            PdfDictionary extensions;
            extensions.AddKey(ns, newExtension);
            this->GetCatalog().GetDictionary().AddKey("Extensions", extensions);
        }
    }
}

bool PdfMemDocument::HasPdfExtension(const PdfName& ns, int64_t level) const {

    auto extensions = this->GetCatalog().GetDictionary().FindKey("Extensions");
    if (extensions != nullptr)
    {
        auto extension = extensions->GetDictionary().FindKey(ns);
        if (extension != nullptr)
        {
            auto levelObj = extension->GetDictionary().FindKey("ExtensionLevel");
            if (levelObj != nullptr && levelObj->IsNumber() && levelObj->GetNumber() == level)
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
    auto extensions = this->GetCatalog().GetDictionary().FindKey("Extensions");
    if (extensions == nullptr)
        return ret;

    // Loop through all declared extensions
    for (auto& pair : extensions->GetDictionary())
    {
        auto bv = pair.second.GetDictionary().FindKey("BaseVersion");
        auto el = pair.second.GetDictionary().FindKey("ExtensionLevel");

        if (bv != nullptr && el != nullptr && bv->IsName() && el->IsNumber())
        {
            // Convert BaseVersion name to PdfVersion
            for (unsigned i = 0; i <= MAX_PDF_VERSION_STRING_INDEX; i++)
            {
                if (bv->GetName().GetString() == s_PdfVersionNums[i])
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
void PdfMemDocument::RemovePdfExtension(const PdfName& ns, int64_t level)
{
    if (this->HasPdfExtension(ns, level))
        this->GetCatalog().GetDictionary().FindKey("Extensions")->GetDictionary().RemoveKey("ns");
}

void PdfMemDocument::Write(const string_view& filename, PdfSaveOptions options)
{
    PdfFileOutputDevice device(filename);
    this->Write(device, options);
}

void PdfMemDocument::Write(PdfOutputDevice& device, PdfSaveOptions options)
{
    // makes sure pending subset-fonts are embedded
    GetFontManager().EmbedSubsetFonts();

    PdfWriter writer(this->GetObjects(), this->GetTrailer());
    writer.SetPdfVersion(this->GetPdfVersion());
    writer.SetSaveOptions(options);
    writer.SetWriteMode(m_WriteMode);

    if (m_Encrypt != nullptr)
        writer.SetEncrypted(*m_Encrypt);

    writer.Write(device);
}

void PdfMemDocument::WriteUpdate(const string_view& filename, PdfSaveOptions options)
{
    PdfFileOutputDevice device(filename, false);
    this->WriteUpdate(device, options);
}

void PdfMemDocument::WriteUpdate(PdfOutputDevice& device, PdfSaveOptions options)
{
    // makes sure pending subset-fonts are embedded
    GetFontManager().EmbedSubsetFonts();
    PdfWriter writer(this->GetObjects(), this->GetTrailer());
    writer.SetSaveOptions(options);
    writer.SetPdfVersion(this->GetPdfVersion());
    writer.SetWriteMode(m_WriteMode);
    writer.SetPrevXRefOffset(m_PrevXRefOffset);
    writer.SetUseXRefStream(m_SoureHasXRefStream);
    writer.SetIncrementalUpdate(m_Linearized);

    if (m_Encrypt != nullptr)
        writer.SetEncrypted(*m_Encrypt);

    PdfObject* catalog;
    if (m_SourceVersion < this->GetPdfVersion() && (catalog = this->getCatalog()) && catalog->IsDictionary())
    {
        if (this->GetPdfVersion() < PdfVersion::V1_0 || this->GetPdfVersion() > PdfVersion::V1_7)
            PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        catalog->GetDictionary().AddKey("Version", PdfName(s_PdfVersionNums[(unsigned)this->GetPdfVersion()]));
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

PdfObject* PdfMemDocument::GetNamedObjectFromCatalog(const string_view& name) const
{
    return const_cast<PdfMemDocument&>(*this).GetCatalog().GetDictionary().FindKey(name);
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

      HOWEVER: because pdfmm doesn't currently do any sort of "object garbage collection" during
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
    PdfPermissions protection, PdfEncryptAlgorithm algorithm,
    PdfKeyLength keyLength)
{
    m_Encrypt = PdfEncrypt::CreatePdfEncrypt(userPassword, ownerPassword, protection, algorithm, keyLength);
}

void PdfMemDocument::SetEncrypted(const PdfEncrypt& encrypt)
{
    m_Encrypt = PdfEncrypt::CreatePdfEncrypt(encrypt);
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
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(obj);
    if (parserObject == nullptr)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle,
            "FreeObjectMemory works only on classes of type PdfParserObject.");
    }

    parserObject->FreeObjectMemory(force);
}

bool PdfMemDocument::IsPrintAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsPrintAllowed();
}

bool PdfMemDocument::IsEditAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsEditAllowed();
}

bool PdfMemDocument::IsCopyAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsCopyAllowed();
}

bool PdfMemDocument::IsEditNotesAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsEditNotesAllowed();
}

bool PdfMemDocument::IsFillAndSignAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsFillAndSignAllowed();
}

bool PdfMemDocument::IsAccessibilityAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsAccessibilityAllowed();
}

bool PdfMemDocument::IsDocAssemblyAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsDocAssemblyAllowed();
}

bool PdfMemDocument::IsHighPrintAllowed() const
{
    return m_Encrypt == nullptr ? true : m_Encrypt->IsHighPrintAllowed();
}
