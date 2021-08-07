/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfListBox.h"

using namespace mm;

PdfListBox::PdfListBox(PdfObject& obj, PdfAnnotation* widget)
    : PdChoiceField(PdfFieldType::ListBox, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfListBox::PdfListBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdChoiceField(PdfFieldType::ListBox, doc, widget, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfPage& page, const PdfRect& rect)
    : PdChoiceField(PdfFieldType::ListBox, page, rect)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}
