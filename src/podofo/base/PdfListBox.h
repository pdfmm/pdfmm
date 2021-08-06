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

namespace PoDoFo
{
    /** A list box
     */
    class PODOFO_DOC_API PdfListBox : public PdChoiceField
    {
        friend class PdfField;
    private:
        PdfListBox(PdfObject& obj, PdfAnnotation* widget);

    public:
        PdfListBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

        PdfListBox(PdfPage& page, const PdfRect& rect);
    };
}

#endif // PDF_LISTBOX_H
