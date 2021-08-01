/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfPage.h"

#include <sstream>

#include "base/PdfDictionary.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"
#include "base/PdfWriter.h"
#include "base/PdfStream.h"
#include "base/PdfColor.h"

#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

static int normalize(int value, int start, int end);

PdfPage::PdfPage(PdfDocument& parent, const PdfRect& size)
    : PdfElement(parent, "Page"), PdfCanvas(), m_contents(nullptr)
{
    InitNewPage(size);
}

PdfPage::PdfPage(PdfObject& obj, const deque<PdfObject*>& listOfParents)
    : PdfElement(obj), PdfCanvas(), m_contents(nullptr)
{
    m_Resources = obj.GetDictionary().FindKey("Resources");
    if (m_Resources == nullptr)
    {
        // Resources might be inherited
        for (auto& parent : listOfParents)
            m_Resources = parent->GetDictionary().FindKey("Resources");
    }

    PdfObject* contents = obj.GetDictionary().FindKey("Contents");
    if (contents != nullptr)
        m_contents = new PdfContents(*this, *contents);
}

PdfPage::~PdfPage()
{
    for (auto& pair : m_mapAnnotations)
        delete pair.second;

    delete m_contents;
}

PdfRect PdfPage::GetRect() const
{
    return this->GetMediaBox();
}

bool PdfPage::HasRotation(double &teta) const
{
    int rotationRaw = normalize(GetRotationRaw(), 0, 360);
    if (rotationRaw == 0)
    {
        teta = 0;
        return false;
    }

    // Convert to radians and make it a counterclockwise rotation,
    // as common mathematical notation for rotations
    teta = -rotationRaw * M_PI / 180;
    return true;
}

void PdfPage::InitNewPage(const PdfRect& rSize)
{
    SetMediaBox(rSize);

    // The PDF specification suggests that we send all available PDF Procedure sets
    this->GetObject().GetDictionary().AddKey("Resources", PdfDictionary());
    m_Resources = this->GetObject().GetDictionary().FindKey("Resources");
    m_Resources->GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}

void PdfPage::EnsureContentsCreated() const
{
    if (m_contents != nullptr)
        return;

    auto& page = const_cast<PdfPage&>(*this);
    page.m_contents = new PdfContents(page);
    page.GetObject().GetDictionary().AddKey(PdfName::KeyContents,
        m_contents->GetContents().GetIndirectReference());
}

void PdfPage::EnsureResourcesCreated() const
{
    if (m_Resources != nullptr)
        return;

    auto& page = const_cast<PdfPage&>(*this);
    page.GetObject().GetDictionary().AddKey("Resources", PdfDictionary());
    page.m_Resources = page.GetObject().GetDictionary().FindKey("Resources");
    m_Resources->GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}

PdfStream& PdfPage::GetStreamForAppending(EPdfStreamAppendFlags flags)
{
    EnsureContentsCreated();
    return m_contents->GetStreamForAppending(flags);
}

PdfRect PdfPage::CreateStandardPageSize(const PdfPageSize ePageSize, bool bLandscape)
{
    PdfRect rect;

    switch (ePageSize)
    {
        case PdfPageSize::A0:
            rect.SetWidth(2384.0);
            rect.SetHeight(3370.0);
            break;

        case PdfPageSize::A1:
            rect.SetWidth(1684.0);
            rect.SetHeight(2384.0);
            break;

        case PdfPageSize::A2:
            rect.SetWidth(1191.0);
            rect.SetHeight(1684.0);
            break;

        case PdfPageSize::A3:
            rect.SetWidth(842.0);
            rect.SetHeight(1190.0);
            break;

        case PdfPageSize::A4:
            rect.SetWidth(595.0);
            rect.SetHeight(842.0);
            break;

        case PdfPageSize::A5:
            rect.SetWidth(420.0);
            rect.SetHeight(595.0);
            break;

        case PdfPageSize::A6:
            rect.SetWidth(297.0);
            rect.SetHeight(420.0);
            break;

        case PdfPageSize::Letter:
            rect.SetWidth(612.0);
            rect.SetHeight(792.0);
            break;

        case PdfPageSize::Legal:
            rect.SetWidth(612.0);
            rect.SetHeight(1008.0);
            break;

        case PdfPageSize::Tabloid:
            rect.SetWidth(792.0);
            rect.SetHeight(1224.0);
            break;

        default:
            break;
    }

    if (bLandscape)
    {
        double dTmp = rect.GetWidth();
        rect.SetWidth(rect.GetHeight());
        rect.SetHeight(dTmp);
    }

    return rect;
}

const PdfObject* PdfPage::GetInheritedKeyFromObject(const string_view& inKey, const PdfObject& inObject, int depth) const
{
    const PdfObject* pObj = nullptr;

    // check for it in the object itself
    if (inObject.GetDictionary().HasKey(inKey))
    {
        pObj = inObject.GetDictionary().GetKey(inKey);
        if (!pObj->IsNull())
            return pObj;
    }

    // if we get here, we need to go check the parent - if there is one!
    if (inObject.GetDictionary().HasKey("Parent"))
    {
        // CVE-2017-5852 - prevent stack overflow if Parent chain contains a loop, or is very long
        // e.g. pObj->GetParent() == pObj or pObj->GetParent()->GetParent() == pObj
        // default stack sizes
        // Windows: 1 MB
        // Linux: 2 MB
        // macOS: 8 MB for main thread, 0.5 MB for secondary threads
        // 0.5 MB is enough space for 1000 512 byte stack frames and 2000 256 byte stack frames
        const int maxRecursionDepth = 1000;

        if (depth > maxRecursionDepth)
            PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);

        pObj = inObject.GetDictionary().FindKey("Parent");
        if (pObj == &inObject)
        {
            ostringstream oss;
            oss << "Object " << inObject.GetIndirectReference().ObjectNumber() << " "
                << inObject.GetIndirectReference().GenerationNumber() << " references itself as Parent";
            PODOFO_RAISE_ERROR_INFO(EPdfError::BrokenFile, oss.str().c_str());
        }

        if (pObj)
            pObj = GetInheritedKeyFromObject(inKey, *pObj, depth + 1);
    }

    return pObj;
}

PdfRect PdfPage::GetPageBox(const string_view& inBox) const
{
    PdfRect	pageBox;
    const PdfObject* pObj;

    // Take advantage of inherited values - walking up the tree if necessary
    pObj = GetInheritedKeyFromObject(inBox, this->GetObject());

    // Sometime page boxes are defined using reference objects
    while (pObj && pObj->IsReference())
    {
        pObj = this->GetObject().GetDocument()->GetObjects().GetObject(pObj->GetReference());
    }

    // assign the value of the box from the array
    if (pObj && pObj->IsArray())
    {
        pageBox.FromArray(pObj->GetArray());
    }
    else if (inBox == "ArtBox" ||
        inBox == "BleedBox" ||
        inBox == "TrimBox")
    {
        // If those page boxes are not specified then
        // default to CropBox per PDF Spec (3.6.2)
        pageBox = GetPageBox("CropBox");
    }
    else if (inBox == "CropBox")
    {
        // If crop box is not specified then
        // default to MediaBox per PDF Spec (3.6.2)
        pageBox = GetPageBox("MediaBox");
    }

    return pageBox;
}

int PdfPage::GetRotationRaw() const
{
    int rot = 0;

    const PdfObject* pObj = GetInheritedKeyFromObject("Rotate", this->GetObject());
    if (pObj && (pObj->IsNumber() || pObj->GetReal()))
        rot = static_cast<int>(pObj->GetNumber());

    return rot;
}

void PdfPage::SetRotationRaw(int nRotation)
{
    if (nRotation != 0 && nRotation != 90 && nRotation != 180 && nRotation != 270)
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);

    this->GetObject().GetDictionary().AddKey("Rotate", PdfVariant(static_cast<int64_t>(nRotation)));
}

PdfArray* PdfPage::GetAnnotationsArray() const
{
    auto obj = const_cast<PdfPage&>(*this).GetObject().GetDictionary().FindKey("Annots");
    if (obj == nullptr)
        return nullptr;

    return &obj->GetArray();
}

PdfArray& PdfPage::GetOrCreateAnnotationsArray()
{
    auto& dict = GetObject().GetDictionary();
    PdfObject* pObj = dict.FindKey("Annots");

    if (pObj == nullptr)
        pObj = &dict.AddKey("Annots", PdfArray());

    return pObj->GetArray();
}

unsigned PdfPage::GetAnnotationCount() const
{
    auto arr = GetAnnotationsArray();
    return arr == nullptr ? 0 : arr->GetSize();
}

PdfAnnotation* PdfPage::CreateAnnotation(PdfAnnotationType eType, const PdfRect& rRect)
{
    PdfAnnotation* pAnnot = new PdfAnnotation(*this, eType, rRect);
    PdfReference   ref = pAnnot->GetObject().GetIndirectReference();

    auto& arr = GetOrCreateAnnotationsArray();
    arr.push_back(ref);
    m_mapAnnotations[&pAnnot->GetObject()] = pAnnot;

    // Default set print flag
    auto flags = pAnnot->GetFlags();
    pAnnot->SetFlags(flags | PdfAnnotationFlags::Print);

    return pAnnot;
}

PdfAnnotation* PdfPage::GetAnnotation(unsigned index)
{
    PdfAnnotation* annot;
    PdfReference ref;

    auto arr = GetAnnotationsArray();

    if (arr == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    if (index >= arr->GetSize())
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);

    auto& obj = arr->FindAt(index);
    annot = m_mapAnnotations[&obj];
    if (annot == nullptr)
    {
        annot = new PdfAnnotation(*this, obj);
        m_mapAnnotations[&obj] = annot;
    }

    return annot;
}

void PdfPage::DeleteAnnotation(unsigned index)
{
    auto arr = GetAnnotationsArray();
    if (arr == nullptr)
        return;

    if (index >= arr->GetSize())
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);

    auto& pItem = arr->FindAt(index);
    auto found = m_mapAnnotations.find(&pItem);
    if (found != m_mapAnnotations.end())
    {
        delete found->second;
        m_mapAnnotations.erase(found);
    }

    // Delete the PdfObject in the document
    if (pItem.GetIndirectReference().IsIndirect())
        pItem.GetDocument()->GetObjects().RemoveObject(pItem.GetIndirectReference());

    // Delete the annotation from the annotation array.
    // Has to be performed at last
    arr->RemoveAt(index);
}

void PdfPage::DeleteAnnotation(PdfObject& annotObj)
{
    PdfArray* arr = GetAnnotationsArray();
    if (arr == nullptr)
        return;

    // find the array iterator pointing to the annotation, so it can be deleted later
    int index = -1;
    for (unsigned i = 0; i < arr->GetSize(); i++)
    {
        // CLEAN-ME: The following is ugly. Fix operator== in PdfOject and PdfVariant and use operator==
        auto& obj = arr->FindAt(i);
        if (&annotObj == &obj)
        {
            index = (int)i;
            break;
        }
    }

    if (index == -1)
    {
        // The object was not found as annotation in this page
        return;
    }

    // Delete any cached PdfAnnotations
    auto found = m_mapAnnotations.find((PdfObject*)&annotObj);
    if (found != m_mapAnnotations.end())
    {
        delete found->second;
        m_mapAnnotations.erase(found);
    }

    // Delete the PdfObject in the document
    if (annotObj.GetIndirectReference().IsIndirect())
        GetObject().GetDocument()->GetObjects().RemoveObject(annotObj.GetIndirectReference());

    // Delete the annotation from the annotation array.
    // Has to be performed at last
    arr->RemoveAt(index);
}

// added by Petr P. Petrov 21 Febrary 2010
bool PdfPage::SetPageWidth(int newWidth)
{
    PdfObject* pObjMediaBox;

    // Take advantage of inherited values - walking up the tree if necessary
    pObjMediaBox = const_cast<PdfObject*>(GetInheritedKeyFromObject("MediaBox", this->GetObject()));

    // assign the value of the box from the array
    if (pObjMediaBox && pObjMediaBox->IsArray())
    {
        auto& mediaBoxArr = pObjMediaBox->GetArray();

        // in PdfRect::FromArray(), the Left value is subtracted from Width
        double dLeftMediaBox = mediaBoxArr[0].GetReal();
        mediaBoxArr[2] = PdfObject(newWidth + dLeftMediaBox);

        PdfObject* pObjCropBox;

        // Take advantage of inherited values - walking up the tree if necessary
        pObjCropBox = const_cast<PdfObject*>(GetInheritedKeyFromObject("CropBox", this->GetObject()));

        if (pObjCropBox && pObjCropBox->IsArray())
        {
            auto& cropBoxArr = pObjCropBox->GetArray();
            // in PdfRect::FromArray(), the Left value is subtracted from Width
            double dLeftCropBox = cropBoxArr[0].GetReal();
            cropBoxArr[2] = PdfObject(newWidth + dLeftCropBox);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

// added by Petr P. Petrov 21 Febrary 2010
bool PdfPage::SetPageHeight(int newHeight)
{
    PdfObject* pObj;

    // Take advantage of inherited values - walking up the tree if necessary
    pObj = const_cast<PdfObject*>(GetInheritedKeyFromObject("MediaBox", this->GetObject()));

    // assign the value of the box from the array
    if (pObj && pObj->IsArray())
    {
        auto& mediaBoxArr = pObj->GetArray();
        // in PdfRect::FromArray(), the Bottom value is subtracted from Height
        double dBottom = mediaBoxArr[1].GetReal();
        mediaBoxArr[3] = PdfObject(newHeight + dBottom);

        PdfObject* pObjCropBox;

        // Take advantage of inherited values - walking up the tree if necessary
        pObjCropBox = const_cast<PdfObject*>(GetInheritedKeyFromObject("CropBox", this->GetObject()));

        if (pObjCropBox && pObjCropBox->IsArray())
        {
            auto& cropBoxArr = pObjCropBox->GetArray();
            // in PdfRect::FromArray(), the Bottom value is subtracted from Height
            double dBottomCropBox = cropBoxArr[1].GetReal();
            cropBoxArr[3] = PdfObject(newHeight + dBottomCropBox);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void PdfPage::SetMediaBox(const PdfRect& rSize)
{
    PdfVariant mediaBox;
    rSize.ToVariant(mediaBox);
    this->GetObject().GetDictionary().AddKey("MediaBox", mediaBox);
}

void PdfPage::SetTrimBox(const PdfRect& rSize)
{
    PdfVariant trimbox;
    rSize.ToVariant(trimbox);
    this->GetObject().GetDictionary().AddKey("TrimBox", trimbox);
}

unsigned PdfPage::GetPageNumber() const
{
    unsigned pageNumber = 0;
    auto parent = this->GetObject().GetDictionary().FindKey("Parent");
    PdfReference ref = this->GetObject().GetIndirectReference();

    // CVE-2017-5852 - prevent infinite loop if Parent chain contains a loop
    // e.g. pParent->GetIndirectKey( "Parent" ) == pParent or pParent->GetIndirectKey( "Parent" )->GetIndirectKey( "Parent" ) == pParent
    constexpr unsigned maxRecursionDepth = 1000;
    unsigned depth = 0;

    while (parent != nullptr)
    {
        auto kidsObj = parent->GetDictionary().FindKey("Kids");
        if (kidsObj != nullptr)
        {
            const PdfArray& kids = kidsObj->GetArray();
            for (auto& child : kids)
            {
                if (child.GetReference() == ref)
                    break;

                auto node = this->GetObject().GetDocument()->GetObjects().GetObject(child.GetReference());
                if (node == nullptr)
                {
                    std::ostringstream oss;
                    oss << "Object " << child.GetReference().ToString() << " not found from Kids array "
                        << kidsObj->GetIndirectReference().ToString();
                    PODOFO_RAISE_ERROR_INFO(EPdfError::NoObject, oss.str());
                }

                if (node->GetDictionary().GetKey(PdfName::KeyType) != nullptr
                    && node->GetDictionary().GetKey(PdfName::KeyType)->GetName() == PdfName("Pages"))
                {
                    auto count = node->GetDictionary().FindKey("Count");
                    if (count != nullptr)
                        pageNumber += static_cast<int>(count->GetNumber());
                }
                else
                {
                    // if we do not have a page tree node, 
                    // we most likely have a page object:
                    // so the page count is 1
                    pageNumber++;
                }
            }
        }

        ref = parent->GetIndirectReference();
        parent = parent->GetDictionary().FindKey("Parent");
        depth++;

        if (depth > maxRecursionDepth)
            PODOFO_RAISE_ERROR_INFO(EPdfError::BrokenFile, "Loop in Parent chain");
    }

    return ++pageNumber;
}

PdfObject* PdfPage::GetFromResources(const PdfName& type, const PdfName& key)
{
    if (m_Resources == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "No Resources");

    if (m_Resources->GetDictionary().HasKey(type))
    {
        auto typeObj = m_Resources->GetDictionary().FindKey(type);
        if (typeObj != nullptr && typeObj->IsDictionary() && typeObj->GetDictionary().HasKey(key))
        {
            auto obj = typeObj->GetDictionary().GetKey(key);
            if (obj->IsReference())
            {
                const PdfReference& ref = typeObj->GetDictionary().GetKey(key)->GetReference();
                return this->GetObject().GetDocument()->GetObjects().GetObject(ref);
            }
            return obj;
        }
    }

    return nullptr;
}

void PdfPage::SetICCProfile(const string_view& csTag, PdfInputStream& stream,
    int64_t colorComponents, PdfColorSpace alternateColorSpace)
{
    // Check nColorComponents for a valid value
    if (colorComponents != 1 &&
        colorComponents != 3 &&
        colorComponents != 4)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "SetICCProfile nColorComponents must be 1, 3 or 4!");
    }

    // Create a colorspace object
    PdfObject* iccObject = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
    PdfName nameForCS = PdfColor::GetNameForColorSpace(alternateColorSpace);
    iccObject->GetDictionary().AddKey(PdfName("Alternate"), nameForCS);
    iccObject->GetDictionary().AddKey(PdfName("N"), colorComponents);
    iccObject->GetOrCreateStream().Set(stream);

    // Add the colorspace
    PdfArray array;
    array.push_back(PdfName("ICCBased"));
    array.push_back(iccObject->GetIndirectReference());

    PoDoFo::PdfDictionary iccBasedDictionary;
    iccBasedDictionary.AddKey(csTag, array);

    // Add the colorspace to resource
    GetResources().GetDictionary().AddKey(PdfName("ColorSpace"), iccBasedDictionary);
}

PdfObject& PdfPage::GetContents()
{
    EnsureContentsCreated();
    return m_contents->GetContents();
}

PdfObject& PdfPage::GetResources()
{
    EnsureResourcesCreated();
    return *m_Resources;
}

PdfRect PdfPage::GetMediaBox() const
{
    return GetPageBox("MediaBox");
}

PdfRect PdfPage::GetCropBox() const
{
    return GetPageBox("CropBox");
}

PdfRect PdfPage::GetTrimBox() const
{
    return GetPageBox("TrimBox");
}

PdfRect PdfPage::GetBleedBox() const
{
    return GetPageBox("BleedBox");
}

PdfRect PdfPage::GetArtBox() const
{
    return GetPageBox("ArtBox");
}

const PdfObject* PdfPage::GetInheritedKey(const PdfName& rName) const
{
    return this->GetInheritedKeyFromObject(rName.GetString().c_str(), this->GetObject());
}

// https://stackoverflow.com/a/2021986/213871
int normalize(int value, int start, int end)
{
    int width = end - start;
    int offsetValue = value - start;   // value relative to 0

    // + start to reset back to start of original range
    return offsetValue - (offsetValue / width) * width + start;
}
