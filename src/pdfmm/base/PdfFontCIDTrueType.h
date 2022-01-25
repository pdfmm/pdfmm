/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_CID_TRUE_TYPE_H
#define PDF_FONT_CID_TRUE_TYPE_H

#include "PdfFontCID.h"

namespace mm {

/** A PdfFont that represents a CID-keyed font that has a TrueType font backend
 */
class PdfFontCIDTrueType final : public PdfFontCID
{
    friend class PdfFont;

private:
    /** Create a new CID font.
     *
     *  \param parent parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font
     */
    PdfFontCIDTrueType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    PdfFontType GetType() const override;

protected:
    void embedFontSubset() override;
};

};

#endif // PDF_FONT_CID_TRUE_TYPE_H
