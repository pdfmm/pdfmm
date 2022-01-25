/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_CID_TYPE1_H
#define PDF_FONT_CID_TYPE1_H

#include "PdfFontCID.h"

namespace mm {

/** A PdfFont that represents a CID-keyed font that has a Typ1 font backend
 */
class PdfFontCIDType1 final : public PdfFontCID
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
    PdfFontCIDType1(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    bool SupportsSubsetting() const override;
    PdfFontType GetType() const override;

protected:
    void embedFontSubset() override;
};

};

#endif // PDF_FONT_CID_TYPE1_H
