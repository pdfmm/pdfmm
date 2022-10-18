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
#include "PdfAnnotationWidget.h"
#include "PdfAnnotation_Types.h"

using namespace std;
using namespace mm;

static string_view toString(PdfAnnotationType type);
static PdfAnnotationType fromString(const string_view& str);

static PdfName GetAppearanceName(PdfAppearanceType appearance);

PdfAnnotation::PdfAnnotation(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect)
    : PdfDictionaryElement(page.GetDocument(), "Annot"), m_AnnotationType(annotType), m_Page(&page)
{
    const PdfName name(toString(annotType));

    if (name.IsNull())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfArray arr;
    rect.ToArray(arr);

    GetDictionary().AddKey(PdfName::KeySubtype, name);
    GetDictionary().AddKey(PdfName::KeyRect, arr);
    GetDictionary().AddKey("P", page.GetObject().GetIndirectReference());

    // Default set print flag
    auto flags = GetFlags();
    SetFlags(flags | PdfAnnotationFlags::Print);
}

PdfAnnotation::PdfAnnotation(PdfObject& obj, PdfAnnotationType annotType)
    : PdfDictionaryElement(obj), m_AnnotationType(annotType), m_Page(nullptr)
{
}

PdfRect PdfAnnotation::GetRect() const
{
    if (GetDictionary().HasKey(PdfName::KeyRect))
        return PdfRect(GetDictionary().MustFindKey(PdfName::KeyRect).GetArray());

    return PdfRect();
}

void PdfAnnotation::SetRect(const PdfRect& rect)
{
    PdfArray arr;
    rect.ToArray(arr);
    this->GetObject().GetDictionary().AddKey(PdfName::KeyRect, arr);
}

void mm::SetAppearanceStreamForObject(PdfObject& obj, PdfXObjectForm& xobj,
    PdfAppearanceType appearance, const PdfName& state)
{
    // Setting an object as appearance stream requires some resources to be created
    xobj.EnsureResourcesCreated();

    PdfName name;
    if (appearance == PdfAppearanceType::Rollover)
        name = "R";
    else if (appearance == PdfAppearanceType::Down)
        name = "D";
    else // PdfAnnotationAppearance::Normal
        name = "N";

    auto apObj = obj.GetDictionary().FindKey("AP");
    if (apObj == nullptr || !apObj->IsDictionary())
        apObj = &obj.GetDictionary().AddKey("AP", PdfDictionary());

    if (state.IsNull())
    {
        apObj->GetDictionary().AddKeyIndirectSafe(name, xobj.GetObject());
    }
    else
    {
        // when the state is defined, then the appearance is expected to be a dictionary
        auto apInnerObj = apObj->GetDictionary().FindKey(name);
        if (apInnerObj == nullptr || !apInnerObj->IsDictionary())
            apInnerObj = &apObj->GetDictionary().AddKey(name, PdfDictionary());

        apInnerObj->GetDictionary().AddKeyIndirectSafe(state, xobj.GetObject());
    }

    if (!state.IsNull())
    {
        if (!obj.GetDictionary().HasKey("AS"))
            obj.GetDictionary().AddKey("AS", state);
    }
}

unique_ptr<PdfAnnotation> PdfAnnotation::Create(PdfPage& page, const type_info& typeInfo, const PdfRect& rect)
{
    return Create(page, getAnnotationType(typeInfo), rect);
}

unique_ptr<PdfAnnotation> PdfAnnotation::Create(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect)
{
    switch (annotType)
    {
        case PdfAnnotationType::Text:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationText(page, rect));
        case PdfAnnotationType::Link:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationLink(page, rect));
        case PdfAnnotationType::FreeText:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationFreeText(page, rect));
        case PdfAnnotationType::Line:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationLine(page, rect));
        case PdfAnnotationType::Square:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationSquare(page, rect));
        case PdfAnnotationType::Circle:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationCircle(page, rect));
        case PdfAnnotationType::Polygon:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPolygon(page, rect));
        case PdfAnnotationType::PolyLine:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPolyLine(page, rect));
        case PdfAnnotationType::Highlight:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationHighlight(page, rect));
        case PdfAnnotationType::Underline:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationUnderline(page, rect));
        case PdfAnnotationType::Squiggly:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationSquiggly(page, rect));
        case PdfAnnotationType::StrikeOut:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationStrikeOut(page, rect));
        case PdfAnnotationType::Stamp:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationStamp(page, rect));
        case PdfAnnotationType::Caret:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationCaret(page, rect));
        case PdfAnnotationType::Ink:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationInk(page, rect));
        case PdfAnnotationType::Popup:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPopup(page, rect));
        case PdfAnnotationType::FileAttachement:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationFileAttachement(page, rect));
        case PdfAnnotationType::Sound:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationSound(page, rect));
        case PdfAnnotationType::Movie:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationMovie(page, rect));
        case PdfAnnotationType::Widget:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationWidget(page, rect));
        case PdfAnnotationType::Screen:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationScreen(page, rect));
        case PdfAnnotationType::PrinterMark:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationPrinterMark(page, rect));
        case PdfAnnotationType::TrapNet:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationTrapNet(page, rect));
        case PdfAnnotationType::Watermark:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationWatermark(page, rect));
        case PdfAnnotationType::Model3D:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationModel3D(page, rect));
        case PdfAnnotationType::RichMedia:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationRichMedia(page, rect));
        case PdfAnnotationType::WebMedia:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationWebMedia(page, rect));
        case PdfAnnotationType::Redact:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationRedact(page, rect));
        case PdfAnnotationType::Projection:
            return unique_ptr<PdfAnnotation>(new PdfAnnotationProjection(page, rect));
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void PdfAnnotation::SetAppearanceStream(PdfXObjectForm& obj, PdfAppearanceType appearance, const PdfName& state)
{
    SetAppearanceStreamForObject(this->GetObject(), obj, appearance, state);
}

void PdfAnnotation::GetAppearanceStreams(vector<PdfAppearanceIdentity>& streams) const
{
    streams.clear();
    auto apDict = getAppearanceDictionary();
    if (apDict == nullptr)
        return;

    PdfReference reference;
    for (auto& pair1 : apDict->GetIndirectIterator())
    {
        PdfAppearanceType apType;
        if (pair1.first == "R")
            apType = PdfAppearanceType::Rollover;
        else if (pair1.first == "D")
            apType = PdfAppearanceType::Down;
        else if (pair1.first == "N")
            apType = PdfAppearanceType::Normal;
        else
            continue;

        PdfDictionary* apStateDict;
        if ( pair1.second->HasStream())
        {
            streams.push_back({ pair1.second, apType, PdfName() });
        }
        else if (pair1.second->TryGetDictionary(apStateDict))
        {
            for (auto& pair2 : apStateDict->GetIndirectIterator())
            {
                if (pair2.second->HasStream())
                    streams.push_back({ pair2.second, apType, pair2.first });
            }
        }
    }
}

PdfObject* PdfAnnotation::GetAppearanceDictionaryObject()
{
    return GetDictionary().FindKey("AP");
}

const PdfObject* PdfAnnotation::GetAppearanceDictionaryObject() const
{
    return GetDictionary().FindKey("AP");
}

PdfObject* PdfAnnotation::GetAppearanceStream(PdfAppearanceType appearance, const PdfName& state)
{
    return getAppearanceStream(appearance, state);
}

const PdfObject* PdfAnnotation::GetAppearanceStream(PdfAppearanceType appearance, const PdfName& state) const
{
    return getAppearanceStream(appearance, state);
}

PdfObject* PdfAnnotation::getAppearanceStream(PdfAppearanceType appearance, const PdfName& state) const
{
    auto apDict = getAppearanceDictionary();
    if (apDict == nullptr)
        return nullptr;

    PdfName apName = GetAppearanceName(appearance);
    PdfObject* apObjInn = apDict->FindKey(apName);
    if (apObjInn == nullptr)
        return nullptr;

    if (state.IsNull())
        return apObjInn;

    return apObjInn->GetDictionary().FindKey(state);
}

PdfDictionary* PdfAnnotation::getAppearanceDictionary() const
{
    auto apObj = const_cast<PdfAnnotation&>(*this).GetAppearanceDictionaryObject();
    if (apObj == nullptr)
        return nullptr;

    PdfDictionary* apDict;
    (void)apObj->TryGetDictionary(apDict);
    return apDict;
}

void PdfAnnotation::SetFlags(PdfAnnotationFlags uiFlags)
{
    GetDictionary().AddKey("F", PdfVariant(static_cast<int64_t>(uiFlags)));
}

PdfAnnotationFlags PdfAnnotation::GetFlags() const
{
    if (GetDictionary().HasKey("F"))
        return static_cast<PdfAnnotationFlags>(GetDictionary().MustFindKey("F").GetNumber());

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
    if (GetDictionary().HasKey("T"))
        return GetDictionary().MustFindKey("T").GetString();

    return { };
}

void PdfAnnotation::SetContents(const PdfString& contents)
{
    GetDictionary().AddKey("Contents", contents);
}

nullable<PdfString> PdfAnnotation::GetContents() const
{
    if (GetDictionary().HasKey("Contents"))
        return GetDictionary().MustFindKey("Contents").GetString();

    return { };
}

PdfArray PdfAnnotation::GetColor() const
{
    if (GetDictionary().HasKey("C"))
        return PdfArray(GetDictionary().MustFindKey("C").GetArray());

    return PdfArray();
}

void PdfAnnotation::SetColor(double r, double g, double b)
{
    PdfArray c;
    c.Add(PdfObject(r));
    c.Add(PdfObject(g));
    c.Add(PdfObject(b));
    GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor(double C, double M, double Y, double K)
{
    PdfArray c;
    c.Add(PdfObject(C));
    c.Add(PdfObject(M));
    c.Add(PdfObject(Y));
    c.Add(PdfObject(K));
    GetDictionary().AddKey("C", c);
}

void PdfAnnotation::SetColor(double gray)
{
    PdfArray c;
    c.Add(PdfVariant(gray));
    GetDictionary().AddKey("C", c);
}

void PdfAnnotation::ResetColor()
{
    PdfArray c;
    GetDictionary().AddKey("C", c);
}

PdfName GetAppearanceName(PdfAppearanceType appearance)
{
    switch (appearance)
    {
        case PdfAppearanceType::Normal:
            return PdfName("N");
        case PdfAppearanceType::Rollover:
            return PdfName("R");
        case PdfAppearanceType::Down:
            return PdfName("D");
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid appearance type");
    }
}

bool PdfAnnotation::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, PdfAnnotationType::Unknown, xobj_))
        return false;

    xobj.reset(xobj_);
    return true;
}

bool PdfAnnotation::TryCreateFromObject(const PdfObject& obj, unique_ptr<const PdfAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, PdfAnnotationType::Unknown, xobj_))
        return false;

    xobj.reset(xobj_);
    return true;
}

bool PdfAnnotation::tryCreateFromObject(const PdfObject& obj, const type_info& typeInfo, PdfAnnotation*& xobj)
{
    return tryCreateFromObject(obj, getAnnotationType(typeInfo), xobj);
}

bool PdfAnnotation::tryCreateFromObject(const PdfObject& obj, PdfAnnotationType targetType, PdfAnnotation*& xobj)
{
    auto type = getAnnotationType(obj);
    if (targetType != PdfAnnotationType::Unknown && type != targetType)
    {
        xobj = nullptr;
        return false;
    }

    switch (type)
    {
        case PdfAnnotationType::Text:
            xobj = new PdfAnnotationText(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Link:
            xobj = new PdfAnnotationLink(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::FreeText:
            xobj = new PdfAnnotationFreeText(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Line:
            xobj = new PdfAnnotationLine(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Square:
            xobj = new PdfAnnotationSquare(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Circle:
            xobj = new PdfAnnotationCircle(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Polygon:
            xobj = new PdfAnnotationPolygon(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::PolyLine:
            xobj = new PdfAnnotationPolyLine(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Highlight:
            xobj = new PdfAnnotationHighlight(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Underline:
            xobj = new PdfAnnotationUnderline(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Squiggly:
            xobj = new PdfAnnotationSquiggly(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::StrikeOut:
            xobj = new PdfAnnotationStrikeOut(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Stamp:
            xobj = new PdfAnnotationStamp(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Caret:
            xobj = new PdfAnnotationCaret(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Ink:
            xobj = new PdfAnnotationInk(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Popup:
            xobj = new PdfAnnotationPopup(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::FileAttachement:
            xobj = new PdfAnnotationFileAttachement(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Sound:
            xobj = new PdfAnnotationSound(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Movie:
            xobj = new PdfAnnotationMovie(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Widget:
            xobj = new PdfAnnotationWidget(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Screen:
            xobj = new PdfAnnotationScreen(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::PrinterMark:
            xobj = new PdfAnnotationPrinterMark(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::TrapNet:
            xobj = new PdfAnnotationTrapNet(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Watermark:
            xobj = new PdfAnnotationWatermark(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Model3D:
            xobj = new PdfAnnotationModel3D(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::RichMedia:
            xobj = new PdfAnnotationRichMedia(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::WebMedia:
            xobj = new PdfAnnotationWebMedia(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Redact:
            xobj = new PdfAnnotationRedact(const_cast<PdfObject&>(obj));
            return true;
        case PdfAnnotationType::Projection:
            xobj = new PdfAnnotationProjection(const_cast<PdfObject&>(obj));
            return true;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfAnnotationType PdfAnnotation::getAnnotationType(const type_info& typeInfo)
{
    if (typeInfo == typeid(PdfAnnotationText))
        return PdfAnnotationType::Text;
    if (typeInfo == typeid(PdfAnnotationLink))
        return PdfAnnotationType::Link;
    else if (typeInfo == typeid(PdfAnnotationFreeText))
        return PdfAnnotationType::FreeText;
    else if (typeInfo == typeid(PdfAnnotationLine))
        return PdfAnnotationType::Line;
    else if (typeInfo == typeid(PdfAnnotationSquare))
        return PdfAnnotationType::Square;
    else if (typeInfo == typeid(PdfAnnotationCircle))
        return PdfAnnotationType::Circle;
    else if (typeInfo == typeid(PdfAnnotationPolygon))
        return PdfAnnotationType::Polygon;
    else if (typeInfo == typeid(PdfAnnotationPolyLine))
        return PdfAnnotationType::PolyLine;
    else if (typeInfo == typeid(PdfAnnotationHighlight))
        return PdfAnnotationType::Highlight;
    else if (typeInfo == typeid(PdfAnnotationUnderline))
        return PdfAnnotationType::Underline;
    else if (typeInfo == typeid(PdfAnnotationSquiggly))
        return PdfAnnotationType::Squiggly;
    else if (typeInfo == typeid(PdfAnnotationStrikeOut))
        return PdfAnnotationType::StrikeOut;
    else if (typeInfo == typeid(PdfAnnotationStamp))
        return PdfAnnotationType::Stamp;
    else if (typeInfo == typeid(PdfAnnotationCaret))
        return PdfAnnotationType::Caret;
    else if (typeInfo == typeid(PdfAnnotationInk))
        return PdfAnnotationType::Ink;
    else if (typeInfo == typeid(PdfAnnotationPopup))
        return PdfAnnotationType::Popup;
    else if (typeInfo == typeid(PdfAnnotationFileAttachement))
        return PdfAnnotationType::FileAttachement;
    else if (typeInfo == typeid(PdfAnnotationSound))
        return PdfAnnotationType::Sound;
    else if (typeInfo == typeid(PdfAnnotationMovie))
        return PdfAnnotationType::Movie;
    else if (typeInfo == typeid(PdfAnnotationWidget))
        return PdfAnnotationType::Widget;
    else if (typeInfo == typeid(PdfAnnotationScreen))
        return PdfAnnotationType::Screen;
    else if (typeInfo == typeid(PdfAnnotationPrinterMark))
        return PdfAnnotationType::PrinterMark;
    else if (typeInfo == typeid(PdfAnnotationTrapNet))
        return PdfAnnotationType::TrapNet;
    else if (typeInfo == typeid(PdfAnnotationWatermark))
        return PdfAnnotationType::Watermark;
    else if (typeInfo == typeid(PdfAnnotationModel3D))
        return PdfAnnotationType::Model3D;
    else if (typeInfo == typeid(PdfAnnotationRichMedia))
        return PdfAnnotationType::RichMedia;
    else if (typeInfo == typeid(PdfAnnotationWebMedia))
        return PdfAnnotationType::WebMedia;
    else if (typeInfo == typeid(PdfAnnotationRedact))
        return PdfAnnotationType::Redact;
    else if (typeInfo == typeid(PdfAnnotationProjection))
        return PdfAnnotationType::Projection;
    else
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
}

PdfAnnotationType PdfAnnotation::getAnnotationType(const PdfObject& obj)
{
    const PdfName* name;
    auto subTypeObj = obj.GetDictionary().FindKey(PdfName::KeySubtype);
    if (subTypeObj == nullptr || !subTypeObj->TryGetName(name))
        return PdfAnnotationType::Unknown;

    auto subtype = name->GetString();
    return fromString(subtype);
}

string_view toString(PdfAnnotationType type)
{
    switch (type)
    {
        case PdfAnnotationType::Text:
            return "Text"sv;
        case PdfAnnotationType::Link:
            return "Link"sv;
        case PdfAnnotationType::FreeText:
            return "FreeText"sv;
        case PdfAnnotationType::Line:
            return "Line"sv;
        case PdfAnnotationType::Square:
            return "Square"sv;
        case PdfAnnotationType::Circle:
            return "Circle"sv;
        case PdfAnnotationType::Polygon:
            return "Polygon"sv;
        case PdfAnnotationType::PolyLine:
            return "PolyLine"sv;
        case PdfAnnotationType::Highlight:
            return "Highlight"sv;
        case PdfAnnotationType::Underline:
            return "Underline"sv;
        case PdfAnnotationType::Squiggly:
            return "Squiggly"sv;
        case PdfAnnotationType::StrikeOut:
            return "StrikeOut"sv;
        case PdfAnnotationType::Stamp:
            return "Stamp"sv;
        case PdfAnnotationType::Caret:
            return "Caret"sv;
        case PdfAnnotationType::Ink:
            return "Ink"sv;
        case PdfAnnotationType::Popup:
            return "Popup"sv;
        case PdfAnnotationType::FileAttachement:
            return "FileAttachment"sv;
        case PdfAnnotationType::Sound:
            return "Sound"sv;
        case PdfAnnotationType::Movie:
            return "Movie"sv;
        case PdfAnnotationType::Widget:
            return "Widget"sv;
        case PdfAnnotationType::Screen:
            return "Screen"sv;
        case PdfAnnotationType::PrinterMark:
            return "PrinterMark"sv;
        case PdfAnnotationType::TrapNet:
            return "TrapNet"sv;
        case PdfAnnotationType::Watermark:
            return "Watermark"sv;
        case PdfAnnotationType::Model3D:
            return "3D"sv;
        case PdfAnnotationType::RichMedia:
            return "RichMedia"sv;
        case PdfAnnotationType::WebMedia:
            return "WebMedia"sv;
        case PdfAnnotationType::Redact:
            return "Redact"sv;
        case PdfAnnotationType::Projection:
            return "Projection"sv;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfAnnotationType fromString(const string_view& str)
{
    if (str == "Text"sv)
        return PdfAnnotationType::Text;
    else if (str == "Link"sv)
        return PdfAnnotationType::Link;
    else if (str == "FreeText"sv)
        return PdfAnnotationType::FreeText;
    else if (str == "Line"sv)
        return PdfAnnotationType::Line;
    else if (str == "Square"sv)
        return PdfAnnotationType::Square;
    else if (str == "Circle"sv)
        return PdfAnnotationType::Circle;
    else if (str == "Polygon"sv)
        return PdfAnnotationType::Polygon;
    else if (str == "PolyLine"sv)
        return PdfAnnotationType::PolyLine;
    else if (str == "Highlight"sv)
        return PdfAnnotationType::Highlight;
    else if (str == "Underline"sv)
        return PdfAnnotationType::Underline;
    else if (str == "Squiggly"sv)
        return PdfAnnotationType::Squiggly;
    else if (str == "StrikeOut"sv)
        return PdfAnnotationType::StrikeOut;
    else if (str == "Stamp"sv)
        return PdfAnnotationType::Stamp;
    else if (str == "Caret"sv)
        return PdfAnnotationType::Caret;
    else if (str == "Ink"sv)
        return PdfAnnotationType::Ink;
    else if (str == "Popup"sv)
        return PdfAnnotationType::Popup;
    else if (str == "FileAttachment"sv)
        return PdfAnnotationType::FileAttachement;
    else if (str == "Sound"sv)
        return PdfAnnotationType::Sound;
    else if (str == "Movie"sv)
        return PdfAnnotationType::Movie;
    else if (str == "Widget"sv)
        return PdfAnnotationType::Widget;
    else if (str == "Screen"sv)
        return PdfAnnotationType::Screen;
    else if (str == "PrinterMark"sv)
        return PdfAnnotationType::PrinterMark;
    else if (str == "TrapNet"sv)
        return PdfAnnotationType::TrapNet;
    else if (str == "Watermark"sv)
        return PdfAnnotationType::Watermark;
    else if (str == "3D"sv)
        return PdfAnnotationType::Model3D;
    else if (str == "RichMedia"sv)
        return PdfAnnotationType::RichMedia;
    else if (str == "WebMedia"sv)
        return PdfAnnotationType::WebMedia;
    else if (str == "Redact"sv)
        return PdfAnnotationType::Redact;
    else if (str == "Projection"sv)
        return PdfAnnotationType::Projection;
    else
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
}
