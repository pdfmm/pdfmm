/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfPushButton.h"

using namespace std;
using namespace PoDoFo;

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

void PdfPushButton::SetRolloverCaption(const PdfString& rsText)
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey(PdfName("RC"), rsText);
}

optional<PdfString>  PdfPushButton::GetRolloverCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(false);
    if (pMK && pMK->GetDictionary().HasKey(PdfName("RC")))
        return pMK->GetDictionary().GetKey(PdfName("RC"))->GetString();

    return { };
}

void PdfPushButton::SetAlternateCaption(const PdfString& rsText)
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey(PdfName("AC"), rsText);

}

optional<PdfString>  PdfPushButton::GetAlternateCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(false);
    if (pMK && pMK->GetDictionary().HasKey(PdfName("AC")))
        return pMK->GetDictionary().GetKey(PdfName("AC"))->GetString();

    return { };
}
