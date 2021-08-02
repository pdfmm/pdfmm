/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfCheckBox.h"

using namespace PoDoFo;


PdfCheckBox::PdfCheckBox(PdfObject& obj, PdfAnnotation* widget)
    : PdfButton(PdfFieldType::CheckBox, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfCheckBox::PdfCheckBox(PdfDocument& doc, PdfAnnotation* widget,bool insertInAcroform)
    : PdfButton(PdfFieldType::CheckBox, doc, widget, insertInAcroform)
{
}

PdfCheckBox::PdfCheckBox(PdfPage& page, const PdfRect& rect)
    : PdfButton(PdfFieldType::CheckBox, page, rect)
{
}

void PdfCheckBox::AddAppearanceStream(const PdfName& rName, const PdfReference& rReference)
{
    if (!GetObject().GetDictionary().HasKey(PdfName("AP")))
        GetObject().GetDictionary().AddKey(PdfName("AP"), PdfDictionary());

    if (!GetObject().GetDictionary().GetKey(PdfName("AP"))->GetDictionary().HasKey(PdfName("N")))
        GetObject().GetDictionary().GetKey(PdfName("AP"))->GetDictionary().AddKey(PdfName("N"), PdfDictionary());

    GetObject().GetDictionary().GetKey(PdfName("AP"))->
        GetDictionary().GetKey(PdfName("N"))->GetDictionary().AddKey(rName, rReference);
}

void PdfCheckBox::SetAppearanceChecked(const PdfXObject& rXObject)
{
    this->AddAppearanceStream(PdfName("Yes"), rXObject.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetAppearanceUnchecked(const PdfXObject& rXObject)
{
    this->AddAppearanceStream(PdfName("Off"), rXObject.GetObject().GetIndirectReference());
}

void PdfCheckBox::SetChecked(bool bChecked)
{
    GetObject().GetDictionary().AddKey(PdfName("V"), (bChecked ? PdfName("Yes") : PdfName("Off")));
    GetObject().GetDictionary().AddKey(PdfName("AS"), (bChecked ? PdfName("Yes") : PdfName("Off")));
}

bool PdfCheckBox::IsChecked() const
{
    PdfDictionary dic = GetObject().GetDictionary();

    if (dic.HasKey(PdfName("V"))) {
        PdfName name = dic.GetKey(PdfName("V"))->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
    }
    else if (dic.HasKey(PdfName("AS"))) {
        PdfName name = dic.GetKey(PdfName("AS"))->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
    }

    return false;
}
