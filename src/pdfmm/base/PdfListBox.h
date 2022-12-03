/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_LISTBOX_H
#define PDF_LISTBOX_H

#include "PdfChoiceField.h"

namespace mm
{
    /** A list box
     */
    class PDFMM_API PdfListBox : public PdChoiceField
    {
        friend class PdfField;

    private:
        PdfListBox(PdfAcroForm& acroform, const std::shared_ptr<PdfField>& parent);

        PdfListBox(PdfAnnotationWidget& widget, const std::shared_ptr<PdfField>& parent);

        PdfListBox(PdfObject& obj, PdfAcroForm* acroform);

    public:
        PdfListBox* GetParent();
        const PdfListBox* GetParent() const;
    };
}

#endif // PDF_LISTBOX_H
