/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CMAP_ENCODING_H
#define PDF_CMAP_ENCODING_H

#include "PdfEncodingMap.h"

namespace mm
{
    class PdfObject;
    class PdfObjectStream;

    class PDFMM_API PdfCMapEncoding final : public PdfEncodingMapBase
    {
        friend class PdfEncodingMap;

    public:
        /** Construct a PdfCMapEncoding from a map
         */
        PdfCMapEncoding(PdfCharCodeMap&& map);

    private:
        PdfCMapEncoding(PdfCharCodeMap&& map, const PdfEncodingLimits& limits);

    public:
        bool HasLigaturesSupport() const override;
        const PdfEncodingLimits& GetLimits() const override;

    private:
        PdfEncodingLimits m_Limits;
    };
}

#endif // PDF_CMAP_ENCODING_H
