/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_METRICS_FREETYPE_H
#define PDF_FONT_METRICS_FREETYPE_H

#include "PdfDeclarations.h"

#include "PdfFontMetrics.h"
#include "PdfString.h"

FORWARD_DECLARE_FTFACE();

namespace mm {

class PdfArray;
class PdfObject;
class PdfVariant;
class PdfEncodingMap;
struct PdfEncodingLimits;

class PDFMM_API PdfFontMetricsFreetype final : public PdfFontMetrics
{
public:
    /** Create a font metrics object for a given memory buffer
     *  \param buffer block of memory representing the font data (PdfFontMetricsFreetype will copy the buffer)
     */
    PdfFontMetricsFreetype(const std::shared_ptr<charbuff>& buffer,
        nullable<const PdfFontMetrics&> refMetrics = { });

    ~PdfFontMetricsFreetype();

public:
    static std::unique_ptr<PdfFontMetricsFreetype> FromBuffer(const bufferview& buffer);

    static std::unique_ptr<PdfFontMetricsFreetype> FromFace(FT_Face face);

    std::unique_ptr<PdfCMapEncoding> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const override;

    unsigned GetGlyphCount() const override;

    bool TryGetGlyphWidth(unsigned gid, double& width) const override;

    bool HasUnicodeMapping() const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    double GetDefaultWidthRaw() const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeOutPosition() const override;

    double GetStrikeOutThickness() const override;

    std::string GetBaseFontName() const override;

    std::string GetFontName() const override;

    std::string GetFontFamilyName() const override;

    PdfFontStretch GetFontStretch() const override;

    int GetWeightRaw() const override;

    PdfFontDescriptorFlags GetFlags() const override;

    void GetBoundingBox(std::vector<double>& bbox) const override;

    double GetItalicAngle() const override;

    double GetAscent() const override;

    double GetDescent() const override;

    double GetLeadingRaw() const override;

    double GetCapHeight() const override;

    double GetXHeightRaw() const override;

    double GetStemV() const override;

    double GetStemHRaw() const override;

    double GetAvgWidthRaw() const override;

    double GetMaxWidthRaw() const override;

    PdfFontFileType GetFontFileType() const override;

    bufferview GetFontFileData() const override;

    /** Get direct access to the internal FreeType handle
     *
     *  \returns the internal freetype handle
     */
    inline FT_Face GetFace() const { return m_Face; }

protected:
    bool getIsBoldHint() const override;

    bool getIsItalicHint() const override;

private:
    PdfFontMetricsFreetype(FT_Face face, const std::shared_ptr<charbuff>& buffer);

    /** Initialize this object from an in memory buffer
     * Called internally by the constructors
     */
    void initFromBuffer(const PdfFontMetrics* refMetrics);

    /** Load the metric data from the FTFace data
     * Called internally by the constructors
     */
    void initFromFace(const PdfFontMetrics* refMetrics);

private:
    FT_Face m_Face;
    PdfFontFileType m_FontFileType;

    std::string m_FontBaseName;
    std::string m_FontName;
    std::string m_FontFamilyName;
    PdfFontStretch m_FontStretch;
    int m_Weight;
    PdfFontDescriptorFlags m_Flags;
    double m_ItalicAngle;
    double m_Ascent;
    double m_Descent;
    double m_Leading;
    double m_CapHeight;
    double m_XHeight;
    double m_StemV;
    double m_StemH;
    double m_AvgWidth;
    double m_MaxWidth;
    double m_DefaultWidth;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;

    std::shared_ptr<charbuff> m_FontData;
    bool m_HasUnicodeMapping;
    bool m_HasSymbolCharset;
};

};

#endif // PDF_FONT_METRICS_FREETYPE_H
