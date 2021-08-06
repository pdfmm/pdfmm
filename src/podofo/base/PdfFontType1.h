/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_TYPE1_H
#define PDF_FONT_TYPE1_H

#include "PdfDefines.h"

#include <set>

#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embedd type1 fonts into a PDF file
 *  or to draw with type1 fonts.
 */
class PdfFontType1 final : public PdfFontSimple
{
    friend class PdfFontFactory;

private:

    /** Create a new Type1 font object.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param embed if true the font will get embedded.
     *  \param subsetting if true the font will use subsetting.
     *
     */
    PdfFontType1(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    bool SupportsSubsetting() const override;
    PdfFontType GetType() const override;

protected:
    void embedFontSubset() override;
    void embedFontFile(PdfObject& descriptor) override;
    void initImported() override;

private:
    bool FindSeac(const char* buffer, size_t length);

    ptrdiff_t FindInBuffer(const char* needle, const char* haystack, size_t len) const;
};

};

#endif // PDF_FONT_TYPE1_H

