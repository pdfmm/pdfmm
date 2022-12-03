/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfListBox.h"

using namespace std;
using namespace mm;

PdfListBox::PdfListBox(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdChoiceField(acroform, PdfFieldType::ListBox, parent)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdChoiceField(widget, PdfFieldType::ListBox, parent)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdChoiceField(obj, acroform, PdfFieldType::ListBox)
{
    // NOTE: Do not do other initializations here
}

PdfListBox* PdfListBox::GetParent()
{
    return GetParentTyped<PdfListBox>(PdfFieldType::ListBox);
}

const PdfListBox* PdfListBox::GetParent() const
{
    return GetParentTyped<PdfListBox>(PdfFieldType::ListBox);
}
