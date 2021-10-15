/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>

#include "PdfAcroForm.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfDocument.h"
#include "PdfFont.h"

#include <sstream>

using namespace std;
using namespace mm;

// The AcroForm dict does NOT have a /Type key!
PdfAcroForm::PdfAcroForm(PdfDocument& doc, PdfAcroFormDefaulAppearance defaultAppearance)
    : PdfElement(doc)
{
    // Initialize with an empty fields array
    this->GetObject().GetDictionary().AddKey("Fields", PdfArray());
    Init(defaultAppearance);
}

PdfAcroForm::PdfAcroForm(PdfObject& obj)
    : PdfElement(obj)
{
}

PdfArray& PdfAcroForm::GetFieldsArray()
{
    auto fields = GetObject().GetDictionary().FindKey("Fields");
    if (fields == nullptr)
        fields = &GetObject().GetDictionary().AddKey("Fields", PdfArray());

    return fields->GetArray();
}

void PdfAcroForm::Init(PdfAcroFormDefaulAppearance defaultAppearance)
{
    // Add default appearance: black text, 12pt times 
    // -> only if we do not have a DA key yet

    if (defaultAppearance == PdfAcroFormDefaulAppearance::BlackText12pt)
    {
        PdfFontCreationParams params;
        params.Embed = false;
        params.Flags = PdfFontCreationFlags::AutoSelectStandard14;
        auto font = GetDocument().GetFontManager().GetFont("Helvetica", params);

        // Create DR key
        if (!this->GetObject().GetDictionary().HasKey("DR"))
            this->GetObject().GetDictionary().AddKey("DR", PdfDictionary());
        auto& resource = this->GetObject().GetDictionary().MustFindKey("DR");

        if (!resource.GetDictionary().HasKey("Font"))
            resource.GetDictionary().AddKey("Font", PdfDictionary());

        auto& fontDict = resource.GetDictionary().MustFindKey("Font");
        fontDict.GetDictionary().AddKey(font->GetIdentifier(), font->GetObject().GetIndirectReference());

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
    return this->GetObject().GetDictionary().FindKeyAs<bool>("NeedAppearances", false);
}
