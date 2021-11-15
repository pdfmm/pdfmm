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
     *  \param isSymbol whether use a symbol encoding, rather than unicode
     */
    PdfFontMetricsFreetype(const std::shared_ptr<chars>& buffer, bool isSymbol);

    ~PdfFontMetricsFreetype();

public:
    static std::unique_ptr<PdfFontMetricsFreetype> FromBuffer(const std::string_view& buffer, bool isSymbol);

    static std::unique_ptr<PdfFontMetricsFreetype> FromFace(FT_Face face, bool isSymbol);

    /** Create a best effort /ToUnicode map based on the
     * character unicode maps of the font
     *
     * This map may be unreliable because of ligatures,
     * other kind of character subsitutions, or glyphs
     * mapping to multiple unicode codepoints
     */
    std::unique_ptr<PdfEncodingMap> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const;

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

    std::string GetFontName() const override;

    unsigned GetWeight() const override;

    double GetCapHeight() const override;

    double GetXHeight() const override;

    double GetStemV() const override;

    double GetItalicAngle() const override;

    bool IsSymbol() const override;

    std::string_view GetFontData() const override;

    bool IsBold() const override;

    bool IsItalic() const override;

    /** Get direct access to the internal FreeType handle
     *
     *  \returns the internal freetype handle
     */
    inline FT_Face GetFace() const { return m_Face; }

private:
    PdfFontMetricsFreetype(FT_Face face, const std::shared_ptr<chars>& buffer, bool isSymbol);

    /** Initialize this object from an in memory buffer
     *  Called internally by the constructors
      * \param isSymbol Whether use a symbol charset, rather than unicode
     */
    void InitFromBuffer();

    /** Load the metric data from the FTFace data
     *		Called internally by the constructors
      * \param isSymbol Whether use a symbol charset, rather than unicode
     */
    void InitFromFace();

    void InitFontSizes();

 protected:
    FT_Face m_Face;

 private:
    bool m_IsSymbol;         // Internal member to singnal a symbol font
    bool m_IsBold;
    bool m_IsItalic;

    unsigned m_Weight;
    double m_ItalicAngle;

    double m_Ascent;
    double m_Descent;

    double m_LineSpacing;
    double m_UnderlineThickness;
    double m_UnderlinePosition;
    double m_StrikeOutThickness;
    double m_StrikeOutPosition;
    double m_CapHeight;
    double m_XHeight;

    std::shared_ptr<chars> m_FontData;
    std::vector<double> m_Widths;
    std::string m_fontName;
    std::string m_baseFontName;
};

};

#endif // PDF_FONT_METRICS_FREETYPE_H
