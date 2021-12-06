/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfCheckBox.h"

using namespace mm;


PdfCheckBox::PdfCheckBox(PdfObject& obj, PdfAnnotation* widget)
    : PdfButton(PdfFieldType::CheckBox, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfCheckBox::PdfCheckBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfButton(PdfFieldType::CheckBox, doc, widget, insertInAcroform)
{
}

PdfCheckBox::PdfCheckBox(PdfPage& page, const PdfRect& rect)
    : PdfButton(PdfFieldType::CheckBox, page, rect)
{
}

void PdfCheckBox::AddAppearanceStream(const PdfName& name, const PdfReference& reference)
{
    if (!GetObject().GetDictionary().HasKey("AP"))
        GetObject().GetDictionary().AddKey("AP", PdfDictionary());

    if (!GetObject().GetDictionary().MustFindKey("AP").GetDictionary().HasKey("N"))
        GetObject().GetDictionary().MustFindKey("AP").GetDictionary().AddKey("N", PdfDictionary());

    GetObject().GetDictionary().MustFindKey("AP").
        GetDictionary().MustFindKey("N").GetDictionary().AddKey(name, reference);
}

void PdfCheckBox::SetAppearanceChecked(const PdfXObject& xobj)
{
    this->AddAppearanceStream("Yes", xobj.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetAppearanceUnchecked(const PdfXObject& xobj)
{
    this->AddAppearanceStream("Off", xobj.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetChecked(bool isChecked)
{
    GetObject().GetDictionary().AddKey("V", (isChecked ? PdfName("Yes") : PdfName("Off")));
    GetObject().GetDictionary().AddKey("AS", (isChecked ? PdfName("Yes") : PdfName("Off")));
}

bool PdfCheckBox::IsChecked() const
{
    auto& dict = GetObject().GetDictionary();
    if (dict.HasKey("V"))
    {
        auto& name = dict.MustFindKey("V").GetName();
        return (name == "Yes" || name == "On");
    }
    else if (dict.HasKey("AS"))
    {
        auto& name = dict.MustFindKey("AS").GetName();
        return (name == "Yes" || name == "On");
    }

    return false;
}
