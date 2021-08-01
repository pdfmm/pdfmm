/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfDocument.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <sstream>

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfObject.h"
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
#include "PdfXObject.h"


using namespace std;
using namespace PoDoFo;

PdfDocument::PdfDocument(bool bEmpty) :
    m_vecObjects(*this),
    m_Catalog(nullptr),
    m_fontCache(*this)
{
    if (!bEmpty)
    {
        m_Trailer.reset(new PdfObject()); // The trailer is NO part of the vector of objects
        m_Trailer->SetDocument(*this);
        m_Catalog = m_vecObjects.CreateDictionaryObject("Catalog");

        m_Info.reset(new PdfInfo(*this));

        m_Trailer->GetDictionary().AddKey("Root", m_Catalog->GetIndirectReference());
        m_Trailer->GetDictionary().AddKey("Info", m_Info->GetObject().GetIndirectReference());

        InitPagesTree();
    }
}

PdfDocument::~PdfDocument()
{
    this->Clear();
}

void PdfDocument::Clear() 
{
    m_fontCache.EmptyCache();
    m_vecObjects.Clear();
    m_Catalog = nullptr;
}

void PdfDocument::InitPagesTree()
{
    auto pagesRootObj = m_Catalog->GetDictionary().FindKey("Pages");
    if (pagesRootObj == nullptr)
    {
        m_PageTree.reset(new PdfPagesTree(*this));
        m_Catalog->GetDictionary().AddKey("Pages", m_PageTree->GetObject().GetIndirectReference());
    }
    else
    {
        m_PageTree.reset(new PdfPagesTree(*pagesRootObj));
    }
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog(const string_view& name) const
{
    return m_Catalog->GetDictionary().FindKey(name);
}

void PdfDocument::EmbedSubsetFonts()
{
	m_fontCache.EmbedSubsetFonts();
}

const PdfDocument& PdfDocument::Append(const PdfDocument& doc, bool appendAll)
{
    unsigned difference = static_cast<unsigned>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());


    // Because GetNextObject uses m_nObjectCount instead 
    // of m_vecObjects.GetSize()+m_vecObjects.GetFreeObjects().size()+1
    // make sure the free objects are already present before appending to
    // prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    for (auto& ref : doc.GetObjects().GetFreeObjects())
        m_vecObjects.AddFreeObject(PdfReference(ref.ObjectNumber() + difference, ref.GenerationNumber()));

    // append all objects first and fix their references
    for (auto& obj : doc.GetObjects())
    {
        PdfReference ref(static_cast<uint32_t>(obj->GetIndirectReference().ObjectNumber() + difference), obj->GetIndirectReference().GenerationNumber());
        auto newObj = new PdfObject(PdfDictionary(), true);
        newObj->SetIndirectReference(ref);
        m_vecObjects.AddObject(newObj);
        *newObj = *obj;

        PdfError::LogMessage(LogSeverity::Information, "Fixing references in %i %i R by %i",
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
            auto& obj = m_vecObjects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
                + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
            if (obj.IsDictionary() && obj.GetDictionary().HasKey("Parent"))
                obj.GetDictionary().RemoveKey("Parent");

            // Deal with inherited attributes
            const PdfName* pInherited = inheritableAttributes;
            while (pInherited->GetLength() != 0)
            {
                const PdfObject* pAttribute = page.GetInheritedKey(*pInherited);
                if (pAttribute != nullptr)
                {
                    PdfObject attribute(*pAttribute);
                    FixObjectReferences(attribute, difference);
                    obj.GetDictionary().AddKey(*pInherited, attribute);
                }

                pInherited++;
            }

            m_PageTree->InsertPage(m_PageTree->GetPageCount(), &obj);
        }

        // append all outlines
        PdfOutlineItem* pRoot = this->GetOutlines();
        PdfOutlines* pAppendRoot = const_cast<PdfDocument&>(doc).GetOutlines(PoDoFo::ePdfDontCreateObject);
        if (pAppendRoot && pAppendRoot->First())
        {
            // only append outlines if appended document has outlines
            while (pRoot && pRoot->Next())
                pRoot = pRoot->Next();

            PdfReference ref(pAppendRoot->First()->GetObject().GetIndirectReference().ObjectNumber()
                + difference, pAppendRoot->First()->GetObject().GetIndirectReference().GenerationNumber());
            pRoot->InsertChild(new PdfOutlines(m_vecObjects.MustGetObject(ref)));
        }
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

const PdfDocument& PdfDocument::InsertExistingPageAt(const PdfDocument& doc, unsigned pageIndex, unsigned atIndex)
{
    /* copy of PdfDocument::Append, only restricts which page to add */
    unsigned difference = static_cast<unsigned>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());


    // Ulrich Arnold 30.7.2009: Because GetNextObject uses m_nObjectCount instead 
    //                          of m_vecObjects.GetSize()+m_vecObjects.GetFreeObjects().size()+1
    //                          make sure the free objects are already present before appending to
    //                          prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    TCIPdfReferenceList itFree = doc.GetObjects().GetFreeObjects().begin();
    while (itFree != doc.GetObjects().GetFreeObjects().end())
    {
        m_vecObjects.AddFreeObject(PdfReference((*itFree).ObjectNumber() + difference, (*itFree).GenerationNumber()));

        itFree++;
    }

    // append all objects first and fix their references
    for (auto& obj : GetObjects())
    {
        PdfReference ref(static_cast<uint32_t>(obj->GetIndirectReference().ObjectNumber() + difference), obj->GetIndirectReference().GenerationNumber());
        auto newObj = new PdfObject(PdfDictionary(), true);
        newObj->SetIndirectReference(ref);
        m_vecObjects.AddObject(newObj);
        *newObj = *obj;

        PdfError::LogMessage(LogSeverity::Information, "Fixing references in %i %i R by %i",
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

        auto page = doc.GetPageTree().GetPage(i);
        auto& obj = m_vecObjects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
            + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
        if (obj.IsDictionary() && obj.GetDictionary().HasKey("Parent"))
            obj.GetDictionary().RemoveKey("Parent");

        // Deal with inherited attributes
        const PdfName* pInherited = inheritableAttributes;
        while (pInherited->GetLength() != 0)
        {
            const PdfObject* pAttribute = page.GetInheritedKey(*pInherited);
            if (pAttribute)
            {
                PdfObject attribute(*pAttribute);
                FixObjectReferences(attribute, difference);
                obj.GetDictionary().AddKey(*pInherited, attribute);
            }

            pInherited++;
        }

        m_PageTree->InsertPage(atIndex, &obj);
    }

    // append all outlines
    PdfOutlineItem* pRoot = this->GetOutlines();
    PdfOutlines* pAppendRoot = const_cast<PdfDocument&>(doc).GetOutlines(PoDoFo::ePdfDontCreateObject);
    if (pAppendRoot && pAppendRoot->First())
    {
        // only append outlines if appended document has outlines
        while (pRoot && pRoot->Next())
            pRoot = pRoot->Next();

        PdfReference ref(pAppendRoot->First()->GetObject().GetIndirectReference().ObjectNumber()
            + difference, pAppendRoot->First()->GetObject().GetIndirectReference().GenerationNumber());
        pRoot->InsertChild(new PdfOutlines(m_vecObjects.MustGetObject(ref)));
    }

    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

PdfRect PdfDocument::FillXObjectFromDocumentPage(PdfXObject& xobj, const PdfDocument& doc, unsigned pageIndex, bool useTrimBox)
{
    unsigned difference = static_cast<unsigned>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());
    Append(doc, false);
    auto& page = doc.GetPageTree().GetPage(pageIndex);
    return FillXObjectFromPage(xobj, page, useTrimBox, difference);
}

PdfRect PdfDocument::FillXObjectFromExistingPage(PdfXObject& xobj, unsigned pageIndex, bool useTrimBox)
{
    auto& pPage = m_PageTree->GetPage(pageIndex);
    return FillXObjectFromPage(xobj, pPage, useTrimBox, 0);
}

PdfRect PdfDocument::FillXObjectFromPage(PdfXObject& xobj, const PdfPage& page, bool useTrimBox, unsigned difference)
{
    // TODO: remove unused objects: page, ...

    auto& obj = m_vecObjects.MustGetObject(PdfReference(page.GetObject().GetIndirectReference().ObjectNumber()
        + difference, page.GetObject().GetIndirectReference().GenerationNumber()));
    PdfRect box = page.GetMediaBox();

    // intersect with crop-box
    box.Intersect(page.GetCropBox());

    // intersect with trim-box according to parameter
    if (useTrimBox)
        box.Intersect(page.GetTrimBox());

    // link resources from external doc to x-object
    if (obj.IsDictionary() && obj.GetDictionary().HasKey("Resources"))
        xobj.GetObject().GetDictionary().AddKey("Resources", *obj.GetDictionary().GetKey("Resources"));

    // copy top-level content from external doc to x-object
    if (obj.IsDictionary() && obj.GetDictionary().HasKey("Contents"))
    {
        // get direct pointer to contents
        PdfObject* pContents;
        if (obj.GetDictionary().GetKey("Contents")->IsReference())
            pContents = m_vecObjects.GetObject(obj.GetDictionary().GetKey("Contents")->GetReference());
        else
            pContents = obj.GetDictionary().GetKey("Contents");

        if (pContents->IsArray())
        {
            // copy array as one stream to xobject
            PdfArray pArray = pContents->GetArray();

            PdfStream& pObjStream = xobj.GetObject().GetOrCreateStream();

            TVecFilters vFilters;
            vFilters.push_back(PdfFilterType::FlateDecode);
            pObjStream.BeginAppend(vFilters);

            for (auto& child : pArray)
            {
                if (child.IsReference())
                {
                    // TODO: not very efficient !!
                    PdfObject* pObj = GetObjects().GetObject(child.GetReference());

                    while (pObj != nullptr)
                    {
                        if (pObj->IsReference())    // Recursively look for the stream
                        {
                            pObj = GetObjects().GetObject(pObj->GetReference());
                        }
                        else if (pObj->HasStream())
                        {
                            PdfStream& pcontStream = pObj->GetOrCreateStream();

                            unique_ptr<char> pcontStreamBuffer;
                            size_t pcontStreamLength;
                            pcontStream.GetFilteredCopy(pcontStreamBuffer, pcontStreamLength);
                            pObjStream.Append(pcontStreamBuffer.get(), pcontStreamLength);
                            break;
                        }
                        else
                        {
                            PODOFO_RAISE_ERROR(EPdfError::InvalidStream);
                            break;
                        }
                    }
                }
                else
                {
                    string str;
                    child.ToString(str);
                    pObjStream.Append(str);
                    pObjStream.Append(" ");
                }
            }
            pObjStream.EndAppend();
        }
        else if (pContents->HasStream())
        {
            // copy stream to xobject
            PdfStream& pObjStream = xobj.GetObject().GetOrCreateStream();
            PdfStream& pcontStream = pContents->GetOrCreateStream();

            TVecFilters vFilters;
            vFilters.push_back(PdfFilterType::FlateDecode);
            pObjStream.BeginAppend(vFilters);
            unique_ptr<char> pcontStreamBuffer;
            size_t pcontStreamLength;
            pcontStream.GetFilteredCopy(pcontStreamBuffer, pcontStreamLength);
            pObjStream.Append(pcontStreamBuffer.get(), pcontStreamLength);
            pObjStream.EndAppend();
        }
        else
        {
            PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
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
            PODOFO_RAISE_ERROR(EPdfError::InvalidName);
    }

    return thePageMode;
}

void PdfDocument::SetPageMode(PdfPageMode inMode)
{
    switch (inMode) {
        default:
        case PdfPageMode::DontCare:
            // m_pCatalog->RemoveKey("PageMode");
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
        PdfDictionary	vpDict;
        vpDict.AddKey(whichPref, valueObj);

        m_Catalog->GetDictionary().AddKey("ViewerPreferences", PdfObject(vpDict));
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

void PdfDocument::SetPrintScaling(const PdfName& inScalingType)
{
    SetViewerPreference("PrintScaling", inScalingType);
}

void PdfDocument::SetBaseURI(const string_view& inBaseURI)
{
    PdfDictionary uriDict;
    uriDict.AddKey("Base", PdfString(inBaseURI));
    m_Catalog->GetDictionary().AddKey("URI", PdfDictionary(uriDict));
}

void PdfDocument::SetLanguage(const string_view& inLanguage)
{
    m_Catalog->GetDictionary().AddKey("Lang", PdfString(inLanguage));
}

void PdfDocument::SetBindingDirection(const PdfName& inDirection)
{
    SetViewerPreference("Direction", inDirection);
}

void PdfDocument::SetPageLayout(PdfPageLayout inLayout)
{
    switch (inLayout)
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

PdfOutlines* PdfDocument::GetOutlines(bool create)
{
    PdfObject* obj;

    if (m_Outlines == nullptr)
    {
        obj = GetNamedObjectFromCatalog("Outlines");
        if (obj == nullptr)
        {
            if (!create)
                return nullptr;

            m_Outlines.reset(new PdfOutlines(*this));
            m_Catalog->GetDictionary().AddKey("Outlines", m_Outlines->GetObject().GetIndirectReference());
        }
        else if (obj->GetDataType() != EPdfDataType::Dictionary)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
        }
        else
            m_Outlines.reset(new PdfOutlines(*obj));
    }

    return m_Outlines.get();
}

PdfNamesTree* PdfDocument::GetNamesTree(bool create)
{
    PdfObject* pObj;
    if (m_NameTree == nullptr)
    {
        pObj = GetNamedObjectFromCatalog("Names");
        if (pObj == nullptr)
        {
            if (!create)
                return nullptr;

            PdfNamesTree tmpTree(*this);
            pObj = &tmpTree.GetObject();
            m_Catalog->GetDictionary().AddKey("Names", pObj->GetIndirectReference());
            m_NameTree.reset(new PdfNamesTree(*pObj, m_Catalog));
        }
        else if (pObj->GetDataType() != EPdfDataType::Dictionary)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
        }
        else
            m_NameTree.reset(new PdfNamesTree(*pObj, m_Catalog));
    }

    return m_NameTree.get();
}

PdfAcroForm* PdfDocument::GetAcroForm(bool create, EPdfAcroFormDefaulAppearance defaultAppearance)
{
    if (m_AcroForms == nullptr)
    {
        auto obj = GetNamedObjectFromCatalog("AcroForm");
        if (obj == nullptr)
        {
            if (!create)
                return nullptr;

            m_AcroForms.reset(new PdfAcroForm(*this, defaultAppearance));
            m_Catalog->GetDictionary().AddKey("AcroForm", m_AcroForms->GetObject().GetIndirectReference());
        }
        else if (obj->GetDataType() != EPdfDataType::Dictionary)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
        }
        else
        {
            m_AcroForms.reset(new PdfAcroForm(*obj, defaultAppearance));
        }
    }

    return m_AcroForms.get();
}

void PdfDocument::AddNamedDestination(const PdfDestination& dest, const PdfString& name)
{
    PdfNamesTree* nameTree = GetNamesTree();
    nameTree->AddValue("Dests", name, dest.GetObject()->GetIndirectReference());
}

void PdfDocument::AttachFile(const PdfFileSpec& rFileSpec)
{
    PdfNamesTree* pNames = this->GetNamesTree(true);

    if (pNames == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    pNames->AddValue("EmbeddedFiles", rFileSpec.GetFilename(false), rFileSpec.GetObject().GetIndirectReference());
}
    
PdfFileSpec* PdfDocument::GetAttachment(const PdfString& name)
{
    PdfNamesTree* pNames = this->GetNamesTree();

    if (pNames == nullptr)
        return nullptr;

    PdfObject* pObj = pNames->GetValue("EmbeddedFiles", name);

    if (pObj == nullptr)
        return nullptr;

    return new PdfFileSpec(*pObj);
}

void PdfDocument::SetInfo(std::unique_ptr<PdfInfo>& pInfo)
{
    if (pInfo == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_Info = std::move(pInfo);
}

void PdfDocument::SetTrailer(std::unique_ptr<PdfObject>& pObject)
{
    if (pObject == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_Trailer = std::move(pObject);
    m_Trailer->SetDocument(*this);
}

PdfObject& PdfDocument::GetCatalog()
{
    if (m_Catalog == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_Catalog;
}

const PdfObject& PdfDocument::GetCatalog() const
{
    if (m_Catalog == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_Catalog;
}

const PdfPagesTree& PdfDocument::GetPageTree() const
{
    if (m_PageTree == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_PageTree;
}

PdfPagesTree& PdfDocument::GetPageTree()
{
    if (m_PageTree == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_PageTree;
}


PdfObject& PdfDocument::GetTrailer()
{
    if (m_Trailer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_Trailer;
}

const PdfObject& PdfDocument::GetTrailer() const
{
    if (m_Trailer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_Trailer;
}

PdfInfo& PdfDocument::GetInfo()
{
    if (m_Info == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_Info;
}

const PdfInfo& PdfDocument::GetInfo() const
{
    if (m_Info == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_Info;
}
