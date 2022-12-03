/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfComboBox.h"

using namespace std;
using namespace mm;


PdfComboBox::PdfComboBox(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdChoiceField(acroform, PdfFieldType::ComboBox, parent)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdChoiceField(widget, PdfFieldType::ComboBox, parent)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdChoiceField(obj, acroform, PdfFieldType::ComboBox)
{
    // NOTE: Do not do other initializations here
}

void PdfComboBox::SetEditable(bool edit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Edit), edit);
}

bool PdfComboBox::IsEditable() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Edit), false);
}

PdfComboBox* PdfComboBox::GetParent()
{
    return GetParentTyped<PdfComboBox>(PdfFieldType::ComboBox);
}

const PdfComboBox* PdfComboBox::GetParent() const
{
    return GetParentTyped<PdfComboBox>(PdfFieldType::ComboBox);
}
