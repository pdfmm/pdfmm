/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfRadioButton.h"

using namespace mm;

PdfRadioButton::PdfRadioButton(PdfObject& obj, PdfAnnotation* widget)
    : PdfButton(PdfFieldType::RadioButton, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfRadioButton::PdfRadioButton(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfButton(PdfFieldType::RadioButton, doc, widget, insertInAcroform)
{
}

PdfRadioButton::PdfRadioButton(PdfPage& page, const PdfRect& rect)
    : PdfButton(PdfFieldType::RadioButton, page, rect)
{
}
