/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfPushButton.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfPushButton::PdfPushButton(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdfButton(acroform, PdfFieldType::PushButton, parent)
{
    init();
}

PdfPushButton::PdfPushButton(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdfButton(widget, PdfFieldType::PushButton, parent)
{
    init();
}

PdfPushButton::PdfPushButton(PdfObject& obj, PdfAcroForm* acroform)
    : PdfButton(obj, acroform, PdfFieldType::PushButton)
{
    // NOTE: Do not call init() here
}

void PdfPushButton::init()
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

PdfPushButton* PdfPushButton::GetParent()
{
    return GetParentTyped<PdfPushButton>(PdfFieldType::PushButton);
}

const PdfPushButton* PdfPushButton::GetParent() const
{
    return GetParentTyped<PdfPushButton>(PdfFieldType::PushButton);
}
