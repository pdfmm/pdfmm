/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfField.h"

#include "PdfAcroForm.h"
#include "PdfDocument.h"
#include "PdfPainter.h"
#include "PdfPage.h"
#include "PdfStreamedDocument.h"
#include "PdfXObject.h"
#include "PdfSignature.h"
#include "PdfPushButton.h"
#include "PdfRadioButton.h"
#include "PdfCheckBox.h"
#include "PdfTextBox.h"
#include "PdfComboBox.h"
#include "PdfListBox.h"

#include <sstream>

using namespace std;
using namespace mm;

void getFullName(const PdfObject* obj, bool escapePartialNames, string& fullname);

PdfField::PdfField(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect)
    : m_Field(fieldType)
{
    m_Widget = page.CreateAnnotation(PdfAnnotationType::Widget, rect);
    m_Object = &m_Widget->GetObject();
    Init(page.GetDocument().GetAcroForm());
}

PdfField::PdfField(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : m_Field(fieldType), m_Object(widget == nullptr ? nullptr : &widget->GetObject()), m_Widget(widget)
{
    auto parent = doc.GetAcroForm();
    if (m_Object == nullptr)
        m_Object = parent->GetDocument().GetObjects().CreateDictionaryObject();

    Init(insertInAcroform ? parent : nullptr);
}

PdfField::PdfField(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect, bool bAppearanceNone)
    : m_Field(fieldType)
{
    m_Widget = page.CreateAnnotation(PdfAnnotationType::Widget, rect);
    m_Object = &m_Widget->GetObject();

    Init(page.GetDocument().GetAcroForm(true,
        bAppearanceNone ?
        PdfAcroFormDefaulAppearance::None
        : PdfAcroFormDefaulAppearance::BlackText12pt));
}

PdfField::PdfField(PdfFieldType fieldType, PdfObject& obj, PdfAnnotation* widget)
    : m_Field(fieldType), m_Object(&obj), m_Widget(widget)
{
}

PdfField* PdfField::CreateField(PdfObject& obj)
{
    return createField(GetFieldType(obj), obj, nullptr);
}

PdfField* PdfField::CreateField(PdfAnnotation& widget)
{
    PdfObject& obj = widget.GetObject();
    return createField(GetFieldType(obj), obj, &widget);
}

PdfField* PdfField::CreateChildField()
{
    return createChildField(nullptr, PdfRect());
}

PdfField* PdfField::CreateChildField(PdfPage& page, const PdfRect& rect)
{
    return createChildField(&page, rect);
}

PdfField* PdfField::createChildField(PdfPage* page, const PdfRect& rect)
{
    PdfFieldType type = GetType();
    auto doc = m_Object->GetDocument();
    PdfField* field;
    PdfObject* childObj;
    if (page == nullptr)
    {
        childObj = doc->GetObjects().CreateDictionaryObject();
        field = createField(type, *childObj, nullptr);
    }
    else
    {
        PdfAnnotation* annot = page->CreateAnnotation(PdfAnnotationType::Widget, rect);
        childObj = &annot->GetObject();
        field = createField(type, *childObj, annot);
    }

    auto& dict = m_Object->GetDictionary();
    auto kids = dict.FindKey("Kids");
    if (kids == nullptr)
        kids = &dict.AddKey("Kids", PdfArray());

    auto& arr = kids->GetArray();
    arr.push_back(childObj->GetIndirectReference());
    childObj->GetDictionary().AddKey("Parent", m_Object->GetIndirectReference());
    return field;
}

PdfField* PdfField::createField(PdfFieldType type, PdfObject& obj, PdfAnnotation* widget)
{
    switch (type)
    {
        case PdfFieldType::Unknown:
            return new PdfField(obj, widget);
        case PdfFieldType::PushButton:
            return new PdfPushButton(obj, widget);
        case PdfFieldType::CheckBox:
            return new PdfCheckBox(obj, widget);
        case PdfFieldType::RadioButton:
            return new PdfRadioButton(obj, widget);
        case PdfFieldType::TextField:
            return new PdfTextBox(obj, widget);
        case PdfFieldType::ComboBox:
            return new PdfComboBox(obj, widget);
        case PdfFieldType::ListBox:
            return new PdfListBox(obj, widget);
        case PdfFieldType::Signature:
            return new PdfSignature(obj, widget);
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

PdfFieldType PdfField::GetFieldType(const PdfObject& obj)
{
    PdfFieldType ret = PdfFieldType::Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    auto ftObj = obj.GetDictionary().FindKeyParent("FT");
    if (ftObj == nullptr)
        return PdfFieldType::Unknown;

    auto& fieldType = ftObj->GetName();
    if (fieldType == "Btn")
    {
        int64_t flags;
        PdfField::GetFieldFlags(obj, flags);

        if ((flags & PdfButton::ePdfButton_PushButton) == PdfButton::ePdfButton_PushButton)
        {
            ret = PdfFieldType::PushButton;
        }
        else if ((flags & PdfButton::ePdfButton_Radio) == PdfButton::ePdfButton_Radio)
        {
            ret = PdfFieldType::RadioButton;
        }
        else
        {
            ret = PdfFieldType::CheckBox;
        }
    }
    else if (fieldType == "Tx")
    {
        ret = PdfFieldType::TextField;
    }
    else if (fieldType == "Ch")
    {
        int64_t flags;
        PdfField::GetFieldFlags(obj, flags);

        if ((flags & PdChoiceField::ePdfListField_Combo) == PdChoiceField::ePdfListField_Combo)
        {
            ret = PdfFieldType::ComboBox;
        }
        else
        {
            ret = PdfFieldType::ListBox;
        }
    }
    else if (fieldType == "Sig")
    {
        ret = PdfFieldType::Signature;
    }

    return ret;
}

void PdfField::Init(PdfAcroForm* parent)
{
    if (parent != nullptr)
    {
        // Insert into the parents kids array
        PdfArray& fields = parent->GetFieldsArray();
        fields.push_back(m_Object->GetIndirectReference());
    }

    PdfDictionary& dict = m_Object->GetDictionary();
    switch (m_Field)
    {
        case PdfFieldType::CheckBox:
            dict.AddKey("FT", PdfName("Btn"));
            break;
        case PdfFieldType::PushButton:
            dict.AddKey("FT", PdfName("Btn"));
            dict.AddKey("Ff", PdfObject((int64_t)PdfButton::ePdfButton_PushButton));
            break;
        case PdfFieldType::RadioButton:
            dict.AddKey("FT", PdfName("Btn"));
            dict.AddKey("Ff", PdfObject((int64_t)(PdfButton::ePdfButton_Radio | PdfButton::ePdfButton_NoToggleOff)));
            break;
        case PdfFieldType::TextField:
            dict.AddKey("FT", PdfName("Tx"));
            break;
        case PdfFieldType::ListBox:
            dict.AddKey("FT", PdfName("Ch"));
            break;
        case PdfFieldType::ComboBox:
            dict.AddKey("FT", PdfName("Ch"));
            dict.AddKey("Ff", PdfObject((int64_t)PdChoiceField::ePdfListField_Combo));
            break;
        case PdfFieldType::Signature:
            dict.AddKey("FT", PdfName("Sig"));
            break;

        case PdfFieldType::Unknown:
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
        }
        break;
    }
}

PdfField::PdfField(PdfObject& obj, PdfAnnotation* widget)
    : m_Field(PdfFieldType::Unknown), m_Object(&obj), m_Widget(widget)
{
    m_Field = GetFieldType(obj);
}

PdfObject* PdfField::GetAppearanceCharacteristics(bool create) const
{
    auto mkObj = m_Object->GetDictionary().FindKey("MK");

    if (mkObj == nullptr && create)
    {
        PdfDictionary dictionary;
        mkObj = &const_cast<PdfField*>(this)->m_Object->GetDictionary().AddKey("MK", dictionary);
    }

    return mkObj;
}

void PdfField::AssertTerminalField() const
{
    if (GetDictionary().HasKey("Kids"))
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "This method can be called only on terminal field. Ensure this field has "
            "not been retrieved from AcroFormFields collection or it's not a parent of terminal fields");
    }
}

void PdfField::SetFieldFlag(int64_t value, bool set)
{
    int64_t curr = 0;

    if (m_Object->GetDictionary().HasKey("Ff"))
        curr = m_Object->GetDictionary().MustFindKey("Ff").GetNumber();

    if (set)
        curr |= value;
    else
    {
        if ((curr & value) == value)
            curr ^= value;
    }

    m_Object->GetDictionary().AddKey("Ff", curr);
}

bool PdfField::GetFieldFlag(int64_t value, bool defvalue) const
{
    int64_t flag;
    if (!GetFieldFlags(*m_Object, flag))
        return defvalue;

    return (flag & value) == value;
}


bool PdfField::GetFieldFlags(const PdfObject& obj, int64_t& value)
{
    auto flagsObject = obj.GetDictionary().FindKeyParent("Ff");
    if (flagsObject == nullptr)
    {
        value = 0;
        return false;
    }

    value = flagsObject->GetNumber();
    return true;
}

void PdfField::SetHighlightingMode(PdfHighlightingMode mode)
{
    PdfName value;

    switch (mode)
    {
        case PdfHighlightingMode::None:
            value = "N";
            break;
        case PdfHighlightingMode::Invert:
            value = "I";
            break;
        case PdfHighlightingMode::InvertOutline:
            value = "O";
            break;
        case PdfHighlightingMode::Push:
            value = "P";
            break;
        case PdfHighlightingMode::Unknown:
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidName);
            break;
    }

    m_Object->GetDictionary().AddKey("H", value);
}

PdfHighlightingMode PdfField::GetHighlightingMode() const
{
    PdfHighlightingMode mode = PdfHighlightingMode::Invert;

    if (m_Object->GetDictionary().HasKey("H"))
    {
        auto& value = m_Object->GetDictionary().MustFindKey("H").GetName();
        if (value == "N")
            return PdfHighlightingMode::None;
        else if (value == "I")
            return PdfHighlightingMode::Invert;
        else if (value == "O")
            return PdfHighlightingMode::InvertOutline;
        else if (value == "P")
            return PdfHighlightingMode::Push;
    }

    return mode;
}

void PdfField::SetBorderColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double gray)
{
    PdfArray array;
    array.push_back(gray);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double red, double green, double blue)
{
    PdfArray array;
    array.push_back(red);
    array.push_back(green);
    array.push_back(blue);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double cyan, double magenta, double yellow, double black)
{
    PdfArray array;
    array.push_back(cyan);
    array.push_back(magenta);
    array.push_back(yellow);
    array.push_back(black);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBackgroundColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double gray)
{
    PdfArray array;
    array.push_back(gray);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double red, double green, double blue)
{
    PdfArray array;
    array.push_back(red);
    array.push_back(green);
    array.push_back(blue);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double cyan, double magenta, double yellow, double black)
{
    PdfArray array;
    array.push_back(cyan);
    array.push_back(magenta);
    array.push_back(yellow);
    array.push_back(black);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetName(const PdfString& name)
{
    m_Object->GetDictionary().AddKey("T", name);
}

optional<PdfString> PdfField::GetName() const
{
    PdfObject* name = m_Object->GetDictionary().FindKeyParent("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

optional<PdfString> PdfField::GetNameRaw() const
{
    PdfObject* name = m_Object->GetDictionary().GetKey("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

string PdfField::GetFullName(bool escapePartialNames) const
{
    string fullName;
    getFullName(m_Object, escapePartialNames, fullName);
    return fullName;
}

void PdfField::SetAlternateName(const PdfString& name)
{
    m_Object->GetDictionary().AddKey("TU", name);
}

optional<PdfString> PdfField::GetAlternateName() const
{
    if (m_Object->GetDictionary().HasKey("TU"))
        return m_Object->GetDictionary().MustFindKey("TU").GetString();

    return { };
}

void PdfField::SetMappingName(const PdfString& name)
{
    m_Object->GetDictionary().AddKey("TM", name);
}

optional<PdfString> PdfField::GetMappingName() const
{
    if (m_Object->GetDictionary().HasKey("TM"))
        return m_Object->GetDictionary().MustFindKey("TM").GetString();

    return { };
}

void PdfField::AddAlternativeAction(const PdfName& name, const PdfAction& action)
{
    auto aaObj = m_Object->GetDictionary().FindKey("AA");
    if (aaObj == nullptr)
        aaObj = &m_Object->GetDictionary().AddKey("AA", PdfDictionary());

    aaObj->GetDictionary().AddKey(name, action.GetObject().GetIndirectReference());
}

void PdfField::SetReadOnly(bool readOnly)
{
    this->SetFieldFlag(static_cast<int>(PdfFieldFlags::ReadOnly), readOnly);
}

bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag(static_cast<int>(PdfFieldFlags::ReadOnly), false);
}

void PdfField::SetRequired(bool required)
{
    this->SetFieldFlag(static_cast<int>(PdfFieldFlags::Required), required);
}

bool PdfField::IsRequired() const
{
    return this->GetFieldFlag(static_cast<int>(PdfFieldFlags::Required), false);
}

void PdfField::SetNoExport(bool exprt)
{
    this->SetFieldFlag(static_cast<int>(PdfFieldFlags::NoExport), exprt);
}

bool PdfField::IsNoExport() const
{
    return this->GetFieldFlag(static_cast<int>(PdfFieldFlags::NoExport), false);
}

PdfPage* PdfField::GetPage() const
{
    return m_Widget->GetPage();
}

void PdfField::SetMouseEnterAction(const PdfAction& action)
{
    this->AddAlternativeAction("E", action);
}

void PdfField::SetMouseLeaveAction(const PdfAction& action)
{
    this->AddAlternativeAction("X", action);
}

void PdfField::SetMouseDownAction(const PdfAction& action)
{
    this->AddAlternativeAction("D", action);
}

void PdfField::SetMouseUpAction(const PdfAction& action)
{
    this->AddAlternativeAction("U", action);
}

void PdfField::SetFocusEnterAction(const PdfAction& action)
{
    this->AddAlternativeAction("Fo", action);
}

void PdfField::SetFocusLeaveAction(const PdfAction& action)
{
    this->AddAlternativeAction("BI", action);
}

void PdfField::SetPageOpenAction(const PdfAction& action)
{
    this->AddAlternativeAction("PO", action);
}

void PdfField::SetPageCloseAction(const PdfAction& action)
{
    this->AddAlternativeAction("PC", action);
}

void PdfField::SetPageVisibleAction(const PdfAction& action)
{
    this->AddAlternativeAction("PV", action);
}

void PdfField::SetPageInvisibleAction(const PdfAction& action)
{
    this->AddAlternativeAction("PI", action);
}

void PdfField::SetKeystrokeAction(const PdfAction& action)
{
    this->AddAlternativeAction("K", action);
}

void PdfField::SetValidateAction(const PdfAction& action)
{
    this->AddAlternativeAction("V", action);
}

PdfFieldType PdfField::GetType() const
{
    return m_Field;
}

PdfAnnotation* PdfField::GetWidgetAnnotation() const
{
    return m_Widget;
}

PdfObject& PdfField::GetObject() const
{
    return *m_Object;
}

PdfDictionary& PdfField::GetDictionary()
{
    return m_Object->GetDictionary();
}

const PdfDictionary& PdfField::GetDictionary() const
{
    return m_Object->GetDictionary();
}

void getFullName(const PdfObject* obj, bool escapePartialNames, string& fullname)
{
    const PdfDictionary& dict = obj->GetDictionary();
    auto parent = dict.FindKey("Parent");;
    if (parent != nullptr)
        getFullName(parent, escapePartialNames, fullname);

    const PdfObject* nameObj = dict.GetKey("T");
    if (nameObj != nullptr)
    {
        string name = nameObj->GetString().GetString();
        if (escapePartialNames)
        {
            // According to ISO 32000-1:2008, "12.7.3.2 Field Names":
            // "Because the PERIOD is used as a separator for fully
            // qualified names, a partial name shall not contain a
            // PERIOD character."
            // In case the partial name still has periods (effectively
            // violating the standard and Pdf Reference) the fullname
            // would be unintelligible, let's escape them with double
            // dots "..", example "parent.partial..name"
            size_t currpos = 0;
            while ((currpos = name.find('.', currpos)) != std::string::npos)
            {
                name.replace(currpos, 1, "..");
                currpos += 2;
            }
        }

        if (fullname.length() == 0)
            fullname = name;
        else
            fullname.append(".").append(name);
    }
}

