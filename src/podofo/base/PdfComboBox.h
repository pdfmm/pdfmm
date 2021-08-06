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

namespace PoDoFo
{
    /** A combo box with a drop down list of items.
     */
    class PODOFO_DOC_API PdfComboBox : public PdChoiceField
    {
        friend class PdfField;
    private:
        PdfComboBox(PdfObject& obj, PdfAnnotation* widget);

    public:
        PdfComboBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

        PdfComboBox(PdfPage& page, const PdfRect& rect);

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

    };
}

#endif // PDF_COMBOBOX_H
