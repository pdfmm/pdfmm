/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfXObject.h"

#include <sstream>

#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfDocument.h"

using namespace std;
using namespace mm;

PdfXObject::PdfXObject(PdfDocument& doc, const PdfRect& rect, const string_view& prefix, bool withoutObjNum)
    : PdfElement(doc, "XObject"), m_Rect(rect), m_Resources(nullptr)
{
    InitXObject(rect, prefix);
    if (withoutObjNum)
        m_Identifier = PdfName(prefix);
}

PdfXObject::PdfXObject(PdfDocument& doc, const PdfDocument& sourceDoc, unsigned pageIndex, const string_view& prefix, bool useTrimBox)
    : PdfElement(doc, "XObject"), m_Resources(nullptr)
{
    InitXObject(m_Rect, prefix);

    // Implementation note: source document must be different from distination
    if (&doc == reinterpret_cast<const PdfDocument*>(&sourceDoc))
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

    // After filling set correct BBox, independent of rotation
    m_Rect = doc.FillXObjectFromDocumentPage(*this, sourceDoc, pageIndex, useTrimBox);

    InitAfterPageInsertion(sourceDoc, pageIndex);
}

PdfXObject::PdfXObject(PdfDocument& doc, unsigned pageIndex, const string_view& prefix, bool useTrimBox)
    : PdfElement(doc, "XObject"), PdfCanvas(), m_Resources(nullptr)
{
    m_Rect = PdfRect();

    InitXObject(m_Rect, prefix);

    // After filling set correct BBox, independent of rotation
    m_Rect = doc.FillXObjectFromExistingPage(*this, pageIndex, useTrimBox);

    InitAfterPageInsertion(doc, pageIndex);
}

PdfXObject::PdfXObject(PdfObject& obj)
    : PdfElement(obj), PdfCanvas(), m_Resources(nullptr)
{
    InitIdentifiers(getPdfXObjectType(obj), { });
    m_Resources = obj.GetDictionary().FindKey("Resources");

    if (obj.GetDictionary().HasKey("BBox"))
        m_Rect = PdfRect(obj.GetDictionary().MustFindKey("BBox").GetArray());
}

PdfXObject::PdfXObject(PdfDocument& doc, PdfXObjectType subType, const string_view& prefix)
    : PdfElement(doc, "XObject"), m_Resources(nullptr)
{
    InitIdentifiers(subType, prefix);

    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(subType)));
}

PdfXObject::PdfXObject(PdfObject& obj, PdfXObjectType subType)
    : PdfElement(obj), m_Resources(nullptr)
{
    if (getPdfXObjectType(obj) != subType)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    InitIdentifiers(subType, { });
}

bool PdfXObject::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfXObject>& xobj, PdfXObjectType& type)
{
    auto typeObj = obj.GetDictionary().GetKey(PdfName::KeyType);
    if (typeObj == nullptr
        || !typeObj->IsName()
        || typeObj->GetName().GetString() != "XObject")
    {
        xobj = nullptr;
        type = PdfXObjectType::Unknown;
        return false;
    }

    type = getPdfXObjectType(obj);
    switch (type)
    {
        case PdfXObjectType::Form:
        case PdfXObjectType::PostScript:
        {
            xobj.reset(new PdfXObject(obj, type));
            return true;
        }
        case PdfXObjectType::Image:
        {
            xobj.reset(new PdfImage(obj));
            return true;
        }
        default:
        {
            xobj = nullptr;
            return false;
        }
    }
}

PdfXObjectType PdfXObject::getPdfXObjectType(const PdfObject& obj)
{
    auto subTypeObj = obj.GetDictionary().FindKey(PdfName::KeySubtype);
    if (subTypeObj == nullptr || !subTypeObj->IsName())
        return PdfXObjectType::Unknown;

    auto subtype = obj.GetDictionary().FindKey(PdfName::KeySubtype)->GetName().GetString();
    return FromString(subtype);
}

string PdfXObject::ToString(PdfXObjectType type)
{
    switch (type)
    {
        case PdfXObjectType::Form:
            return "Form";
        case PdfXObjectType::Image:
            return "Image";
        case PdfXObjectType::PostScript:
            return "PS";
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }
}

PdfXObjectType PdfXObject::FromString(const string& str)
{
    if (str == "Form")
        return PdfXObjectType::Form;
    else if (str == "Image")
        return PdfXObjectType::Image;
    else if (str == "PS")
        return PdfXObjectType::PostScript;
    else
        return PdfXObjectType::Unknown;
}

void PdfXObject::InitAfterPageInsertion(const PdfDocument& doc, unsigned pageIndex)
{
    PdfArray bbox;
    m_Rect.ToArray(bbox);
    this->GetObject().GetDictionary().AddKey("BBox", bbox);

    int rotation = doc.GetPageTree().GetPage(pageIndex).GetRotationRaw();
    // correct negative rotation
    if (rotation < 0)
        rotation = 360 + rotation;

    // Swap offsets/width/height for vertical rotation
    switch (rotation)
    {
        case 90:
        case 270:
        {
            double temp;

            temp = m_Rect.GetWidth();
            m_Rect.SetWidth(m_Rect.GetHeight());
            m_Rect.SetHeight(temp);

            temp = m_Rect.GetLeft();
            m_Rect.SetLeft(m_Rect.GetBottom());
            m_Rect.SetBottom(temp);
        }
        break;

        default:
            break;
    }

    // Build matrix for rotation and cropping
    double alpha = -rotation / 360.0 * 2.0 * M_PI;

    double a, b, c, d, e, f;

    a = cos(alpha);
    b = sin(alpha);
    c = -sin(alpha);
    d = cos(alpha);

    switch (rotation)
    {
        case 90:
            e = -m_Rect.GetLeft();
            f = m_Rect.GetBottom() + m_Rect.GetHeight();
            break;

        case 180:
            e = m_Rect.GetLeft() + m_Rect.GetWidth();
            f = m_Rect.GetBottom() + m_Rect.GetHeight();
            break;

        case 270:
            e = m_Rect.GetLeft() + m_Rect.GetWidth();
            f = -m_Rect.GetBottom();
            break;

        case 0:
        default:
            e = -m_Rect.GetLeft();
            f = -m_Rect.GetBottom();
            break;
    }

    PdfArray matrix;
    matrix.push_back(PdfObject(a));
    matrix.push_back(PdfObject(b));
    matrix.push_back(PdfObject(c));
    matrix.push_back(PdfObject(d));
    matrix.push_back(PdfObject(e));
    matrix.push_back(PdfObject(f));

    this->GetObject().GetDictionary().AddKey("Matrix", matrix);
}

PdfRect PdfXObject::GetRect() const
{
    return m_Rect;
}

bool PdfXObject::HasRotation(double& teta) const
{
    teta = 0;
    return false;
}

void PdfXObject::SetRect(const PdfRect& rect)
{
    PdfArray bbox;
    rect.ToArray(bbox);
    GetObject().GetDictionary().AddKey("BBox", bbox);
    m_Rect = rect;
}

void PdfXObject::EnsureResourcesInitialized()
{
    if (m_Resources == nullptr)
        InitResources();

    // A Form XObject must have a stream
    GetObject().ForceCreateStream();
}

inline PdfStream& PdfXObject::GetStreamForAppending(PdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject().GetOrCreateStream();
}

void PdfXObject::InitXObject(const PdfRect& rect, const string_view& prefix)
{
    InitIdentifiers(PdfXObjectType::Form, prefix);

    // Initialize static data
    if (m_matrix.empty())
    {
        // This matrix is the same for all PdfXObjects so cache it
        m_matrix.push_back(PdfObject(static_cast<int64_t>(1)));
        m_matrix.push_back(PdfObject(static_cast<int64_t>(0)));
        m_matrix.push_back(PdfObject(static_cast<int64_t>(0)));
        m_matrix.push_back(PdfObject(static_cast<int64_t>(1)));
        m_matrix.push_back(PdfObject(static_cast<int64_t>(0)));
        m_matrix.push_back(PdfObject(static_cast<int64_t>(0)));
    }

    PdfArray bbox;
    rect.ToArray(bbox);
    this->GetObject().GetDictionary().AddKey("BBox", bbox);
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(PdfXObjectType::Form)));
    this->GetObject().GetDictionary().AddKey("FormType", PdfVariant(static_cast<int64_t>(1))); // only 1 is only defined in the specification.
    this->GetObject().GetDictionary().AddKey("Matrix", m_matrix);

    InitResources();
}

void PdfXObject::InitIdentifiers(PdfXObjectType subType, const string_view& prefix)
{
    ostringstream out;
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    if (prefix.length() == 0)
        out << "XOb" << this->GetObject().GetIndirectReference().ObjectNumber();
    else
        out << prefix << this->GetObject().GetIndirectReference().ObjectNumber();

    m_Identifier = PdfName(out.str().c_str());
    m_Reference = this->GetObject().GetIndirectReference();
    m_type = subType;
}

void PdfXObject::InitResources()
{
    // The PDF specification suggests that we send all available PDF Procedure sets
    m_Resources = &this->GetObject().GetDictionary().AddKey("Resources", PdfDictionary());
    m_Resources->GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}


PdfObject& PdfXObject::GetOrCreateContents()
{
    return GetObject();
}

PdfObject& PdfXObject::GetOrCreateResources()
{
    EnsureResourcesInitialized();
    return *m_Resources;
}
