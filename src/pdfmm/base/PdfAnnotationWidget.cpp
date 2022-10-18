/*
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfAnnotationWidget.h"

using namespace std;
using namespace mm;

PdfAnnotationWidget::PdfAnnotationWidget(PdfPage& page, const PdfRect& rect)
    : PdfAnnotationActionBase(page, PdfAnnotationType::Widget, rect)
{
}

PdfAnnotationWidget::PdfAnnotationWidget(PdfObject& obj)
    : PdfAnnotationActionBase(obj, PdfAnnotationType::Widget)
{
}
