/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfFontType3.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfName.h"
#include "base/PdfStream.h"

using namespace std;
using namespace PoDoFo;

PdfFontType3::PdfFontType3(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding)
    : PdfFontSimple(doc, metrics, encoding)
{
}

PdfFontType PdfFontType3::GetType() const
{
    return PdfFontType::Type3;
}

void PdfFontType3::initImported()
{
    this->Init("Type3");
}

void PdfFontType3::embedFontFile(PdfObject& descriptor)
{
    (void)descriptor;
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFontFormat);
}
