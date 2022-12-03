/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_COMBOBOX_H
#define PDF_COMBOBOX_H

#include "PdfChoiceField.h"

namespace mm
{
    /** A combo box with a drop down list of items.
     */
    class PDFMM_API PdfComboBox : public PdChoiceField
    {
        friend class PdfField;

    private:
        PdfComboBox(PdfAcroForm& acroform, const std::shared_ptr<PdfField>& parent);

        PdfComboBox(PdfAnnotationWidget& widget, const std::shared_ptr<PdfField>& parent);

        PdfComboBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        /**
         * Sets the combobox to be editable
         *
         * \param edit if true the combobox can be edited by the user
         *
         * By default a combobox is not editable
         */
        void SetEditable(bool edit);

        /**
         *  \returns true if this is an editable combobox
         */
        bool IsEditable() const;

        PdfComboBox* GetParent();
        const PdfComboBox* GetParent() const;
    };
}

#endif // PDF_COMBOBOX_H
