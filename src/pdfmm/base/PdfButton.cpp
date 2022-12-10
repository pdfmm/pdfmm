/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfButton.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfButton::PdfButton(PdfAcroForm& acroform, PdfFieldType fieldType,
        const shared_ptr<PdfField>& parent)
    : PdfField(acroform, fieldType, parent)
{
}

PdfButton::PdfButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
        const shared_ptr<PdfField>& parent)
    : PdfField(widget, fieldType, parent)
{
}

PdfButton::PdfButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfField(obj, acroform, fieldType)
{
}

bool PdfButton::IsPushButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false);
}

bool PdfButton::IsCheckBox() const
{
    return (!this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false) &&
        !this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false));
}

bool PdfButton::IsRadioButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false);
}

void PdfButton::SetCaption(const PdfString& text)
{
    PdfObject& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("CA", text);
}

nullable<PdfString> PdfButton::GetCaption() const
{
    auto mk = this->GetAppearanceCharacteristics();
    if (mk != nullptr && mk->GetDictionary().HasKey("CA"))
        return mk->GetDictionary().MustFindKey("CA").GetString();

    return { };
}

PdfToggleButton::PdfToggleButton(PdfAcroForm& acroform, PdfFieldType fieldType,
    const shared_ptr<PdfField>& parent)
    : PdfButton(acroform, fieldType, parent)
{
}

PdfToggleButton::PdfToggleButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
    const shared_ptr<PdfField>& parent)
    : PdfButton(widget, fieldType, parent)
{
}

PdfToggleButton::PdfToggleButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType)
    : PdfButton(obj, acroform, fieldType)
{
}
