/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_OBJECT_H
#define PDF_FONT_OBJECT_H

#include "PdfFont.h"

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

public:
    static std::unique_ptr<PdfFontObject> Create(PdfObject& obj, PdfObject& descendantObj,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding);

    static std::unique_ptr<PdfFontObject> Create(PdfObject& obj,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding);

public:
    bool tryMapCIDToGID(unsigned cid, unsigned& gid) const override;

public:
    bool IsObjectLoaded() const override;
    PdfFontType GetType() const override;
};

}

#endif // PDF_FONT_OBJECT_H
