/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_BUTTON_H
#define PDF_BUTTON_H

#include "PdfField.h"

namespace mm
{
    class PDFMM_API PdfButton : public PdfField
    {
        friend class PdfField;
    protected:
        enum
        {
            ePdfButton_NoToggleOff = 0x0004000,
            ePdfButton_Radio = 0x0008000,
            ePdfButton_PushButton = 0x0010000,
            ePdfButton_RadioInUnison = 0x2000000
        };

        PdfButton(PdfFieldType fieldType, PdfDocument& doc, PdfAnnotation* widget, bool insertInAcrofrom);

        PdfButton(PdfFieldType fieldType, PdfObject& object, PdfAnnotation* widget);

        PdfButton(PdfFieldType fieldType, PdfPage& page, const PdfRect& rect);

    public:
        /**
         * \returns true if this is a pushbutton
         */
        bool IsPushButton() const;

        /**
         * \returns true if this is a checkbox
         */
        bool IsCheckBox() const;

        /**
         * \returns true if this is a radiobutton
         */
        bool IsRadioButton() const;

        /** Set the normal caption of this button
         *
         *  \param text the caption
         */
        void SetCaption(const PdfString& text);

        /**
         *  \returns the caption of this button
         */
        nullable<PdfString> GetCaption() const;
    };
}

#endif // PDF_BUTTON_H
