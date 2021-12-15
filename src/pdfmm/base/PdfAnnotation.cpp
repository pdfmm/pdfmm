/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>

#include "PdfAnnotation.h"
#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfDate.h"
#include "PdfAction.h"
#include "PdfFileSpec.h"
#include "PdfPage.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfXObjectForm.h"

using namespace std;
using namespace mm;

static const char* s_names[] = {
    nullptr,
    "Text",                       // - supported
    "Link",
    "FreeText",       // PDF 1.3  // - supported
    "Line",           // PDF 1.3  // - supported
    "Square",         // PDF 1.3
    "Circle",         // PDF 1.3
    "Polygon",        // PDF 1.5
    "PolyLine",       // PDF 1.5
    "Highlight",      // PDF 1.3
    "Underline",      // PDF 1.3
    "Squiggly",       // PDF 1.4
    "StrikeOut",      // PDF 1.3
    "Stamp",          // PDF 1.3
    "Caret",          // PDF 1.5
    "Ink",            // PDF 1.3
    "Popup",          // PDF 1.3
    "FileAttachment", // PDF 1.3
    "Sound",          // PDF 1.2
    "Movie",          // PDF 1.2
    "Widget",         // PDF 1.2  // - supported
    "Screen",         // PDF 1.5
    "PrinterMark",    // PDF 1.4
    "TrapNet",        // PDF 1.3
    "Watermark",      // PDF 1.6
    "3D",             // PDF 1.6
    "RichMedia",      // PDF 1.7 ADBE ExtensionLevel 3 ALX: Petr P. Petrov
    "WebMedia",       // PDF 1.7 IPDF ExtensionLevel 1
};

static PdfName GetAppearanceName(PdfAnnotationAppearance appearance);

PdfAnnotation::PdfAnnotation(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect)
    : PdfDictionaryElement(page.GetDocument(), "Annot"), m_AnnotationType(annotType), m_Page(&page)
{
    const PdfName name(utls::TypeNameForIndex((unsigned)annotType, s_names, (unsigned)std::size(s_names)));

    if (name.IsNull())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfArray arr;
    rect.ToArray(arr);

    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, name);
    this->GetObject().GetDictionary().AddKey(PdfName::KeyRect, arr);
    this->GetObject().GetDictionary().AddKey("P", page.GetObject().GetIndirectReference());
}

PdfAnnotation::PdfAnnotation(PdfPage& page, PdfObject& obj)
    : PdfDictionaryElement(obj), m_AnnotationType(PdfAnnotationType::Unknown), m_Page(&page)
{
    m_AnnotationType = static_cast<PdfAnnotationType>(utls::TypeNameToIndex(
        this->GetObject().GetDictionary().FindKeyAs<PdfName>(PdfName::KeySubtype).GetString().c_str(),
        s_names, (unsigned)std::size(s_names), (int)PdfAnnotationType::Unknown));
}

PdfAnnotation::~PdfAnnotation() { }

PdfRect PdfAnnotation::GetRect() const
{
    if (this->GetObject().GetDictionary().HasKey(PdfName::KeyRect))
        return PdfRect(this->GetObject().GetDictionary().MustFindKey(PdfName::KeyRect).GetArray());

    return PdfRect();
}

void PdfAnnotation::SetRect(const PdfRect& rect)
{
    PdfArray arr;
    rect.ToArray(arr);
    this->GetObject().GetDictionary().AddKey(PdfName::KeyRect, arr);
}

void mm::SetAppearanceStreamForObject(PdfObject& obj, PdfXObjectForm& xobj, PdfAnnotationAppearance appearance, const PdfName& state)
{
    // Setting an object as appearance stream requires some resources to be created
    xobj.EnsureResourcesCreated();

    PdfDictionary dict;
    PdfDictionary internal;
    PdfName name;

    if (appearance == PdfAnnotationAppearance::Rollover)
    {
        name = "R";
    }
    else if (appearance == PdfAnnotationAppearance::Down)
    {
        name = "D";
    }
    else // PdfAnnotationAppearance::Normal
    {
        name = "N";
    }

    if (obj.GetDictionary().HasKey("AP"))
    {
        PdfObject* objAP = obj.GetDictionary().GetKey("AP");
        if (objAP->GetDataType() == PdfDataType::Reference)
        {
            auto document = objAP->GetDocument();
            if (document == nullptr)
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

            objAP = document->GetObjects().GetObject(objAP->GetReference());
            if (objAP == nullptr)
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
        }

        if (objAP->GetDataType() != PdfDataType::Dictionary)
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

        if (state.IsNull())
        {
            // allow overwrite only reference by a reference
            if (objAP->GetDictionary().HasKey(name) && objAP->GetDictionary().GetKey(name)->GetDataType() != PdfDataType::Reference)
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

            objAP->GetDictionary().AddKey(name, xobj.GetObject().GetIndirectReference());
        }
        else
        {
            // when the state is defined, then the appearance is expected to be a dictionary
            if (objAP->GetDictionary().HasKey(name) && objAP->GetDictionary().GetKey(name)->GetDataType() != PdfDataType::Dictionary)
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

            if (objAP->GetDictionary().HasKey(name))
            {
                objAP->GetDictionary().GetKey(name)->GetDictionary().AddKey(state, xobj.GetObject().GetIndirectReference());
            }
            else
            {
                internal.AddKey(state, xobj.GetObject().GetIndirectReference());
                objAP->GetDictionary().AddKey(name, internal);
            }
        }
    }
    else
    {
        if (state.IsNull())
        {
            dict.AddKey(name, xobj.GetObject().GetIndirectReference());
            obj.GetDictionary().AddKey("AP", dict);
        }
        else
        {
            internal.AddKey(state, xobj.GetObject().GetIndirectReference());
            dict.AddKey(name, internal);
            obj.GetDictionary().AddKey("AP", dict);
        }
    }

    if (!state.IsNull())
    {
        if (!obj.GetDictionary().HasKey("AS"))
            obj.GetDictionary().AddKey("AS", state);
    }
}

void PdfAnnotation::SetAppearanceStream(PdfXObjectForm& obj, PdfAnnotationAppearance appearance, const PdfName& state)
{
    SetAppearanceStreamForObject(this->GetObject(), obj, appearance, state);
}

bool PdfAnnotation::HasAppearanceStream() const
{
    return this->GetObject().GetDictionary().HasKey("AP");
}

PdfObject* PdfAnnotation::GetAppearanceDictionary()
{
    return GetObject().GetDictionary().FindKey("AP");
}

PdfObject* PdfAnnotation::GetAppearanceStream(PdfAnnotationAppearance appearance, const PdfName& state)
{
    PdfObject* apObj = GetAppearanceDictionary();
    if (apObj == nullptr)
        return nullptr;

    PdfName apName = GetAppearanceName(appearance);
    PdfObject* apObjInn = apObj->GetDictionary().FindKey(apName);
    if (apObjInn == nullptr)
        return nullptr;

    if (state.IsNull())
        return apObjInn;

    return apObjInn->GetDictionary().FindKey(state);
}

void PdfAnnotation::SetFlags(PdfAnnotationFlags uiFlags)
{
    this->GetObject().GetDictionary().AddKey("F", PdfVariant(static_cast<int64_t>(uiFlags)));
}

PdfAnnotationFlags PdfAnnotation::GetFlags() const
{
    if (this->GetObject().GetDictionary().HasKey("F"))
        return static_cast<PdfAnnotationFlags>(this->GetObject().GetDictionary().MustFindKey("F").GetNumber());

    return PdfAnnotationFlags::None;
}

void PdfAnnotation::SetBorderStyle(double dHCorner, double dVCorner, double width)
{
    this->SetBorderStyle(dHCorner, dVCorner, width, PdfArray());
}

void PdfAnnotation::SetBorderStyle(double dHCorner, double dVCorner, double width, const PdfArray& strokeStyle)
{
    // TODO : Support for Border style for PDF Vers > 1.0
    PdfArray values;

    values.Add(dHCorner);
    values.Add(dVCorner);
    values.Add(width);
    if (strokeStyle.size() != 0)
        values.Add(strokeStyle);

    this->GetObject().GetDictionary().AddKey("Border", values);
}

void PdfAnnotation::SetTitle(const PdfString& title)
{
    this->GetObject().GetDictionary().AddKey("T", title);
}

nullable<PdfString> PdfAnnotation::GetTitle() const
{
    if (this->GetObject().GetDictionary().HasKey("T"))
        return this->GetObject().GetDictionary().MustFindKey("T").GetString();

    return { };
}

void PdfAnnotation::SetContents(const PdfString& contents)
{
    this->GetObject().GetDictionary().AddKey("Contents", contents);
}

nullable<PdfString> PdfAnnotation::GetContents() const
{
    if (this->GetObject().GetDictionary().HasKey("Contents"))
        return this->GetObject().GetDictionary().MustFindKey("Contents").GetString();

    return { };
}

void PdfAnnotation::SetDestination(const shared_ptr<PdfDestination>& destination)
{
    destination->AddToDictionary(this->GetObject().GetDictionary());
    m_Destination = destination;
}

shared_ptr<PdfDestination> PdfAnnotation::GetDestination() const
{
    return const_cast<PdfAnnotation&>(*this).getDestination();
}

shared_ptr<PdfDestination> PdfAnnotation::getDestination()
{
    if (m_Destination == nullptr)
    {
        auto obj = GetObject().GetDictionary().FindKey("Dest");
        if (obj == nullptr)
            return nullptr;

        m_Destination.reset(new PdfDestination(*obj));
    }

    return m_Destination;
}

bool PdfAnnotation::HasDestination() const
{
    return this->GetObject().GetDictionary().HasKey("Dest");
}

void PdfAnnotation::SetAction(const shared_ptr<PdfAction>& action)
{
    this->GetObject().GetDictionary().AddKey("A", action->GetObject().GetIndirectReference());
    m_Action = action;
}

shared_ptr<PdfAction> PdfAnnotation::GetAction() const
{
    return const_cast<PdfAnnotation&>(*this).getAction();
}

shared_ptr<PdfAction> PdfAnnotation::getAction()
{
    if (m_Action == nullptr)
    {
        auto obj = this->GetObject().GetDictionary().FindKey("A");
        if (obj == nullptr)
            return nullptr;

        m_Action.reset(new PdfAction(*obj));
    }

    return m_Action;
}

bool PdfAnnotation::HasAction() const
{
    return this->GetObject().GetDictionary().HasKey("A");
}

void PdfAnnotation::SetOpen(bool b)
{
    this->GetObject().GetDictionary().AddKey("Open", b);
}

bool PdfAnnotation::GetOpen() const
{
    return this->GetObject().GetDictionary().GetKeyAs<bool>("Open", false);
}

bool PdfAnnotation::HasFileAttachement() const
{
    return this->GetObject().GetDictionary().HasKey("FS");
}

void PdfAnnotation::SetFileAttachement(const shared_ptr<PdfFileSpec>& fileSpec)
{
    this->GetObject().GetDictionary().AddKey("FS", fileSpec->GetObject().GetIndirectReference());
    m_FileSpec = fileSpec;
}

shared_ptr<PdfFileSpec> PdfAnnotation::GetFileAttachement() const
{
    return const_cast<PdfAnnotation&>(*this).getFileAttachment();
}

shared_ptr<PdfFileSpec> PdfAnnotation::getFileAttachment()
{
    if (m_FileSpec == nullptr)
    {
        auto obj = this->GetObject().GetDictionary().FindKey("FS");
        if (obj == nullptr)
            return nullptr;

        m_FileSpec.reset(new PdfFileSpec(*obj));
    }

    return m_FileSpec;
}

PdfArray PdfAnnotation::GetQuadPoints() const
{
    if (this->GetObject().GetDictionary().HasKey("QuadPoints"))
        return this->GetObject().GetDictionary().MustFindKey("QuadPoints").GetArray();

    return PdfArray();
}

void PdfAnnotation::SetQuadPoints(const PdfArray& quadPoints)
{
    if (m_AnnotationType != PdfAnnotationType::Highlight &&
        m_AnnotationType != PdfAnnotationType::Underline &&
        m_AnnotationType != PdfAnnotationType::Squiggly &&
        m_AnnotationType != PdfAnnotationType::StrikeOut)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Must be a text markup annotation (highlight, underline, squiggly or strikeout) to set quad points");

    this->GetObject().GetDictionary().AddKey("QuadPoints", quadPoints);
}

PdfArray PdfAnnotation::GetColor() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return PdfArray(this->GetObject().GetDictionary().MustFindKey("C").GetArray());

    return PdfArray();
}

void PdfAnnotation::SetColor(double r, double g, double b)
{
    PdfArray c;
    c.Add(PdfObject(r));
    c.Add(PdfObject(g));
    c.Add(PdfObject(b));
    this->GetObject().GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor(double C, double M, double Y, double K)
{
    PdfArray c;
    c.Add(PdfObject(C));
    c.Add(PdfObject(M));
    c.Add(PdfObject(Y));
    c.Add(PdfObject(K));
    this->GetObject().GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor(double gray)
{
    PdfArray c;
    c.Add(PdfVariant(gray));
    this->GetObject().GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor()
{
    PdfArray c;
    this->GetObject().GetDictionary().AddKey("C", c);
}

PdfName GetAppearanceName(PdfAnnotationAppearance appearance)
{
    switch (appearance)
    {
        case mm::PdfAnnotationAppearance::Normal:
            return PdfName("N");
        case mm::PdfAnnotationAppearance::Rollover:
            return PdfName("R");
        case mm::PdfAnnotationAppearance::Down:
            return PdfName("D");
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid appearance type");
    }
}
