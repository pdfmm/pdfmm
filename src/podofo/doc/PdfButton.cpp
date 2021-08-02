/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfButton.h"

using namespace std;
using namespace PoDoFo;

PdfButton::PdfButton(PdfFieldType eField, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcrofrom)
    : PdfField(eField, doc, widget, insertInAcrofrom)
{
}

PdfButton::PdfButton(PdfFieldType eField, PdfObject& object, PdfAnnotation* widget)
    : PdfField(eField, object, widget)
{
}

PdfButton::PdfButton(PdfFieldType eField, PdfPage& page, const PdfRect& rect)
    : PdfField(eField, page, rect)
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
    PdfObject* mk = this->GetAppearanceCharacteristics(true);
    mk->GetDictionary().AddKey("CA", text);
}

optional<PdfString> PdfButton::GetCaption() const
{
    auto mk = this->GetAppearanceCharacteristics(false);
    if (mk != nullptr && mk->GetDictionary().HasKey("CA"))
        return mk->GetDictionary().MustFindKey("CA").GetString();

    return { };
}
