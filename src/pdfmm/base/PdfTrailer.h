/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TRAILER
#define PDF_TRAILER

#include "PdfElement.h"

namespace mm
{
    class PDFMM_API PdfTrailer final : public PdfDictionaryElement
    {
    public:
        PdfTrailer(PdfObject& obj);
    public:
    };
}

#endif // PDF_TRAILER
