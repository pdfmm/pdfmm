/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_METRICS_OBJECT_H
#define PDF_FONT_METRICS_OBJECT_H

#include "PdfDefines.h"

#include <array>

#include "PdfFontMetrics.h"
#include "PdfArray.h"
#include "PdfName.h"
#include "PdfString.h"


namespace mm {

class PdfArray;
class PdfObject;
class PdfVariant;

class PDFMM_API PdfFontMetricsObject final : public PdfFontMetrics
{
    friend class PdfFont;

private:
    /** Create a font metrics object based on an existing PdfObject
     *
     *  \param obj an existing font descriptor object
     *  \param pEncoding a PdfEncoding which will NOT be owned by PdfFontMetricsObject
     */
    PdfFontMetricsObject(const PdfObject& font, const PdfObject* descriptor = nullptr);

public:
    unsigned GetGlyphCount() const override;

    bool TryGetGlyphWidth(unsigned gid, double& width) const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    double GetDefaultCharWidth() const override;

    void GetBoundingBox(std::vector<double>& bbox) const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeOutPosition() const override;

    double GetStrikeOutThickness() const override;

    double GetAscent() const override;

    double GetDescent() const override;

    std::string GetFontName() const override;

    unsigned GetWeight() const override;

    double GetItalicAngle() const override;

    bool IsSymbol() const override;

    std::string_view GetFontData() const override;

    bool IsBold() const override;

    bool IsItalic() const override;

private:
    std::vector<double> GetBBox(const PdfObject& obj);

private:
    std::string m_FontName;
    std::vector<double> m_BBox;
    std::array<double, 6> m_matrix;
    std::vector<double> m_Widths;
    double m_DefaultWidth;
    unsigned m_Weight;
    double m_ItalicAngle;
    double m_Ascent;
    double m_Descent;
    double m_LineSpacing;

    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;

    bool m_IsSymbol;     // Internal member to singnal a symbol font
};

};

#endif // PDF_FONT_METRICS_OBJECT_H
