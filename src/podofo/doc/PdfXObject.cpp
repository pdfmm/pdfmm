/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfXObject.h"

#include <sstream>

#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"

#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfDocument.h"

using namespace std;
using namespace PoDoFo;

PdfXObject::PdfXObject(PdfDocument& doc, const PdfRect& rect, const string_view& prefix, bool withoutObjNum)
    : PdfElement(doc, "XObject"), m_rRect(rect), m_pResources(nullptr)
{
    InitXObject(rect, prefix);
    if (withoutObjNum)
        m_Identifier = PdfName(prefix);
}

PdfXObject::PdfXObject(PdfDocument& doc, const PdfDocument& sourceDoc, unsigned pageIndex, const string_view& prefix, bool useTrimBox)
    : PdfElement(doc, "XObject"), m_pResources(nullptr)
{
    InitXObject(m_rRect, prefix);

    // Implementation note: source document must be different from distination
    if (&doc == reinterpret_cast<const PdfDocument*>(&sourceDoc))
    {
        PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
    }

    // After filling set correct BBox, independent of rotation
    m_rRect = doc.FillXObjectFromDocumentPage(*this, sourceDoc, pageIndex, useTrimBox);

    InitAfterPageInsertion(sourceDoc, pageIndex);
}

PdfXObject::PdfXObject(PdfDocument& doc, unsigned pageIndex, const string_view& prefix, bool useTrimBox)
    : PdfElement(doc, "XObject"), PdfCanvas(), m_pResources(nullptr)
{
    m_rRect = PdfRect();

    InitXObject(m_rRect, prefix);

    // After filling set correct BBox, independent of rotation
    m_rRect = doc.FillXObjectFromExistingPage(*this, pageIndex, useTrimBox);

    InitAfterPageInsertion(doc, pageIndex);
}

PdfXObject::PdfXObject(PdfObject& obj)
    : PdfElement(obj), PdfCanvas(), m_pResources(nullptr)
{
    InitIdentifiers(getPdfXObjectType(obj), { });
    m_pResources = obj.GetDictionary().FindKey("Resources");

    if (obj.GetDictionary().FindKey("BBox"))
        m_rRect = PdfRect(obj.GetDictionary().FindKey("BBox")->GetArray());
}

PdfXObject::PdfXObject(PdfDocument& doc, PdfXObjectType subType, const string_view& prefix)
    : PdfElement(doc, "XObject"), m_pResources(nullptr)
{
    InitIdentifiers(subType, prefix);

    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(subType)));
}

PdfXObject::PdfXObject(PdfObject& obj, PdfXObjectType subType)
    : PdfElement(obj), m_pResources(nullptr)
{
    if (getPdfXObjectType(obj) != subType)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    InitIdentifiers(subType, { });
}

bool PdfXObject::TryCreateFromObject(PdfObject &obj, unique_ptr<PdfXObject>& xobj, PdfXObjectType &type)
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

PdfXObjectType PdfXObject::getPdfXObjectType(const PdfObject &obj)
{
    auto subTypeObj = obj.GetDictionary().GetKey(PdfName::KeySubtype);
    if (subTypeObj == nullptr || !subTypeObj->IsName())
        return PdfXObjectType::Unknown;

    auto subtype = obj.GetDictionary().GetKey(PdfName::KeySubtype)->GetName().GetString();
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
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }
}

PdfXObjectType PdfXObject::FromString(const string &str)
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
    PdfVariant var;
    m_rRect.ToVariant(var);
    this->GetObject().GetDictionary().AddKey("BBox", var);

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

            temp = m_rRect.GetWidth();
            m_rRect.SetWidth(m_rRect.GetHeight());
            m_rRect.SetHeight(temp);

            temp = m_rRect.GetLeft();
            m_rRect.SetLeft(m_rRect.GetBottom());
            m_rRect.SetBottom(temp);
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
            e = -m_rRect.GetLeft();
            f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;

        case 180:
            e = m_rRect.GetLeft() + m_rRect.GetWidth();
            f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;

        case 270:
            e = m_rRect.GetLeft() + m_rRect.GetWidth();
            f = -m_rRect.GetBottom();
            break;

        case 0:
        default:
            e = -m_rRect.GetLeft();
            f = -m_rRect.GetBottom();
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
    return m_rRect;
}

bool PdfXObject::HasRotation(double& teta) const
{
    teta = 0;
    return false;
}

void PdfXObject::SetRect(const PdfRect& rect)
{
    PdfVariant array;
    rect.ToVariant(array);
    GetObject().GetDictionary().AddKey("BBox", array);
    m_rRect = rect;
}

void PdfXObject::EnsureResourcesInitialized()
{
    if (m_pResources == nullptr)
        InitResources();

    // A Form XObject must have a stream
    GetObject().ForceCreateStream();
}

inline PdfStream & PdfXObject::GetStreamForAppending(EPdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject().GetOrCreateStream();
}

void PdfXObject::InitXObject(const PdfRect& rRect, const string_view& prefix)
{
    InitIdentifiers(PdfXObjectType::Form, prefix);

    // Initialize static data
    if (m_matrix.empty())
    {
        // This matrix is the same for all PdfXObjects so cache it
        m_matrix.push_back(PdfVariant(static_cast<int64_t>(1)));
        m_matrix.push_back(PdfVariant(static_cast<int64_t>(0)));
        m_matrix.push_back(PdfVariant(static_cast<int64_t>(0)));
        m_matrix.push_back(PdfVariant(static_cast<int64_t>(1)));
        m_matrix.push_back(PdfVariant(static_cast<int64_t>(0)));
        m_matrix.push_back(PdfVariant(static_cast<int64_t>(0)));
    }

    PdfVariant    var;
    rRect.ToVariant(var);
    this->GetObject().GetDictionary().AddKey("BBox", var);
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
    this->GetObject().GetDictionary().AddKey("Resources", PdfObject(PdfDictionary()));
    m_pResources = this->GetObject().GetDictionary().GetKey("Resources");
    m_pResources->GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}


PdfObject& PdfXObject::GetContents()
{
    return const_cast<PdfXObject&>(*this).GetObject();
}

PdfObject& PdfXObject::GetResources()
{
    const_cast<PdfXObject&>(*this).EnsureResourcesInitialized();
    return *m_pResources;
}
