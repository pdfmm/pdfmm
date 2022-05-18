/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TEXT_STATE_H
#define PDF_TEXT_STATE_H

#include "PdfDeclarations.h"

namespace mm
{
    class PdfFont;

    // TODO: Add missing properties ISO 32000-1:2008 "9.3 Text State Parameters and Operators"
    struct PDFMM_API PdfTextState final
    {
    public:
        PdfTextState();
    public:
        const PdfFont* Font;
        double FontSize;
        double FontScale;
        double CharSpacing;
        double WordSpacing;
        PdfTextRenderingMode RenderingMode;
    };
}

#endif // PDF_TEXT_STATE_H
