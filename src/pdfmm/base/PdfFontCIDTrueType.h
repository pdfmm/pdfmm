/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_CID_H
#define PDF_FONT_CID_H

#include "PdfDeclarations.h"
#include "PdfFont.h"

namespace mm {

/** A PdfFont that represents a CID-keyed font that has a TrueType font backend
 */
// TODO: Move the common code to a PDfFontCID class and create a PdfFontCIDType1 class
class PdfFontCIDTrueType final : public PdfFont
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
    bool SupportsSubsetting() const override;
    PdfFontType GetType() const override;

protected:
    void embedFont() override;
    void embedFontSubset() override;
    PdfObject* getDescendantFontObject() override;

private:
    void embedFontFile(PdfObject& descriptor);
    void createWidths(PdfDictionary& fontDict, const CIDToGIDMap& glyphWidths);
    CIDToGIDMap getIdentityCIDToGIDMap();
    static CIDToGIDMap getCIDToGIDMapSubset(const UsedGIDsMap& usedGIDs);

protected:
    void initImported() override;

private:
    PdfObject* m_descendantFont;
    PdfObject* m_descriptor;
};

};

#endif // PDF_FONT_CID_H
