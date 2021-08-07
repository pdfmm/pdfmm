/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CHECKBOX_H
#define PDF_CHECKBOX_H

#include "PdfButton.h"
#include "PdfXObject.h"

namespace mm
{
    /** A checkbox can be checked or unchecked by the user
     */
    class PDFMM_API PdfCheckBox : public PdfButton
    {
        friend class PdfField;
    private:
        PdfCheckBox(PdfObject& obj, PdfAnnotation* widget);
    public:
        PdfCheckBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

        PdfCheckBox(PdfPage& page, const PdfRect& rect);

        /** Set the appearance stream which is displayed when the checkbox
         *  is checked.
         *
         *  \param rXObject an xobject which contains the drawing commands for a checked checkbox
         */
        void SetAppearanceChecked(const PdfXObject& xobj);

        /** Set the appearance stream which is displayed when the checkbox
         *  is unchecked.
         *
         *  \param rXObject an xobject which contains the drawing commands for an unchecked checkbox
         */
        void SetAppearanceUnchecked(const PdfXObject& xobj);

        /** Sets the state of this checkbox
         *
         *  \param isChecked if true the checkbox will be checked
         */
        void SetChecked(bool isChecked);

        /**
         * \returns true if the checkbox is checked
         */
        bool IsChecked() const;

    private:

        /** Add a appearance stream to this checkbox
         *
         *  \param name name of the appearance stream
         *  \param reference reference to the XObject containing the appearance stream
         */
        void AddAppearanceStream(const PdfName& name, const PdfReference& reference);
    };
}

#endif // PDF_CHECKBOX_H
