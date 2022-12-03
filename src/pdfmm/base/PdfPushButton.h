/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PUSH_BUTTON_H
#define PDF_PUSH_BUTTON_H

#include "PdfButton.h"

namespace mm
{
    /** A push button is a button which has no state and value
     *  but can toggle actions.
     */
    class PDFMM_API PdfPushButton : public PdfButton
    {
        friend class PdfField;

    private:
        PdfPushButton(PdfAcroForm& acroform, const std::shared_ptr<PdfField>& parent);

        PdfPushButton(PdfAnnotationWidget& widget, const std::shared_ptr<PdfField>& parent);

        PdfPushButton(PdfObject& obj, PdfAcroForm* acroform);

    public:
        /** Set the rollover caption of this button
         *  which is displayed when the cursor enters the field
         *  without the mouse button being pressed
         *
         *  \param text the caption
         */
        void SetRolloverCaption(const PdfString& text);

        /**
         *  \returns the rollover caption of this button
         */
        nullable<PdfString> GetRolloverCaption() const;

        /** Set the alternate caption of this button
         *  which is displayed when the button is pressed.
         *
         *  \param text the caption
         */
        void SetAlternateCaption(const PdfString& text);

        /**
         *  \returns the rollover caption of this button
         */
        nullable<PdfString> GetAlternateCaption() const;

        PdfPushButton* GetParent();
        const PdfPushButton* GetParent() const;

    private:
        void init();
    };
}

#endif // PDF_PUSH_BUTTON_H
