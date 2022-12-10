/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDF_ANNOTATION_ACTION_BASE_H
#define PDF_ANNOTATION_ACTION_BASE_H

#include "PdfAnnotation.h"
#include "PdfAction.h"

namespace mm {

    class PDFMM_API PdfAnnotationActionBase : public PdfAnnotation
    {
        friend class PdfAnnotationWidget;
        friend class PdfAnnotationLink;
        friend class PdfAnnotationScreen;

    private:
        PdfAnnotationActionBase(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect);
        PdfAnnotationActionBase(PdfObject& obj, PdfAnnotationType annotType);

    public:
        /** Set the action that is executed for this annotation
         *  \param action an action object
         *
         *  \see GetAction
         */
        void SetAction(const std::shared_ptr<PdfAction>& action);

        /** Get the action that is executed for this annotation
         *  \returns an action object. The action object is owned
         *           by the PdfAnnotation.
         *
         *  \see SetAction
         */
        std::shared_ptr<PdfAction> GetAction() const;

    private:
        std::shared_ptr<PdfAction> getAction();

    private:
        std::shared_ptr<PdfAction> m_Action;
    };
}

#endif // PDF_ANNOTATION_ACTION_BASE_H
