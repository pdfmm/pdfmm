/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>

#include "PdfButton.h"

using namespace std;
using namespace mm;

PdfButton::PdfButton(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcrofrom)
    : PdfField(fieldType, doc, widget, insertInAcrofrom)
{
}

PdfButton::PdfButton(PdfFieldType fieldType, PdfObject& object, PdfAnnotation* widget)
    : PdfField(fieldType, object, widget)
{
}

PdfButton::PdfButton(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect)
    : PdfField(fieldType, page, rect)
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
