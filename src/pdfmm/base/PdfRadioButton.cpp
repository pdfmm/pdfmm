/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfRadioButton.h"

using namespace std;
using namespace mm;

PdfRadioButton::PdfRadioButton(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdfToggleButton(acroform, PdfFieldType::RadioButton, parent)
{
}

PdfRadioButton::PdfRadioButton(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdfToggleButton(widget, PdfFieldType::RadioButton, parent)
{
}

PdfRadioButton::PdfRadioButton(PdfObject& obj, PdfAcroForm* acroform)
    : PdfToggleButton(obj, acroform, PdfFieldType::RadioButton)
{
}

PdfRadioButton* PdfRadioButton::GetParent()
{
    return GetParentTyped<PdfRadioButton>(PdfFieldType::RadioButton);
}

const PdfRadioButton* PdfRadioButton::GetParent() const
{
    return GetParentTyped<PdfRadioButton>(PdfFieldType::RadioButton);
}
