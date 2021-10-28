/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfExtGState.h"

#include <sstream>

#include "PdfDictionary.h"
#include "PdfWriter.h"
#include "PdfLocale.h"
#include "PdfPage.h"

using namespace std;
using namespace mm;

PdfExtGState::PdfExtGState(PdfDocument& doc)
    : PdfDictionaryElement(doc, "ExtGState")
{
    ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "ExtGS" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.str().c_str());

    this->Init();
}

void PdfExtGState::Init()
{
}

void PdfExtGState::SetFillOpacity(double opac)
{
    this->GetObject().GetDictionary().AddKey("ca", PdfVariant(opac));
}

void PdfExtGState::SetStrokeOpacity(double opac)
{
    this->GetObject().GetDictionary().AddKey("CA", PdfVariant(opac));
}

void PdfExtGState::SetBlendMode(const string_view& blendMode)
{
    this->GetObject().GetDictionary().AddKey("BM", PdfName(blendMode));
}

void PdfExtGState::SetOverprint(bool enable)
{
    this->GetObject().GetDictionary().AddKey("OP", PdfVariant(enable));
}

void PdfExtGState::SetFillOverprint(bool enable)
{
    this->GetObject().GetDictionary().AddKey("op", PdfVariant(enable));
}

void PdfExtGState::SetStrokeOverprint(bool enable)
{
    this->GetObject().GetDictionary().AddKey("OP", PdfVariant(enable));
}

void PdfExtGState::SetNonZeroOverprint(bool enable)
{
    this->GetObject().GetDictionary().AddKey("OPM", PdfVariant(static_cast<int64_t>(enable ? 1 : 0)));
}

void PdfExtGState::SetRenderingIntent(const string_view& intent)
{
    this->GetObject().GetDictionary().AddKey("RI", PdfName(intent));
}

void PdfExtGState::SetFrequency(double frequency)
{
    PdfDictionary halftoneDict;
    halftoneDict.AddKey("HalftoneType", PdfVariant(static_cast<int64_t>(1)));
    halftoneDict.AddKey("Frequency", PdfVariant(frequency));
    halftoneDict.AddKey("Angle", PdfVariant(45.0));
    halftoneDict.AddKey("SpotFunction", PdfName("SimpleDot"));

    this->GetObject().GetDictionary().AddKey("HT", halftoneDict);
}
