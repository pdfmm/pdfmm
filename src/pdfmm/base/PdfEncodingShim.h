/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DYNAMIC_ENCODING_H
#define PDF_DYNAMIC_ENCODING_H

#include "PdfEncoding.h"
#include "PdfCharCodeMap.h"

namespace mm {

    class PdfFont;

    /**
     * Encoding shim class that mocks an existing encoding
     * Used by PdfFont to to wrap
     */
    class PDFMM_API PdfEncodingShim final : public PdfEncoding
    {
        friend class PdfFont;

    private:
        PdfEncodingShim(const PdfEncoding& encoding, PdfFont& font);

    public:
        PdfFont& GetFont() const override;

    private:
        PdfFont* m_Font;
    };

    /**
     * WIP: Encoding class with an external encoding map storage
     * To be used by PdfFont in case of dynamic encoding requested
     */
    class PDFMM_API PdfDynamicEncoding final : public PdfEncoding
    {
        friend class PdfFont;
        friend class PdfEncodingFactory;

    public:
        PdfFont& GetFont() const override;

    private:
        /**
         * To be used by PdfFont
         */
        PdfDynamicEncoding(const std::shared_ptr<PdfCharCodeMap>& map, PdfFont& font);

    private:
        PdfFont* m_font;
    };

};

#endif // PDF_DYNAMIC_ENCODING_H
