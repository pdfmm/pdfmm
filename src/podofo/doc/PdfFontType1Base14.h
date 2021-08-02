/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_TYPE1_BASE14_H
#define PDF_FONT_TYPE1_BASE14_H

#include "podofo/base/PdfDefines.h"
#include "PdfFontSimple.h"
#include <unordered_map>

namespace PoDoFo {

// TODO: Rename to PdfFontStandard14
/** A PdfFont implementation that can be used
 *  draw with base14 type1 fonts into a PDF file. 
 */
class PdfFontType1Base14 final : public PdfFont
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
     *  
     */
     PdfFontType1Base14(PdfDocument& doc, PdfStd14FontType fontType,
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
    PdfFontType1Base14(PdfObject& obj, PdfStd14FontType baseFont,
        const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    static std::string_view GetStandard14FontName(PdfStd14FontType stdFont);
    static bool IsStandard14Font(const std::string_view& fontName, PdfStd14FontType& baseFont);
    PdfStd14FontType GetStd14Type() const { return m_FontType; }

    PdfFontType GetType() const override;

protected:
    bool TryMapCIDToGID(unsigned cid, unsigned& gid) const override;
    bool TryMapGIDToCID(unsigned gid, unsigned& cid) const override;
    void initImported() override;

private:
    PdfStd14FontType m_FontType;
};

};

#endif // PDF_FONT_TYPE1_BASE14_H

