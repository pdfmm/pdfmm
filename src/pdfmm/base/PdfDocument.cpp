/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include <pdfmm/private/XMPUtils.h>
#include "PdfDocument.h"

#include <algorithm>
#include <deque>
#include <sstream>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfImmediateWriter.h"
#include "PdfObject.h"
#include "PdfObjectStream.h"
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
#include "PdfXObject.h"


using namespace std;
using namespace mm;

PdfDocument::PdfDocument(bool empty) :
    m_Objects(*this),
    m_Catalog(nullptr),
    m_FontManager(*this)
{
    if (!empty)
    {
        m_Trailer.reset(new PdfObject()); // The trailer is NO part of the vector of objects
        m_Trailer->SetDocument(this);
        m_Catalog = m_Objects.CreateDictionaryObject("Catalog");

        m_Info.reset(new PdfInfo(*this));

        m_Trailer->GetDictionary().AddKeyIndirect("Root", m_Catalog);
        m_Trailer->GetDictionary().AddKeyIndirect("Info", &m_Info->GetObject());

        Init();
    }
}

PdfDocument::PdfDocument(const PdfDocument& doc) :
    m_Objects(*this, doc.m_Objects),
    m_Catalog(nullptr),
    m_FontManager(*this)
{
    SetTrailer(std::make_unique<PdfObject>(doc.GetTrailer()));
    Init();
}

PdfDocument::~PdfDocument()
{
    this->Clear();
}

void PdfDocument::Clear() 
{
    m_FontManager.Clear();
    m_Objects.Clear();
    m_Objects.SetCanReuseObjectNumbers(true);
    m_Catalog = nullptr;
}

void PdfDocument::Init()
{
    auto pagesRootObj = m_Catalog->GetDictionary().FindKey("Pages");
    if (pagesRootObj == nullptr)
    {
        m_PageTree.reset(new PdfPageTree(*this));
        m_Catalog->GetDictionary().AddKey("Pages", m_PageTree->GetObject().GetIndirectReference());
    }
    else
    {
        m_PageTree.reset(new PdfPageTree(*pagesRootObj));
    }

    auto& catalogDict = m_Catalog->GetDictionary();
    auto namesObj = catalogDict.FindKey("Names");
    if (namesObj != nullptr)
        m_NameTree.reset(new PdfNameTree(*namesObj));

    auto outlinesObj = catalogDict.FindKey("Outlines");
    if (outlinesObj != nullptr)
        m_Outlines.reset(new PdfOutlines(*outlinesObj));

    auto acroformObj = catalogDict.FindKey("AcroForm");
    if (acroformObj != nullptr)
        m_AcroForm.reset(new PdfAcroForm(*acroformObj));
}

const PdfDocument& PdfDocument::Append(const PdfDocument& doc, bool appendAll)
{
    unsigned difference = static_cast<unsigned>(m_Objects.GetSize() + m_Objects.GetFreeObjects().size());


    // Because GetNextObject uses m_ObjectCount instead 
    // of m_Objects.GetSize() + m_Objects.GetFreeObjects().size() + 1
    // make sure the free objects are already present before appending to
    // prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    for (auto& ref : doc.GetObjects().GetFreeObjects())
        m_Objects.AddFreeObject(PdfReference(ref.ObjectNumber() + difference, ref.GenerationNumber()));

    // append all objects first and fix their references
    for (auto& obj : doc.GetObjects())
    {
        PdfReference ref(static_cast<uint32_t>(obj->GetIndirectReference().ObjectNumber() + difference), obj->GetIndirectReference().GenerationNumber());
        auto newObj = new PdfObject(PdfDictionary());
        newObj->setDirty();
        newObj->SetIndirectReference(ref);
        m_Objects.PushObject(newObj);
        *newObj = *obj;

        PdfError::LogMessage(PdfLogSeverity::Information, "Fixing references in {} {} R by {}",
            newObj->GetIndirectReference().ObjectNumber(), newObj->GetIndirectReference().GenerationNumber(), difference);
        FixObjectReferences(*newObj, difference);
    }

    if (appendAll)
    {
        const PdfName inheritableAttributes[] = {
            PdfName("Resources"),
            PdfName("MediaBox"),
            PdfName("CropBox"),
            PdfName("Rotate"),
            PdfName::KeyNull
        };

        // append all pages now to our page tree
        for (unsigned i = 0; i < doc.GetPageTree().GetPageCount(); i++)
        {
            auto& page = doc.GetPageTree().GetPage(i);
            auto& obj = m_Objects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
                + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
            if (obj.IsDictionary() && obj.GetDictionary().HasKey("Parent"))
                obj.GetDictionary().RemoveKey("Parent");

            // Deal with inherited attributes
            auto inherited = inheritableAttributes;
            while (!inherited->IsNull())
            {
                auto attribute = page.GetInheritedKey(*inherited);
                if (attribute != nullptr)
                {
                    PdfObject attributeCopy(*attribute);
                    FixObjectReferences(attributeCopy, difference);
                    obj.GetDictionary().AddKey(*inherited, attributeCopy);
                }

                inherited++;
            }

            m_PageTree->InsertPage(m_PageTree->GetPageCount(), &obj);
        }

        // append all outlines
        PdfOutlineItem* root = this->GetOutlines();
        PdfOutlines* appendRoot = const_cast<PdfDocument&>(doc).GetOutlines();
        if (appendRoot && appendRoot->First())
        {
            // only append outlines if appended document has outlines
            while (root && root->Next())
                root = root->Next();

            PdfReference ref(appendRoot->First()->GetObject().GetIndirectReference().ObjectNumber()
                + difference, appendRoot->First()->GetObject().GetIndirectReference().GenerationNumber());
            root->InsertChild(new PdfOutlines(m_Objects.MustGetObject(ref)));
        }
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

const PdfDocument& PdfDocument::InsertExistingPageAt(const PdfDocument& doc, unsigned pageIndex, unsigned atIndex)
{
    // copy of PdfDocument::Append, only restricts which page to add
    unsigned difference = static_cast<unsigned>(m_Objects.GetSize() + m_Objects.GetFreeObjects().size());


    // Because GetNextObject uses m_ObjectCount instead 
    // of m_Objects.GetSize() + m_Objects.GetFreeObjects().size() + 1
    // make sure the free objects are already present before appending to
    // prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    for (auto& freeObj : GetObjects().GetFreeObjects())
    {
        m_Objects.AddFreeObject(PdfReference(freeObj.ObjectNumber() + difference, freeObj.GenerationNumber()));
    }

    // append all objects first and fix their references
    for (auto& obj : GetObjects())
    {
        PdfReference ref(static_cast<uint32_t>(obj->GetIndirectReference().ObjectNumber() + difference), obj->GetIndirectReference().GenerationNumber());
        auto newObj = new PdfObject(PdfDictionary());
        newObj->setDirty();
        newObj->SetIndirectReference(ref);
        m_Objects.PushObject(newObj);
        *newObj = *obj;

        PdfError::LogMessage(PdfLogSeverity::Information, "Fixing references in {} {} R by {}",
            newObj->GetIndirectReference().ObjectNumber(), newObj->GetIndirectReference().GenerationNumber(), difference);
        FixObjectReferences(*newObj, difference);
    }

    const PdfName inheritableAttributes[] = {
        PdfName("Resources"),
        PdfName("MediaBox"),
        PdfName("CropBox"),
        PdfName("Rotate"),
        PdfName::KeyNull
    };

    // append all pages now to our page tree
    for (unsigned i = 0; i < doc.GetPageTree().GetPageCount(); i++)
    {
        if (i != pageIndex)
            continue;

        auto& page = doc.GetPageTree().GetPage(i);
        auto& obj = m_Objects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
            + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
        if (obj.IsDictionary() && obj.GetDictionary().HasKey("Parent"))
            obj.GetDictionary().RemoveKey("Parent");

        // Deal with inherited attributes
        const PdfName* inherited = inheritableAttributes;
        while (!inherited->IsNull())
        {
            auto attribute = page.GetInheritedKey(*inherited);
            if (attribute != nullptr)
            {
                PdfObject attributeCopy(*attribute);
                FixObjectReferences(attributeCopy, difference);
                obj.GetDictionary().AddKey(*inherited, attributeCopy);
            }

            inherited++;
        }

        m_PageTree->InsertPage(atIndex, &obj);
    }

    // append all outlines
    PdfOutlineItem* root = this->GetOutlines();
    PdfOutlines* appendRoot = const_cast<PdfDocument&>(doc).GetOutlines();
    if (appendRoot != nullptr && appendRoot->First())
    {
        // only append outlines if appended document has outlines
        while (root != nullptr && root->Next())
            root = root->Next();

        PdfReference ref(appendRoot->First()->GetObject().GetIndirectReference().ObjectNumber()
            + difference, appendRoot->First()->GetObject().GetIndirectReference().GenerationNumber());
        root->InsertChild(new PdfOutlines(m_Objects.MustGetObject(ref)));
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

PdfRect PdfDocument::FillXObjectFromDocumentPage(PdfXObject& xobj, const PdfDocument& doc, unsigned pageIndex, bool useTrimBox)
{
    unsigned difference = static_cast<unsigned>(m_Objects.GetSize() + m_Objects.GetFreeObjects().size());
    Append(doc, false);
    auto& page = doc.GetPageTree().GetPage(pageIndex);
    return FillXObjectFromPage(xobj, page, useTrimBox, difference);
}

PdfRect PdfDocument::FillXObjectFromExistingPage(PdfXObject& xobj, unsigned pageIndex, bool useTrimBox)
{
    auto& page = m_PageTree->GetPage(pageIndex);
    return FillXObjectFromPage(xobj, page, useTrimBox, 0);
}

PdfRect PdfDocument::FillXObjectFromPage(PdfXObject& xobj, const PdfPage& page, bool useTrimBox, unsigned difference)
{
    // TODO: remove unused objects: page, ...

    auto& pageObj = m_Objects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
        + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
    PdfRect box = page.GetMediaBox();

    // intersect with crop-box
    box.Intersect(page.GetCropBox());

    // intersect with trim-box according to parameter
    if (useTrimBox)
        box.Intersect(page.GetTrimBox());

    // link resources from external doc to x-object
    if (pageObj.IsDictionary() && pageObj.GetDictionary().HasKey("Resources"))
        xobj.GetObject().GetDictionary().AddKey("Resources", *pageObj.GetDictionary().GetKey("Resources"));

    // copy top-level content from external doc to x-object
    if (pageObj.IsDictionary() && pageObj.GetDictionary().HasKey("Contents"))
    {
        // get direct pointer to contents
        auto& contents = pageObj.GetDictionary().MustFindKey("Contents");
        if (contents.IsArray())
        {
            // copy array as one stream to xobject
            PdfArray arr = contents.GetArray();

            PdfObjectStream& xobjStream = xobj.GetObject().GetOrCreateStream();

            PdfFilterList filters;
            filters.push_back(PdfFilterType::FlateDecode);
            xobjStream.BeginAppend(filters);

            for (auto& child : arr)
            {
                if (child.IsReference())
                {
                    // TODO: not very efficient !!
                    auto obj = GetObjects().GetObject(child.GetReference());

                    while (obj != nullptr)
                    {
                        if (obj->IsReference())    // Recursively look for the stream
                        {
                            obj = GetObjects().GetObject(obj->GetReference());
                        }
                        else if (obj->HasStream())
                        {
                            PdfObjectStream& contStream = obj->GetOrCreateStream();

                            unique_ptr<char[]> contStreamBuffer;
                            size_t contStreamLength;
                            contStream.GetFilteredCopy(contStreamBuffer, contStreamLength);
                            xobjStream.Append(contStreamBuffer.get(), contStreamLength);
                            break;
                        }
                        else
                        {
                            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidStream);
                            break;
                        }
                    }
                }
                else
                {
                    string str;
                    child.ToString(str);
                    xobjStream.Append(str);
                    xobjStream.Append(" ");
                }
            }
            xobjStream.EndAppend();
        }
        else if (contents.HasStream())
        {
            // copy stream to xobject
            PdfObjectStream& xobjStream = xobj.GetObject().GetOrCreateStream();
            PdfObjectStream& contentsStream = contents.GetOrCreateStream();

            PdfFilterList filters;
            filters.push_back(PdfFilterType::FlateDecode);
            xobjStream.BeginAppend(filters);
            unique_ptr<char[]> contStreamBuffer;
            size_t contStreamLength;
            contentsStream.GetFilteredCopy(contStreamBuffer, contStreamLength);
            xobjStream.Append(contStreamBuffer.get(), contStreamLength);
            xobjStream.EndAppend();
        }
        else
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
    }

    return box;
}

void PdfDocument::FixObjectReferences(PdfObject& obj, int difference)
{
    if (obj.IsDictionary())
    {
        for (auto& pair : obj.GetDictionary())
        {
            if (pair.second.IsReference())
            {
                pair.second = PdfObject(PdfReference(pair.second.GetReference().ObjectNumber() + difference,
                    pair.second.GetReference().GenerationNumber()));
            }
            else if (pair.second.IsDictionary() ||
                pair.second.IsArray())
            {
                FixObjectReferences(pair.second, difference);
            }
        }
    }
    else if (obj.IsArray())
    {
        for (auto& child : obj.GetArray())
        {
            if (child.IsReference())
            {
                child = PdfObject(PdfReference(child.GetReference().ObjectNumber() + difference,
                    child.GetReference().GenerationNumber()));
            }
            else if (child.IsDictionary() || child.IsArray())
            {
                FixObjectReferences(child, difference);
            }
        }
    }
    else if (obj.IsReference())
    {
        obj = PdfObject(PdfReference(obj.GetReference().ObjectNumber() + difference,
            obj.GetReference().GenerationNumber()));
    }
}

PdfPageMode PdfDocument::GetPageMode() const
{
    // PageMode is optional; the default value is UseNone
    PdfPageMode thePageMode = PdfPageMode::UseNone;

    PdfObject* pageModeObj = m_Catalog->GetDictionary().FindKey("PageMode");
    if (pageModeObj != nullptr)
    {
        PdfName pmName = pageModeObj->GetName();
        if (pmName == "UseNone")
            thePageMode = PdfPageMode::UseNone;
        else if (pmName == "UseThumbs")
            thePageMode = PdfPageMode::UseThumbs;
        else if (pmName == "UseOutlines")
            thePageMode = PdfPageMode::UseBookmarks;
        else if (pmName == "FullScreen")
            thePageMode = PdfPageMode::FullScreen;
        else if (pmName == "UseOC")
            thePageMode = PdfPageMode::UseOC;
        else if (pmName == "UseAttachments")
            thePageMode = PdfPageMode::UseAttachments;
        else
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidName);
    }

    return thePageMode;
}

void PdfDocument::SetPageMode(PdfPageMode inMode)
{
    switch (inMode) {
        default:
        case PdfPageMode::DontCare:
            // this value means leave it alone!
            break;

        case PdfPageMode::UseNone:
            m_Catalog->GetDictionary().AddKey("PageMode", PdfName("UseNone"));
            break;

        case PdfPageMode::UseThumbs:
            m_Catalog->GetDictionary().AddKey("PageMode", PdfName("UseThumbs"));
            break;

        case PdfPageMode::UseBookmarks:
            m_Catalog->GetDictionary().AddKey("PageMode", PdfName("UseOutlines"));
            break;

        case PdfPageMode::FullScreen:
            m_Catalog->GetDictionary().AddKey("PageMode", PdfName("FullScreen"));
            break;

        case PdfPageMode::UseOC:
            m_Catalog->GetDictionary().AddKey("PageMode", PdfName("UseOC"));
            break;

        case PdfPageMode::UseAttachments:
            m_Catalog->GetDictionary().AddKey("PageMode", PdfName("UseAttachments"));
            break;
    }
}

void PdfDocument::SetUseFullScreen()
{
    // first, we get the current mode
    PdfPageMode	curMode = GetPageMode();

    // if current mode is anything but "don't care", we need to move that to non-full-screen
    if (curMode != PdfPageMode::DontCare)
        SetViewerPreference("NonFullScreenPageMode", PdfObject(m_Catalog->GetDictionary().MustFindKey("PageMode")));

    SetPageMode(PdfPageMode::FullScreen);
}

void PdfDocument::SetViewerPreference(const PdfName& whichPref, const PdfObject& valueObj)
{
    PdfObject* prefsObj = m_Catalog->GetDictionary().FindKey("ViewerPreferences");
    if (prefsObj == nullptr)
    {
        // make me a new one and add it
        PdfDictionary vpDict;
        vpDict.AddKey(whichPref, valueObj);
        m_Catalog->GetDictionary().AddKey("ViewerPreferences", std::move(vpDict));
    }
    else
    {
        // modify the existing one
        prefsObj->GetDictionary().AddKey(whichPref, valueObj);
    }
}

void PdfDocument::SetViewerPreference(const PdfName& whichPref, bool inValue)
{
    SetViewerPreference(whichPref, PdfObject(inValue));
}

void PdfDocument::SetHideToolbar()
{
    SetViewerPreference("HideToolbar", true);
}

void PdfDocument::SetHideMenubar()
{
    SetViewerPreference("HideMenubar", true);
}

void PdfDocument::SetHideWindowUI()
{
    SetViewerPreference("HideWindowUI", true);
}

void PdfDocument::SetFitWindow()
{
    SetViewerPreference("FitWindow", true);
}

void PdfDocument::SetCenterWindow()
{
    SetViewerPreference("CenterWindow", true);
}

void PdfDocument::SetDisplayDocTitle()
{
    SetViewerPreference("DisplayDocTitle", true);
}

void PdfDocument::SetPrintScaling(const PdfName& scalingType)
{
    SetViewerPreference("PrintScaling", scalingType);
}

void PdfDocument::SetBaseURI(const string_view& inBaseURI)
{
    PdfDictionary uriDict;
    uriDict.AddKey("Base", PdfString(inBaseURI));
    m_Catalog->GetDictionary().AddKey("URI", PdfDictionary(uriDict));
}

void PdfDocument::SetLanguage(const string_view& language)
{
    m_Catalog->GetDictionary().AddKey("Lang", PdfString(language));
}

void PdfDocument::SetBindingDirection(const PdfName& direction)
{
    SetViewerPreference("Direction", direction);
}

void PdfDocument::CollectGarbage()
{
    m_Objects.CollectGarbage();
}

void PdfDocument::SetPageLayout(PdfPageLayout layout)
{
    switch (layout)
    {
        default:
        case PdfPageLayout::Ignore:
            break;	// means do nothing
        case PdfPageLayout::Default:
            m_Catalog->GetDictionary().RemoveKey("PageLayout");
            break;
        case PdfPageLayout::SinglePage:
            m_Catalog->GetDictionary().AddKey("PageLayout", PdfName("SinglePage"));
            break;
        case PdfPageLayout::OneColumn:
            m_Catalog->GetDictionary().AddKey("PageLayout", PdfName("OneColumn"));
            break;
        case PdfPageLayout::TwoColumnLeft:
            m_Catalog->GetDictionary().AddKey("PageLayout", PdfName("TwoColumnLeft"));
            break;
        case PdfPageLayout::TwoColumnRight:
            m_Catalog->GetDictionary().AddKey("PageLayout", PdfName("TwoColumnRight"));
            break;
        case PdfPageLayout::TwoPageLeft:
            m_Catalog->GetDictionary().AddKey("PageLayout", PdfName("TwoPageLeft"));
            break;
        case PdfPageLayout::TwoPageRight:
            m_Catalog->GetDictionary().AddKey("PageLayout", PdfName("TwoPageRight"));
            break;
    }
}

PdfOutlines& PdfDocument::GetOrCreateOutlines()
{
    if (m_Outlines != nullptr)
        return *m_Outlines.get();

    m_Outlines.reset(new PdfOutlines(*this));
    m_Catalog->GetDictionary().AddKey("Outlines", m_Outlines->GetObject().GetIndirectReference());
    return *m_Outlines.get();
}

PdfNameTree& PdfDocument::GetOrCreateNameTree()
{
    if (m_NameTree != nullptr)
        return *m_NameTree;

    PdfNameTree tmpTree(*this);
    auto obj = &tmpTree.GetObject();
    m_Catalog->GetDictionary().AddKey("Names", obj->GetIndirectReference());
    m_NameTree.reset(new PdfNameTree(*obj));
    return *m_NameTree;
}

PdfAcroForm& PdfDocument::GetOrCreateAcroForm(PdfAcroFormDefaulAppearance defaultAppearance)
{
    if (m_AcroForm != nullptr)
        return *m_AcroForm.get();

    m_AcroForm.reset(new PdfAcroForm(*this, defaultAppearance));
    m_Catalog->GetDictionary().AddKey("AcroForm", m_AcroForm->GetObject().GetIndirectReference());
    return *m_AcroForm.get();
}

void PdfDocument::AddNamedDestination(const PdfDestination& dest, const PdfString& name)
{
    auto& names = GetOrCreateNameTree();
    names.AddValue("Dests", name, dest.GetObject().GetIndirectReference());
}

void PdfDocument::AttachFile(const PdfFileSpec& fileSpec)
{
    auto& names = GetOrCreateNameTree();
    names.AddValue("EmbeddedFiles", fileSpec.GetFilename(false), fileSpec.GetObject().GetIndirectReference());
}
    
PdfFileSpec* PdfDocument::GetAttachment(const PdfString& name)
{
    if (m_NameTree == nullptr)
        return nullptr;

    auto obj = m_NameTree->GetValue("EmbeddedFiles", name);
    if (obj == nullptr)
        return nullptr;

    return new PdfFileSpec(*obj);
}

void PdfDocument::SetTrailer(unique_ptr<PdfObject> obj)
{
    if (obj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_Trailer = std::move(obj);
    m_Trailer->SetDocument(this);

    m_Catalog = m_Trailer->GetDictionary().FindKey("Root");
    if (m_Catalog == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Catalog object not found!");

    auto infoObj = m_Trailer->GetDictionary().FindKey("Info");
    if (infoObj == nullptr)
    {
        m_Info.reset(new PdfInfo(*this));
        m_Trailer->GetDictionary().AddKey("Info", m_Info->GetObject().GetIndirectReference());
    }
    else
    {
        m_Info.reset(new PdfInfo(*infoObj));
    }
}

PdfObject& PdfDocument::GetCatalog()
{
    if (m_Catalog == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Catalog;
}

const PdfObject& PdfDocument::GetCatalog() const
{
    if (m_Catalog == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Catalog;
}

const PdfPageTree& PdfDocument::GetPageTree() const
{
    if (m_PageTree == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_PageTree;
}

PdfPageTree& PdfDocument::GetPageTree()
{
    if (m_PageTree == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_PageTree;
}

PdfObject& PdfDocument::GetTrailer()
{
    if (m_Trailer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Trailer;
}

const PdfObject& PdfDocument::GetTrailer() const
{
    if (m_Trailer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Trailer;
}

PdfInfo& PdfDocument::GetInfo()
{
    if (m_Info == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Info;
}

const PdfInfo& PdfDocument::GetInfo() const
{
    if (m_Info == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Info;
}

PdfObject* PdfDocument::GetStructTreeRoot()
{
    return GetCatalog().GetDictionary().FindKey("StructTreeRoot");
}

PdfObject* PdfDocument::GetMetadata()
{
    return GetCatalog().GetDictionary().FindKey("Metadata");
}

const PdfObject* PdfDocument::GetMetadata() const
{
    if (m_Catalog == nullptr)
        return nullptr;

    return m_Catalog->GetDictionary().FindKey("Metadata");
}

PdfObject& PdfDocument::GetOrCreateMetadata()
{
    auto& dict = GetCatalog().GetDictionary();
    auto metadata = dict.FindKey("Metadata");
    if (metadata != nullptr)
        return *metadata;

    metadata = m_Objects.CreateDictionaryObject("Metadata", "XML");
    dict.AddKeyIndirect("Metadata", metadata);
    return *metadata;
}

PdfObject* PdfDocument::GetMarkInfo()
{
    return GetCatalog().GetDictionary().FindKey("MarkInfo");
}

PdfObject* PdfDocument::GetLanguage()
{
    return GetCatalog().GetDictionary().FindKey("Lang");
}

PdfALevel PdfDocument::GetPdfALevel() const
{
    string value = GetMetadataStreamValue();
    if (value.length() == 0)
        return PdfALevel::Unknown;

    return mm::GetPdfALevel(value);
}

string PdfDocument::GetMetadataStreamValue() const
{
    string ret;
    auto obj = GetMetadata();
    if (obj == nullptr)
        return ret;

    auto stream = obj->GetStream();
    if (stream == nullptr)
        return ret;

    PdfStringOutputStream ouput(ret);
    stream->GetFilteredCopy(ouput);
    return ret;
}

void PdfDocument::SetMetadataStreamValue(const string_view& value)
{
    auto& obj = GetOrCreateMetadata();
    auto& stream = obj.GetOrCreateStream();
    PdfMemoryInputStream input(value);
    stream.SetRawData(input);

    // We are writing raw clear text, which is required in most
    // relevant scenarions (eg. PDF/A). Remove any possibly
    // existing filter
    obj.GetDictionary().RemoveKey(PdfName::KeyFilter);
}

void PdfDocument::updateModifyTimestamp(const PdfDate& modDate)
{
    // Set The /Info entry /ModDate
    GetInfo().SetModDate(modDate);

    string value = GetMetadataStreamValue();
    if (value.length() == 0)
        return;

    value = mm::UpdateXmpModDate(value, modDate);
    SetMetadataStreamValue(value);
}
