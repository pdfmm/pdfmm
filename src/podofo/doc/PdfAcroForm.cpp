/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfAcroForm.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h" 
#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"

#include "PdfDocument.h"
#include "PdfFont.h"

#include <sstream>

using namespace std;
using namespace PoDoFo;

// The AcroForm dict does NOT have a /Type key!
PdfAcroForm::PdfAcroForm(PdfDocument& doc, EPdfAcroFormDefaulAppearance defaultAppearance)
    : PdfElement(doc)
{
    // Initialize with an empty fields array
    this->GetObject().GetDictionary().AddKey("Fields", PdfArray());
    Init(defaultAppearance);
}

PdfAcroForm::PdfAcroForm(PdfObject& obj, EPdfAcroFormDefaulAppearance defaultAppearance)
    : PdfElement(obj)
{
    Init(defaultAppearance);
}

PdfArray & PdfAcroForm::GetFieldsArray()
{
    auto fields = GetObject().GetDictionary().FindKey("Fields");
    if (fields == nullptr)
        fields = &GetObject().GetDictionary().AddKey("Fields", PdfArray());

    return fields->GetArray();
}

void PdfAcroForm::Init(EPdfAcroFormDefaulAppearance defaultAppearance)
{
    // Add default appearance: black text, 12pt times 
    // -> only if we do not have a DA key yet

    if (!this->GetObject().GetDictionary().HasKey("DA") &&
        defaultAppearance == EPdfAcroFormDefaulAppearance::BlackText12pt)
    {
        PdfFontCreationParams params;
        params.Embed = false;
        params.Flags = EFontCreationFlags::AutoSelectBase14;
        PdfFont* font = GetDocument().GetFontCache().GetFont("Helvetica", params);

        PdfObject* resource;
        PdfObject* fontDict;

        // Create DR key
        if (!this->GetObject().GetDictionary().HasKey("DR"))
            this->GetObject().GetDictionary().AddKey("DR", PdfDictionary());
        resource = this->GetObject().GetDictionary().GetKey("DR");
        if (resource->IsReference())
            resource = GetDocument().GetObjects().GetObject(resource->GetReference());

        if (!resource->GetDictionary().HasKey("Font"))
            resource->GetDictionary().AddKey("Font", PdfDictionary());
        fontDict = resource->GetDictionary().GetKey("Font");
        if (fontDict->IsReference())
            fontDict = GetDocument().GetObjects().GetObject(fontDict->GetReference());

        fontDict->GetDictionary().AddKey(font->GetIdentifier(), font->GetObject().GetIndirectReference());

        // Create DA key
        ostringstream oss;
        PdfLocaleImbue(oss);
        oss << "0 0 0 rg /" << font->GetIdentifier().GetString() << " 12 Tf";
        this->GetObject().GetDictionary().AddKey("DA", PdfString(oss.str()));
    }
}

void PdfAcroForm::SetNeedAppearances(bool needAppearances)
{
    this->GetObject().GetDictionary().AddKey("NeedAppearances", PdfVariant(needAppearances));
}

bool PdfAcroForm::GetNeedAppearances() const
{
    return this->GetObject().GetDictionary().FindAs<bool>("NeedAppearances", false);
}
