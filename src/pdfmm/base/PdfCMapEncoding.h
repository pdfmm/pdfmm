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

    class PDFMM_API PdfCMapEncoding : public PdfEncodingMapBase
    {
    public:
        /** Construct a PdfCMapEncoding from a map
         */
        PdfCMapEncoding(PdfCharCodeMap&& map);

    public:
        /** Construct a PdfCMapEncoding from an object
         */
        static std::unique_ptr<PdfCMapEncoding> Create(const PdfObject& cmapObj);

    public:
        bool IsCMapEncoding() const override;
        bool HasLigaturesSupport() const override;

    private:
        struct MapIdentity
        {
            PdfCharCodeMap Map;
            PdfEncodingLimits Limits;
        };
        PdfCMapEncoding(MapIdentity map);
        static MapIdentity parseCMapObject(const PdfObjectStream& stream);
    };
}

#endif // PDF_CMAP_ENCODING_H
