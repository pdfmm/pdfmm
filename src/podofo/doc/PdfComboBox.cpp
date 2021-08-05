/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfComboBox.h"

using namespace PoDoFo;

PdfComboBox::PdfComboBox(PdfObject& obj, PdfAnnotation* widget)
    : PdChoiceField(PdfFieldType::ComboBox, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfComboBox::PdfComboBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdChoiceField(PdfFieldType::ComboBox, doc, widget, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(PdfPage& page, const PdfRect& rect)
    : PdChoiceField(PdfFieldType::ComboBox, page, rect)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

void PdfComboBox::SetEditable(bool edit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Edit), edit);
}

bool PdfComboBox::IsEditable() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Edit), false);
}
