/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_METRICS_STANDARD14_H
#define PDF_FONT_METRICS_STANDARD14_H

#include "PdfDefines.h"

#include "PdfFontMetrics.h"
#include "PdfRect.h"
#include "PdfVariant.h"

namespace mm {

struct Standard14FontData;

/**
 * This is the main class to handle the Standard14 metric data.
 */
class PDFMM_API PdfFontMetricsStandard14 final : public PdfFontMetrics
{
private:
    PdfFontMetricsStandard14(PdfStandard14FontType fontType,
        const Standard14FontData* data, unsigned dataSize,
        bool isSymbol, int16_t ascent, int16_t descent,
        uint16_t mx_height, uint16_t mcap_height,
        int16_t mstrikeout_pos, int16_t munderline_pos,
        const PdfRect& mbbox);

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

    std::string GetBaseFontName() const override;

    unsigned GetWeight() const override;

    double GetItalicAngle() const override;

    bool IsSymbol() const override;

    std::string_view GetFontData() const override;

    bool IsBold() const override;

    bool IsItalic() const override;

    bool FontNameHasBoldItalicInfo() const override;

public:
    static std::shared_ptr<const PdfFontMetricsStandard14> GetInstance(PdfStandard14FontType font);

private:
    PdfStandard14FontType m_Std14FontType;
    const Standard14FontData* m_data;
    const unsigned m_dataSize;
    uint16_t m_x_height;
    uint16_t m_cap_height;
    PdfRect m_BBox;

    bool m_IsSymbol;

    unsigned m_Weight;
    double m_ItalicAngle;

    double m_Ascent;
    double m_Descent;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;
};

}

#endif // PDF_FONT_METRICS_STANDARD14_H
