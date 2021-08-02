/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfAnnotation.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfDate.h"

#include "PdfAction.h"
#include "PdfFileSpec.h"
#include "PdfPage.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"
#include "PdfXObject.h"

using namespace std;
using namespace PoDoFo;

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

static PdfName GetAppearanceName( PdfAnnotationAppearance eAppearance );

PdfAnnotation::PdfAnnotation(PdfPage& page, PdfAnnotationType eAnnot, const PdfRect& rRect)
    : PdfElement(page.GetDocument(), "Annot"), m_eAnnotation(eAnnot), m_pPage(&page)
{
    PdfVariant rect;
    PdfDate date;

    const PdfName name(TypeNameForIndex((unsigned)eAnnot, s_names, std::size(s_names)));

    if (!name.GetLength())
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    rRect.ToVariant(rect);
    PdfString sDate = date.ToString();

    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, name);
    this->GetObject().GetDictionary().AddKey(PdfName::KeyRect, rect);
    this->GetObject().GetDictionary().AddKey("P", page.GetObject().GetIndirectReference());
    this->GetObject().GetDictionary().AddKey("M", sDate);
}

PdfAnnotation::PdfAnnotation(PdfPage& page, PdfObject& obj)
    : PdfElement(obj), m_eAnnotation(PdfAnnotationType::Unknown), m_pPage(&page)
{
    m_eAnnotation = static_cast<PdfAnnotationType>(TypeNameToIndex(
        this->GetObject().GetDictionary().GetKeyAsName(PdfName::KeySubtype).GetString().c_str(),
        s_names, std::size(s_names), (int)PdfAnnotationType::Unknown));
}

PdfAnnotation::~PdfAnnotation() { }

PdfRect PdfAnnotation::GetRect() const
{
    if (this->GetObject().GetDictionary().HasKey(PdfName::KeyRect))
        return PdfRect(this->GetObject().GetDictionary().GetKey(PdfName::KeyRect)->GetArray());

    return PdfRect();
}

void PdfAnnotation::SetRect(const PdfRect& rRect)
{
    PdfVariant rect;
    rRect.ToVariant(rect);
    this->GetObject().GetDictionary().AddKey(PdfName::KeyRect, rect);
}

void PoDoFo::SetAppearanceStreamForObject(PdfObject& forObject, PdfXObject& xobj, PdfAnnotationAppearance eAppearance, const PdfName& state)
{
    // Setting an object as appearance stream requires some resources to be created
    xobj.EnsureResourcesInitialized();

    PdfDictionary dict;
    PdfDictionary internal;
    PdfName name;

    if (eAppearance == PdfAnnotationAppearance::Rollover)
    {
        name = "R";
    }
    else if (eAppearance == PdfAnnotationAppearance::Down)
    {
        name = "D";
    }
    else // PdfAnnotationAppearance::Normal
    {
        name = "N";
    }

    if (forObject.GetDictionary().HasKey("AP"))
    {
        PdfObject* objAP = forObject.GetDictionary().GetKey("AP");
        if (objAP->GetDataType() == EPdfDataType::Reference)
        {
            auto document = objAP->GetDocument();
            if (document == nullptr)
                PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

            objAP = document->GetObjects().GetObject(objAP->GetReference());
            if (objAP == nullptr)
                PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
        }

        if (objAP->GetDataType() != EPdfDataType::Dictionary)
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

        if (state.GetLength() == 0)
        {
            // allow overwrite only reference by a reference
            if (objAP->GetDictionary().HasKey(name) && objAP->GetDictionary().GetKey(name)->GetDataType() != EPdfDataType::Reference)
                PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

            objAP->GetDictionary().AddKey(name, xobj.GetObject().GetIndirectReference());
        }
        else
        {
            // when the state is defined, then the appearance is expected to be a dictionary
            if (objAP->GetDictionary().HasKey(name) && objAP->GetDictionary().GetKey(name)->GetDataType() != EPdfDataType::Dictionary)
                PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

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
        if (state.GetLength() == 0)
        {
            dict.AddKey(name, xobj.GetObject().GetIndirectReference());
            forObject.GetDictionary().AddKey("AP", dict);
        }
        else
        {
            internal.AddKey(state, xobj.GetObject().GetIndirectReference());
            dict.AddKey(name, internal);
            forObject.GetDictionary().AddKey("AP", dict);
        }
    }

    if (state.GetLength() != 0)
    {
        if (!forObject.GetDictionary().HasKey("AS"))
            forObject.GetDictionary().AddKey("AS", state);
    }
}

void PdfAnnotation::SetAppearanceStream(PdfXObject& pObject, PdfAnnotationAppearance eAppearance, const PdfName& state)
{
    SetAppearanceStreamForObject(this->GetObject(), pObject, eAppearance, state);
}

bool PdfAnnotation::HasAppearanceStream() const
{
    return this->GetObject().GetDictionary().HasKey("AP");
}

PdfObject* PdfAnnotation::GetAppearanceDictionary()
{
    return GetObject().GetDictionary().FindKey("AP");
}

PdfObject* PdfAnnotation::GetAppearanceStream(PdfAnnotationAppearance eAppearance, const PdfName& state)
{
    PdfObject* apObj = GetAppearanceDictionary();
    if (apObj == nullptr)
        return nullptr;

    PdfName apName = GetAppearanceName(eAppearance);
    PdfObject* apObjInn = apObj->GetDictionary().FindKey(apName);
    if (apObjInn == nullptr)
        return nullptr;

    if (state.GetLength() == 0)
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
        return static_cast<PdfAnnotationFlags>(this->GetObject().GetDictionary().GetKey("F")->GetNumber());

    return PdfAnnotationFlags::None;
}

void PdfAnnotation::SetBorderStyle(double dHCorner, double dVCorner, double dWidth)
{
    this->SetBorderStyle(dHCorner, dVCorner, dWidth, PdfArray());
}

void PdfAnnotation::SetBorderStyle(double dHCorner, double dVCorner, double dWidth, const PdfArray& rStrokeStyle)
{
    // TODO : Support for Border style for PDF Vers > 1.0
    PdfArray aValues;

    aValues.push_back(dHCorner);
    aValues.push_back(dVCorner);
    aValues.push_back(dWidth);
    if (rStrokeStyle.size())
        aValues.push_back(rStrokeStyle);

    this->GetObject().GetDictionary().AddKey("Border", aValues);
}

void PdfAnnotation::SetTitle(const PdfString& sTitle)
{
    this->GetObject().GetDictionary().AddKey("T", sTitle);
}

optional<PdfString> PdfAnnotation::GetTitle() const
{
    if (this->GetObject().GetDictionary().HasKey("T"))
        return this->GetObject().GetDictionary().GetKey("T")->GetString();

    return { };
}

void PdfAnnotation::SetContents(const PdfString& sContents)
{
    this->GetObject().GetDictionary().AddKey("Contents", sContents);
}

optional<PdfString> PdfAnnotation::GetContents() const
{
    if (this->GetObject().GetDictionary().HasKey("Contents"))
        return this->GetObject().GetDictionary().GetKey("Contents")->GetString();

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
    if (this->GetObject().GetDictionary().HasKey("Open"))
        return this->GetObject().GetDictionary().GetKey("Open")->GetBool();

    return false;
}

bool PdfAnnotation::HasFileAttachement() const
{
    return this->GetObject().GetDictionary().HasKey("FS");
}

void PdfAnnotation::SetFileAttachement(const shared_ptr<PdfFileSpec>& fileSpec)
{
    this->GetObject().GetDictionary().AddKey("FS", fileSpec->GetObject().GetIndirectReference());
    m_pFileSpec = fileSpec;
}

shared_ptr<PdfFileSpec> PdfAnnotation::GetFileAttachement() const
{
    return const_cast<PdfAnnotation&>(*this).getFileAttachment();
}

shared_ptr<PdfFileSpec> PdfAnnotation::getFileAttachment()
{
    if (m_pFileSpec == nullptr)
    {
        auto obj = this->GetObject().GetDictionary().FindKey("FS");
        if (obj == nullptr)
            return nullptr;

        m_pFileSpec.reset(new PdfFileSpec(*obj));
    }

    return m_pFileSpec;
}

PdfArray PdfAnnotation::GetQuadPoints() const
{
    if (this->GetObject().GetDictionary().HasKey("QuadPoints"))
        return this->GetObject().GetDictionary().GetKey("QuadPoints")->GetArray();

    return PdfArray();
}

void PdfAnnotation::SetQuadPoints(const PdfArray& rQuadPoints)
{
    if (m_eAnnotation != PdfAnnotationType::Highlight &&
        m_eAnnotation != PdfAnnotationType::Underline &&
        m_eAnnotation != PdfAnnotationType::Squiggly &&
        m_eAnnotation != PdfAnnotationType::StrikeOut)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Must be a text markup annotation (highlight, underline, squiggly or strikeout) to set quad points");

    this->GetObject().GetDictionary().AddKey("QuadPoints", rQuadPoints);
}

PdfArray PdfAnnotation::GetColor() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return PdfArray(this->GetObject().GetDictionary().GetKey("C")->GetArray());

    return PdfArray();
}

void PdfAnnotation::SetColor(double r, double g, double b)
{
    PdfArray c;
    c.push_back(PdfVariant(r));
    c.push_back(PdfVariant(g));
    c.push_back(PdfVariant(b));
    this->GetObject().GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor(double C, double M, double Y, double K)
{
    PdfArray c;
    c.push_back(PdfVariant(C));
    c.push_back(PdfVariant(M));
    c.push_back(PdfVariant(Y));
    c.push_back(PdfVariant(K));
    this->GetObject().GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor(double gray)
{
    PdfArray c;
    c.push_back(PdfVariant(gray));
    this->GetObject().GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor()
{
    PdfArray c;
    this->GetObject().GetDictionary().AddKey("C", c);
}

PdfName GetAppearanceName(PdfAnnotationAppearance eAppearance)
{
    switch (eAppearance)
    {
        case PoDoFo::PdfAnnotationAppearance::Normal:
            return PdfName("N");
        case PoDoFo::PdfAnnotationAppearance::Rollover:
            return PdfName("R");
        case PoDoFo::PdfAnnotationAppearance::Down:
            return PdfName("D");
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Invalid appearance type");
    }
}
