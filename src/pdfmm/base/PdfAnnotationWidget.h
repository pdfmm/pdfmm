/*
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1
 */

#ifndef PDF_ANNOTATION_WIDGET_H
#define PDF_ANNOTATION_WIDGET_H

#include "PdfAnnotationActionBase.h"

namespace mm {

    class PDFMM_API PdfAnnotationWidget : public PdfAnnotationActionBase
    {
        friend class PdfAnnotation;
    private:
        PdfAnnotationWidget(PdfPage& page, const PdfRect& rect);
        PdfAnnotationWidget(PdfObject& obj);
    };
}

#endif // PDF_ANNOTATION_WIDGET_H
