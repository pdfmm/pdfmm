/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_OBJECT_H
#define PDF_FONT_OBJECT_H

#include "PdfFont.h"
#include "PdfCIDToGIDMap.h"

namespace mm {

class PDFMM_API PdfFontObject final : public PdfFont
{
    friend class PdfFont;

private:
    /** Create a PdfFontObject based on an existing PdfObject
     *  To be used by PdfFontFactory
     */
    PdfFontObject(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

    PdfFontObject(PdfObject& obj, PdfObject& descendantObj,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding);

public:
    bool TryMapCIDToGID(unsigned cid, unsigned& gid) const override;

public:
    bool IsObjectLoaded() const override;
    PdfFontType GetType() const override;

private:
    std::unique_ptr<PdfCIDToGIDMap> m_cidToGidMap;
};

}

#endif // PDF_FONT_OBJECT_H
