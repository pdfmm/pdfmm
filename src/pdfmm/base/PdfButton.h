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
        friend class PdfPushButton;
        friend class PdfToggleButton;

    private:
        PdfButton(PdfAcroForm& acroform, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType);

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

    class PDFMM_API PdfToggleButton : public PdfButton
    {
        friend class PdfCheckBox;
        friend class PdfRadioButton;

    private:
        PdfToggleButton(PdfAcroForm& acroform, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfToggleButton(PdfAnnotationWidget& widget, PdfFieldType fieldType,
            const std::shared_ptr<PdfField>& parent);

        PdfToggleButton(PdfObject& obj, PdfAcroForm* acroform, PdfFieldType fieldType);
    };
}

#endif // PDF_BUTTON_H
