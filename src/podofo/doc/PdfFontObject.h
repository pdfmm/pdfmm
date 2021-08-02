/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_OBJECT_H
#define PDF_FONT_OBJECT_H

#include "PdfFont.h"

namespace PoDoFo {

class PODOFO_DOC_API PdfFontObject final : public PdfFont
{
    friend class PdfFontFactory;

private:
    /** Create a PdfFontObject based on an existing PdfObject
     *  To be used by PdfFontFactory
     */
    PdfFontObject(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    PdfFontType GetType() const override;
};

}

#endif // PDF_FONT_OBJECT_H
