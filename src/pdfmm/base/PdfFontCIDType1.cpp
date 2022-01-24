/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfFontCIDType1.h"

using namespace std;
using namespace mm;

PdfFontCIDType1::PdfFontCIDType1(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) : PdfFontCID(doc, metrics, encoding) { }

bool PdfFontCIDType1::SupportsSubsetting() const
{
    // Not yet supported
    return false;
}

PdfFontType PdfFontCIDType1::GetType() const
{
    return PdfFontType::CIDType1;
}

void PdfFontCIDType1::embedFontFile(PdfObject& descriptor)
{
    (void)descriptor;
    if (IsSubsettingEnabled())
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
    }
    else
    {

    }
}
