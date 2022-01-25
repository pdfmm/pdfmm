/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_CID_H
#define PDF_FONT_CID_H

#include "PdfFont.h"

namespace mm {

/** A PdfFont that represents a CID-keyed font
 */
class PdfFontCID : public PdfFont
{
    friend class PdfFont;

protected:
    PdfFontCID(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    bool SupportsSubsetting() const override;

protected:
    void embedFont() override;
    PdfObject* getDescendantFontObject() override;
    void createWidths(PdfDictionary& fontDict, const CIDToGIDMap& glyphWidths);
    static CIDToGIDMap getCIDToGIDMapSubset(const UsedGIDsMap& usedGIDs);

private:
    CIDToGIDMap getIdentityCIDToGIDMap();

protected:
    void initImported() override;

protected:
    PdfObject& GetDescendantFont() { return *m_descendantFont; }
    PdfObject& GetDescriptor() { return *m_descriptor; }

private:
    PdfObject* m_descendantFont;
    PdfObject* m_descriptor;
};

};

#endif // PDF_FONT_CID_H
