/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_METRICS_FREETYPE_H
#define PDF_FONT_METRICS_FREETYPE_H

#include "PdfDefines.h"

#include "Pdf3rdPtyForwardDecl.h"
#include "PdfFontMetrics.h"
#include "PdfString.h"

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
    PdfFontMetricsFreetype(const std::shared_ptr<chars>& buffer,
        nullable<const PdfFontMetrics&> refMetrics = { });

    ~PdfFontMetricsFreetype();

public:
    static std::unique_ptr<PdfFontMetricsFreetype> FromBuffer(const cspan<char>& buffer);

    static std::unique_ptr<PdfFontMetricsFreetype> FromFace(FT_Face face);

    std::unique_ptr<PdfCMapEncoding> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const override;

    PdfFontDescriptorFlags GetFlags() const override;

    unsigned GetGlyphCount() const override;

    bool TryGetGlyphWidth(unsigned gid, double& width) const override;

    bool TryGetGID(char32_t codePoint, unsigned& gid) const override;

    double GetDefaultWidth() const override;

    void GetBoundingBox(std::vector<double>& bbox) const override;

    double GetLineSpacing() const override;

    double GetUnderlineThickness() const override;

    double GetUnderlinePosition() const override;

    double GetStrikeOutPosition() const override;

    double GetStrikeOutThickness() const override;

    double GetAscent() const override;

    double GetDescent() const override;

    std::string GetBaseFontName() const override;

    std::string GetFontName() const override;

    int GetWeightRaw() const override;

    double GetCapHeight() const override;

    double GetXHeightRaw() const override;

    double GetStemV() const override;

    double GetStemHRaw() const override;

    double GetItalicAngle() const override;

    PdfFontFileType GetFontFileType() const override;

    cspan<char> GetFontFileData() const override;

    /** Get direct access to the internal FreeType handle
     *
     *  \returns the internal freetype handle
     */
    inline FT_Face GetFace() const { return m_Face; }

protected:
    bool getIsBoldHint() const override;

    bool getIsItalicHint() const override;

private:
    PdfFontMetricsFreetype(FT_Face face, const std::shared_ptr<chars>& buffer);

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

    double m_Ascent;
    double m_Descent;
    int m_Weight;
    double m_CapHeight;
    double m_XHeight;
    double m_ItalicAngle;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;
    double m_DefaultWidth;
    PdfFontDescriptorFlags m_Flags;

    std::shared_ptr<chars> m_FontData;
    std::string m_fontName;
    std::string m_baseFontName;

    bool m_HasSymbolCharset;
};

};

#endif // PDF_FONT_METRICS_FREETYPE_H
