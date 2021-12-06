/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfPushButton.h"

using namespace std;
using namespace mm;

PdfPushButton::PdfPushButton(PdfObject& obj, PdfAnnotation* widget)
    : PdfButton(PdfFieldType::PushButton, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfPushButton::PdfPushButton(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfButton(PdfFieldType::PushButton, doc, widget, insertInAcroform)
{
    Init();
}

PdfPushButton::PdfPushButton(PdfPage& page, const PdfRect& rect)
    : PdfButton(PdfFieldType::PushButton, page, rect)
{
    Init();
}

void PdfPushButton::Init()
{
    // make a push button
    this->SetFieldFlag(static_cast<int>(ePdfButton_PushButton), true);
}

void PdfPushButton::SetRolloverCaption(const PdfString& text)
{
    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("RC", text);
}

nullable<PdfString>  PdfPushButton::GetRolloverCaption() const
{
    auto mk = this->GetAppearanceCharacteristics();
    if (mk != nullptr && mk->GetDictionary().HasKey("RC"))
        return mk->GetDictionary().MustFindKey("RC").GetString();

    return { };
}

void PdfPushButton::SetAlternateCaption(const PdfString& text)
{
    auto& mk = this->GetOrCreateAppearanceCharacteristics();
    mk.GetDictionary().AddKey("AC", text);

}

nullable<PdfString>  PdfPushButton::GetAlternateCaption() const
{
    auto mk = this->GetAppearanceCharacteristics();
    if (mk != nullptr && mk->GetDictionary().HasKey("AC"))
        return mk->GetDictionary().MustFindKey("AC").GetString();

    return { };
}
