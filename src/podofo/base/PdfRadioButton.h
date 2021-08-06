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

namespace PoDoFo
{
    /** A radio button
     * TODO: This is just a stub
     */
    class PODOFO_DOC_API PdfRadioButton : public PdfButton
    {
        friend class PdfField;
    private:
        PdfRadioButton(PdfObject& obj, PdfAnnotation* widget);
    public:
        PdfRadioButton(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

        PdfRadioButton(PdfPage& page, const PdfRect& rect);
    };
}

#endif // PDF_RADIO_BUTTON_H
