/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfPage.h"

#include "PdfDictionary.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfWriter.h"
#include "PdfObjectStream.h"
#include "PdfColor.h"
#include "PdfDocument.h"

using namespace std;
using namespace mm;

static int normalize(int value, int start, int end);
static PdfResources* getResources(PdfObject& obj, const deque<PdfObject*>& listOfParents);

PdfPage::PdfPage(PdfDocument& parent, const PdfRect& size)
    : PdfDictionaryElement(parent, "Page"), m_Contents(nullptr)
{
    InitNewPage(size);
}

PdfPage::PdfPage(PdfObject& obj, const deque<PdfObject*>& listOfParents)
    : PdfDictionaryElement(obj), m_Contents(nullptr), m_Resources(::getResources(obj, listOfParents))
{
    PdfObject* contents = obj.GetDictionary().FindKey("Contents");
    if (contents != nullptr)
        m_Contents.reset(new PdfContents(*this, *contents));
}

PdfPage::~PdfPage()
{
    for (auto& pair : m_mapAnnotations)
        delete pair.second;
}

PdfRect PdfPage::GetRect() const
{
    return this->GetMediaBox();
}

bool PdfPage::HasRotation(double& teta) const
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

void PdfPage::InitNewPage(const PdfRect& size)
{
    SetMediaBox(size);
}

void PdfPage::EnsureContentsCreated()
{
    if (m_Contents != nullptr)
        return;

    m_Contents.reset(new PdfContents(*this));
    GetObject().GetDictionary().AddKey(PdfName::KeyContents,
        m_Contents->GetObject().GetIndirectReference());
}

void PdfPage::EnsureResourcesCreated()
{
    if (m_Resources != nullptr)
        return;

    m_Resources.reset(new PdfResources(GetDictionary()));
}

PdfObjectStream& PdfPage::GetStreamForAppending(PdfStreamAppendFlags flags)
{
    EnsureContentsCreated();
    return m_Contents->GetStreamForAppending(flags);
}

PdfRect PdfPage::CreateStandardPageSize(const PdfPageSize pageSize, bool landscape)
{
    PdfRect rect;

    switch (pageSize)
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

    if (landscape)
    {
        double dTmp = rect.GetWidth();
        rect.SetWidth(rect.GetHeight());
        rect.SetHeight(dTmp);
    }

    return rect;
}

const PdfObject* PdfPage::GetInheritedKeyFromObject(const string_view& key, const PdfObject& inObject, int depth) const
{
    const PdfObject* obj = nullptr;

    // check for it in the object itself
    if (inObject.GetDictionary().HasKey(key))
    {
        obj = &inObject.GetDictionary().MustFindKey(key);
        if (!obj->IsNull())
            return obj;
    }

    // if we get here, we need to go check the parent - if there is one!
    if (inObject.GetDictionary().HasKey("Parent"))
    {
        // CVE-2017-5852 - prevent stack overflow if Parent chain contains a loop, or is very long
        // e.g. obj->GetParent() == obj or obj->GetParent()->GetParent() == obj
        // default stack sizes
        // Windows: 1 MB
        // Linux: 2 MB
        // macOS: 8 MB for main thread, 0.5 MB for secondary threads
        // 0.5 MB is enough space for 1000 512 byte stack frames and 2000 256 byte stack frames
        const int maxRecursionDepth = 1000;

        if (depth > maxRecursionDepth)
            PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

        obj = inObject.GetDictionary().FindKey("Parent");
        if (obj == &inObject)
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object {} references itself as Parent", inObject.GetIndirectReference().ToString());
        }

        if (obj != nullptr)
            obj = GetInheritedKeyFromObject(key, *obj, depth + 1);
    }

    return obj;
}

PdfRect PdfPage::GetPageBox(const string_view& inBox) const
{
    PdfRect	pageBox;

    // Take advantage of inherited values - walking up the tree if necessary
    auto obj = GetInheritedKeyFromObject(inBox, this->GetObject());

    // assign the value of the box from the array
    if (obj != nullptr && obj->IsArray())
    {
        pageBox.FromArray(obj->GetArray());
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

    auto obj = GetInheritedKeyFromObject("Rotate", this->GetObject());
    if (obj != nullptr && (obj->IsNumber() || obj->GetReal()))
        rot = static_cast<int>(obj->GetNumber());

    return rot;
}

void PdfPage::SetRotationRaw(int rotation)
{
    if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    this->GetObject().GetDictionary().AddKey("Rotate", PdfVariant(static_cast<int64_t>(rotation)));
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
    PdfObject* obj = dict.FindKey("Annots");

    if (obj == nullptr)
        obj = &dict.AddKey("Annots", PdfArray());

    return obj->GetArray();
}

unsigned PdfPage::GetAnnotationCount() const
{
    auto arr = GetAnnotationsArray();
    return arr == nullptr ? 0 : arr->GetSize();
}

PdfAnnotation* PdfPage::CreateAnnotation(PdfAnnotationType annotType, const PdfRect& rect)
{
    PdfAnnotation* pAnnot = new PdfAnnotation(*this, annotType, rect);
    PdfReference   ref = pAnnot->GetObject().GetIndirectReference();

    auto& arr = GetOrCreateAnnotationsArray();
    arr.Add(ref);
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
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (index >= arr->GetSize())
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

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
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

    auto& item = arr->FindAt(index);
    auto found = m_mapAnnotations.find(&item);
    if (found != m_mapAnnotations.end())
    {
        delete found->second;
        m_mapAnnotations.erase(found);
    }

    // Delete the PdfObject in the document
    if (item.GetIndirectReference().IsIndirect())
        item.GetDocument()->GetObjects().RemoveObject(item.GetIndirectReference());

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
    // Take advantage of inherited values - walking up the tree if necessary
    auto mediaBoxObj = const_cast<PdfObject*>(GetInheritedKeyFromObject("MediaBox", this->GetObject()));

    // assign the value of the box from the array
    if (mediaBoxObj != nullptr && mediaBoxObj->IsArray())
    {
        auto& mediaBoxArr = mediaBoxObj->GetArray();

        // in PdfRect::FromArray(), the Left value is subtracted from Width
        double dLeftMediaBox = mediaBoxArr[0].GetReal();
        mediaBoxArr[2] = PdfObject(newWidth + dLeftMediaBox);

        // Take advantage of inherited values - walking up the tree if necessary
        auto cropBoxObj = const_cast<PdfObject*>(GetInheritedKeyFromObject("CropBox", this->GetObject()));

        if (cropBoxObj != nullptr && cropBoxObj->IsArray())
        {
            auto& cropBoxArr = cropBoxObj->GetArray();
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
    // Take advantage of inherited values - walking up the tree if necessary
    auto obj = const_cast<PdfObject*>(GetInheritedKeyFromObject("MediaBox", this->GetObject()));

    // assign the value of the box from the array
    if (obj != nullptr && obj->IsArray())
    {
        auto& mediaBoxArr = obj->GetArray();
        // in PdfRect::FromArray(), the Bottom value is subtracted from Height
        double bottom = mediaBoxArr[1].GetReal();
        mediaBoxArr[3] = PdfObject(newHeight + bottom);

        // Take advantage of inherited values - walking up the tree if necessary
        auto cropBoxObj = const_cast<PdfObject*>(GetInheritedKeyFromObject("CropBox", this->GetObject()));

        if (cropBoxObj != nullptr && cropBoxObj->IsArray())
        {
            auto& cropBoxArr = cropBoxObj->GetArray();
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

void PdfPage::SetMediaBox(const PdfRect& size)
{
    PdfArray mediaBox;
    size.ToArray(mediaBox);
    this->GetObject().GetDictionary().AddKey("MediaBox", mediaBox);
}

void PdfPage::SetTrimBox(const PdfRect& size)
{
    PdfArray trimbox;
    size.ToArray(trimbox);
    this->GetObject().GetDictionary().AddKey("TrimBox", trimbox);
}

unsigned PdfPage::GetPageNumber() const
{
    unsigned pageNumber = 0;
    auto parent = this->GetObject().GetDictionary().FindKey("Parent");
    PdfReference ref = this->GetObject().GetIndirectReference();

    // CVE-2017-5852 - prevent infinite loop if Parent chain contains a loop
    // e.g. parent->GetIndirectKey( "Parent" ) == parent or parent->GetIndirectKey( "Parent" )->GetIndirectKey( "Parent" ) == parent
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
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject,
                        "Object {} not found from Kids array {}", child.GetReference().ToString(),
                        kidsObj->GetIndirectReference().ToString());
                }

                if (node->GetDictionary().HasKey(PdfName::KeyType)
                    && node->GetDictionary().MustFindKey(PdfName::KeyType).GetName() == "Pages")
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
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile, "Loop in Parent chain");
    }

    return ++pageNumber;
}

void PdfPage::SetICCProfile(const string_view& csTag, PdfInputStream& stream,
    int64_t colorComponents, PdfColorSpace alternateColorSpace)
{
    // Check nColorComponents for a valid value
    if (colorComponents != 1 &&
        colorComponents != 3 &&
        colorComponents != 4)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "SetICCProfile nColorComponents must be 1, 3 or 4!");
    }

    // Create a colorspace object
    PdfObject* iccObject = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
    PdfName nameForCS = PdfColor::GetNameForColorSpace(alternateColorSpace);
    iccObject->GetDictionary().AddKey("Alternate", nameForCS);
    iccObject->GetDictionary().AddKey("N", colorComponents);
    iccObject->GetOrCreateStream().Set(stream);

    // Add the colorspace
    PdfArray array;
    array.Add(PdfName("ICCBased"));
    array.Add(iccObject->GetIndirectReference());

    PdfDictionary iccBasedDictionary;
    iccBasedDictionary.AddKey(csTag, array);

    // Add the colorspace to resource
    GetOrCreateResources().GetDictionary().AddKey("ColorSpace", iccBasedDictionary);
}

PdfContents& PdfPage::GetOrCreateContents()
{
    EnsureContentsCreated();
    return *m_Contents;
}

PdfResources* PdfPage::getResources() const
{
    return m_Resources.get();
}

PdfObject* PdfPage::getContentsObject() const
{
    if (m_Contents == nullptr)
        return nullptr;

    return &const_cast<PdfContents&>(*m_Contents).GetObject();
}

PdfElement& PdfPage::getElement() const
{
    return const_cast<PdfPage&>(*this);
}

PdfResources& PdfPage::GetOrCreateResources()
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

const PdfObject* PdfPage::GetInheritedKey(const PdfName& name) const
{
    return this->GetInheritedKeyFromObject(name.GetString().c_str(), this->GetObject());
}

// https://stackoverflow.com/a/2021986/213871
int normalize(int value, int start, int end)
{
    int width = end - start;
    int offsetValue = value - start;   // value relative to 0

    // + start to reset back to start of original range
    return offsetValue - (offsetValue / width) * width + start;
}

PdfResources* getResources(PdfObject& obj, const deque<PdfObject*>& listOfParents)
{
    auto resources = obj.GetDictionary().FindKey("Resources");
    if (resources == nullptr)
    {
        // Resources might be inherited
        for (auto& parent : listOfParents)
            resources = parent->GetDictionary().FindKey("Resources");
    }

    if (resources == nullptr)
        return nullptr;

    return new PdfResources(*resources);
}
