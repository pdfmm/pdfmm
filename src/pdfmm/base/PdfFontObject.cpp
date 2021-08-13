/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontObject.h"

using namespace std;
using namespace mm;

PdfFontObject::PdfFontObject(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding) :
    PdfFont(obj, metrics, encoding)
{
}

PdfFontType PdfFontObject::GetType() const
{
    // TODO Just read the object from the object
    return PdfFontType::Unknown;
}
