/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_STANDARD14_H
#define PDF_FONT_STANDARD14_H

#include "PdfDefines.h"

#include <unordered_map>

#include "PdfFontSimple.h"

namespace mm {

/**
 * A PdfFont implementation that represents a
 * standard 14 type1 font
 */
class PdfFontStandard14 final : public PdfFont
{
    friend class PdfFont;

private:
    /** Create a new Type1 font object.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *
     */
    PdfFontStandard14(PdfDocument& doc, PdfStandard14FontType fontType,
        const PdfEncoding& encoding);

    /** Create a new Type1 font object based on an existing PdfObject
     *
     *  \param obj an existing PdfObject
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     */
    PdfFontStandard14(PdfObject& obj, PdfStandard14FontType baseFont,
        const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    static std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);
    static bool IsStandard14Font(const std::string_view& fontName, PdfStandard14FontType& baseFont);
    PdfStandard14FontType GetStd14Type() const { return m_FontType; }

    PdfFontType GetType() const override;

protected:
    bool TryMapCIDToGID(unsigned cid, unsigned& gid) const override;
    bool TryMapGIDToCID(unsigned gid, unsigned& cid) const override;
    void initImported() override;

private:
    PdfStandard14FontType m_FontType;
};

};

#endif // PDF_FONT_STANDARD14_H
