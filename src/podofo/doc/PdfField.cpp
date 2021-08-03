/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
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
using namespace PoDoFo;

void getFullName(const PdfObject* obj, bool escapePartialNames, string& fullname);

PdfField::PdfField(PdfFieldType eField, PdfPage& page, const PdfRect& rect)
    : m_eField(eField)
{
    m_pWidget = page.CreateAnnotation(PdfAnnotationType::Widget, rect);
    m_pObject = &m_pWidget->GetObject();
    Init(page.GetDocument().GetAcroForm());
}

PdfField::PdfField(PdfFieldType eField, PdfDocument& doc, PdfAnnotation* pWidget, bool insertInAcroform)
    : m_eField(eField), m_pObject(pWidget == nullptr ? nullptr : &pWidget->GetObject()), m_pWidget(pWidget)
{
    auto parent = doc.GetAcroForm();
    if (m_pObject == nullptr)
        m_pObject = parent->GetDocument().GetObjects().CreateDictionaryObject();

    Init(insertInAcroform ? parent : nullptr);
}

PdfField::PdfField(PdfFieldType eField, PdfPage& page, const PdfRect& rect, bool bAppearanceNone)
    : m_eField(eField)
{
    m_pWidget = page.CreateAnnotation(PdfAnnotationType::Widget, rect);
    m_pObject = &m_pWidget->GetObject();

    Init(page.GetDocument().GetAcroForm(true,
        bAppearanceNone ?
        EPdfAcroFormDefaulAppearance::None
        : EPdfAcroFormDefaulAppearance::BlackText12pt));
}

PdfField::PdfField(PdfFieldType eField, PdfObject& pObject, PdfAnnotation* pWidget)
    : m_eField(eField), m_pObject(&pObject), m_pWidget(pWidget)
{
}

PdfField* PdfField::CreateField(PdfObject& pObject)
{
    return createField(GetFieldType(pObject), pObject, nullptr);
}

PdfField* PdfField::CreateField(PdfAnnotation& pWidget)
{
    PdfObject& pObject = pWidget.GetObject();
    return createField(GetFieldType(pObject), pObject, &pWidget);
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
    auto doc = m_pObject->GetDocument();
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

    auto& dict = m_pObject->GetDictionary();
    auto kids = dict.FindKey("Kids");
    if (kids == nullptr)
        kids = &dict.AddKey("Kids", PdfArray());

    auto& arr = kids->GetArray();
    arr.push_back(childObj->GetIndirectReference());
    childObj->GetDictionary().AddKey("Parent", m_pObject->GetIndirectReference());
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
            PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }
}

PdfFieldType PdfField::GetFieldType(const PdfObject& rObject)
{
    PdfFieldType eField = PdfFieldType::Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    const PdfObject* pFT = rObject.GetDictionary().FindKeyParent("FT");
    if (!pFT)
        return PdfFieldType::Unknown;

    const PdfName fieldType = pFT->GetName();
    if (fieldType == "Btn")
    {
        int64_t flags;
        PdfField::GetFieldFlags(rObject, flags);

        if ((flags & PdfButton::ePdfButton_PushButton) == PdfButton::ePdfButton_PushButton)
        {
            eField = PdfFieldType::PushButton;
        }
        else if ((flags & PdfButton::ePdfButton_Radio) == PdfButton::ePdfButton_Radio)
        {
            eField = PdfFieldType::RadioButton;
        }
        else
        {
            eField = PdfFieldType::CheckBox;
        }
    }
    else if (fieldType == "Tx")
    {
        eField = PdfFieldType::TextField;
    }
    else if (fieldType == "Ch")
    {
        int64_t flags;
        PdfField::GetFieldFlags(rObject, flags);

        if ((flags & PdChoiceField::ePdfListField_Combo) == PdChoiceField::ePdfListField_Combo)
        {
            eField = PdfFieldType::ComboBox;
        }
        else
        {
            eField = PdfFieldType::ListBox;
        }
    }
    else if (fieldType == "Sig")
    {
        eField = PdfFieldType::Signature;
    }

    return eField;
}

void PdfField::Init(PdfAcroForm* pParent)
{
    if (pParent != nullptr)
    {
        // Insert into the parents kids array
        PdfArray& fields = pParent->GetFieldsArray();
        fields.push_back(m_pObject->GetIndirectReference());
    }

    PdfDictionary& dict = m_pObject->GetDictionary();
    switch (m_eField)
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
            PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
        }
        break;
    }
}

PdfField::PdfField(PdfObject& obj, PdfAnnotation* widget)
    : m_eField(PdfFieldType::Unknown), m_pObject(&obj), m_pWidget(widget)
{
    m_eField = GetFieldType(obj);
}

PdfObject* PdfField::GetAppearanceCharacteristics(bool create) const
{
    auto mkObj = m_pObject->GetDictionary().FindKey("MK");

    if (mkObj == nullptr && create)
    {
        PdfDictionary dictionary;
        mkObj = &const_cast<PdfField*>(this)->m_pObject->GetDictionary().AddKey("MK", dictionary);
    }

    return mkObj;
}

void PdfField::AssertTerminalField() const
{
    if (GetDictionary().HasKey("Kids"))
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "This method can be called only on terminal field. Ensure this field has "
            "not been retrieved from AcroFormFields collection or it's not a parent of terminal fields");
    }
}

void PdfField::SetFieldFlag(int64_t lValue, bool bSet)
{
    int64_t lCur = 0;

    if (m_pObject->GetDictionary().HasKey("Ff"))
        lCur = m_pObject->GetDictionary().MustFindKey("Ff").GetNumber();

    if (bSet)
        lCur |= lValue;
    else
    {
        if ((lCur & lValue) == lValue)
            lCur ^= lValue;
    }

    m_pObject->GetDictionary().AddKey("Ff", lCur);
}

bool PdfField::GetFieldFlag(int64_t lValue, bool bDefault) const
{
    int64_t flag;
    if (!GetFieldFlags(*m_pObject, flag))
        return bDefault;

    return (flag & lValue) == lValue;
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

void PdfField::SetHighlightingMode(PdfHighlightingMode eMode)
{
    PdfName value;

    switch (eMode)
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
            PODOFO_RAISE_ERROR(EPdfError::InvalidName);
            break;
    }

    m_pObject->GetDictionary().AddKey("H", value);
}

PdfHighlightingMode PdfField::GetHighlightingMode() const
{
    PdfHighlightingMode eMode = PdfHighlightingMode::Invert;

    if (m_pObject->GetDictionary().HasKey("H"))
    {
        auto& value = m_pObject->GetDictionary().MustFindKey("H").GetName();
        if (value == "N")
            return PdfHighlightingMode::None;
        else if (value == "I")
            return PdfHighlightingMode::Invert;
        else if (value == "O")
            return PdfHighlightingMode::InvertOutline;
        else if (value == "P")
            return PdfHighlightingMode::Push;
    }

    return eMode;
}

void PdfField::SetBorderColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double dGray)
{
    PdfArray array;
    array.push_back(dGray);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double dRed, double dGreen, double dBlue)
{
    PdfArray array;
    array.push_back(dRed);
    array.push_back(dGreen);
    array.push_back(dBlue);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBorderColor(double dCyan, double dMagenta, double dYellow, double dBlack)
{
    PdfArray array;
    array.push_back(dCyan);
    array.push_back(dMagenta);
    array.push_back(dYellow);
    array.push_back(dBlack);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BC", array);
}

void PdfField::SetBackgroundColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double dGray)
{
    PdfArray array;
    array.push_back(dGray);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double dRed, double dGreen, double dBlue)
{
    PdfArray array;
    array.push_back(dRed);
    array.push_back(dGreen);
    array.push_back(dBlue);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetBackgroundColor(double dCyan, double dMagenta, double dYellow, double dBlack)
{
    PdfArray array;
    array.push_back(dCyan);
    array.push_back(dMagenta);
    array.push_back(dYellow);
    array.push_back(dBlack);

    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey("BG", array);
}

void PdfField::SetName(const PdfString& rsName)
{
    m_pObject->GetDictionary().AddKey("T", rsName);
}

optional<PdfString> PdfField::GetName() const
{
    PdfObject* name = m_pObject->GetDictionary().FindKeyParent("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

optional<PdfString> PdfField::GetNameRaw() const
{
    PdfObject* name = m_pObject->GetDictionary().GetKey("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

string PdfField::GetFullName(bool escapePartialNames) const
{
    string fullName;
    getFullName(m_pObject, escapePartialNames, fullName);
    return fullName;
}

void PdfField::SetAlternateName(const PdfString& rsName)
{
    m_pObject->GetDictionary().AddKey("TU", rsName);
}

optional<PdfString> PdfField::GetAlternateName() const
{
    if (m_pObject->GetDictionary().HasKey("TU"))
        return m_pObject->GetDictionary().MustFindKey("TU").GetString();

    return { };
}

void PdfField::SetMappingName(const PdfString& rsName)
{
    m_pObject->GetDictionary().AddKey("TM", rsName);
}

optional<PdfString> PdfField::GetMappingName() const
{
    if (m_pObject->GetDictionary().HasKey("TM"))
        return m_pObject->GetDictionary().MustFindKey("TM").GetString();

    return { };
}

void PdfField::AddAlternativeAction(const PdfName& name, const PdfAction& action)
{
    auto aaObj = m_pObject->GetDictionary().FindKey("AA");
    if (aaObj == nullptr)
        aaObj = &m_pObject->GetDictionary().AddKey("AA", PdfDictionary());

    aaObj->GetDictionary().AddKey(name, action.GetObject().GetIndirectReference());
}

void PdfField::SetReadOnly(bool bReadOnly)
{
    this->SetFieldFlag(static_cast<int>(PdfFieldFlags::ReadOnly), bReadOnly);
}

bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag(static_cast<int>(PdfFieldFlags::ReadOnly), false);
}

void PdfField::SetRequired(bool bRequired)
{
    this->SetFieldFlag(static_cast<int>(PdfFieldFlags::Required), bRequired);
}

bool PdfField::IsRequired() const
{
    return this->GetFieldFlag(static_cast<int>(PdfFieldFlags::Required), false);
}

void PdfField::SetNoExport(bool bExport)
{
    this->SetFieldFlag(static_cast<int>(PdfFieldFlags::NoExport), bExport);
}

bool PdfField::IsNoExport() const
{
    return this->GetFieldFlag(static_cast<int>(PdfFieldFlags::NoExport), false);
}

PdfPage* PdfField::GetPage() const
{
    return m_pWidget->GetPage();
}

void PdfField::SetMouseEnterAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("E", rAction);
}

void PdfField::SetMouseLeaveAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("X", rAction);
}

void PdfField::SetMouseDownAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("D", rAction);
}

void PdfField::SetMouseUpAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("U", rAction);
}

void PdfField::SetFocusEnterAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("Fo", rAction);
}

void PdfField::SetFocusLeaveAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("BI", rAction);
}

void PdfField::SetPageOpenAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("PO", rAction);
}

void PdfField::SetPageCloseAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("PC", rAction);
}

void PdfField::SetPageVisibleAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("PV", rAction);
}

void PdfField::SetPageInvisibleAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("PI", rAction);
}

void PdfField::SetKeystrokeAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("K", rAction);
}

void PdfField::SetValidateAction(const PdfAction& rAction)
{
    this->AddAlternativeAction("V", rAction);
}

PdfFieldType PdfField::GetType() const
{
    return m_eField;
}

PdfAnnotation* PdfField::GetWidgetAnnotation() const
{
    return m_pWidget;
}

PdfObject& PdfField::GetObject() const
{
    return *m_pObject;
}

PdfDictionary& PdfField::GetDictionary()
{
    return m_pObject->GetDictionary();
}

const PdfDictionary& PdfField::GetDictionary() const
{
    return m_pObject->GetDictionary();
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

