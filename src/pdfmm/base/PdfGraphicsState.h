/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_GRAPHICS_STATE_H
#define PDF_GRAPHICS_STATE_H

#include "PdfColor.h"
#include "PdfMath.h"

namespace mm
{
    // TODO: Add missing properties ISO 32000-1:2008 "8.4 Graphics State"
    struct PDFMM_API PdfGraphicsState final
    {
    public:
        PdfGraphicsState();
    public:
        Matrix CTM;
        double LineWidth;
        double MiterLimit;
        PdfLineCapStyle LineCapStyle;
        PdfLineJoinStyle LineJoinStyle;
        std::string RenderingIntent;
        PdfColor FillColor;
        PdfColor StrokeColor;
    };
}

#endif // PDF_GRAPHICS_STATE_H
