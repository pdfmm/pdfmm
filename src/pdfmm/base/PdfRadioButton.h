/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_RADIO_BUTTON_H
#define PDF_RADIO_BUTTON_H

#include "PdfButton.h"

namespace mm
{
    /** A radio button
     * TODO: This is just a stub
     */
    class PDFMM_API PdfRadioButton : public PdfToggleButton
    {
        friend class PdfField;

    private:
        PdfRadioButton(PdfAcroForm& acroform, const std::shared_ptr<PdfField>& parent);

        PdfRadioButton(PdfAnnotationWidget& widget, const std::shared_ptr<PdfField>& parent);

        PdfRadioButton(PdfObject& obj, PdfAcroForm* acroform);

    public:
        PdfRadioButton* GetParent();
        const PdfRadioButton* GetParent() const;
    };
}

#endif // PDF_RADIO_BUTTON_H
