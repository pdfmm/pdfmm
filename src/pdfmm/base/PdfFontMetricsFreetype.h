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

class PDFMM_API PdfFontMetricsFreetype final : public PdfFontMetrics
{
public:
    /** Create a font metrics object for a given true type file
     *  \param library handle to an initialized FreeType2 library handle
     *  \param filename filename of a truetype file
     *  \param isSymbol whether use a symbol encoding, rather than unicode
     */
    PdfFontMetricsFreetype(FT_Library library, const std::string_view& filename,
        bool isSymbol);

    /** Create a font metrics object for a given memory buffer
     *  \param library handle to an initialized FreeType2 library handle
     *  \param buffer block of memory representing the font data (PdfFontMetricsFreetype will copy the buffer)
     *  \param size the length of the buffer
     *  \param isSymbol whether use a symbol encoding, rather than unicode
     */
    PdfFontMetricsFreetype(FT_Library library, const char* buffer, size_t size,
        bool isSymbol);

    /** Create a font metrics object for a given freetype font.
     *  \param library handle to an initialized FreeType2 library handle
     *  \param face a valid freetype font face
     *  \param isSymbol whether use a symbol encoding, rather than unicode
     */
    PdfFontMetricsFreetype(FT_Library library, FT_Face face, bool isSymbol);

    /** Create a font metrics object suitable for subsetting for a given true type file
     *  \param library handle to an initialized FreeType2 library handle
     *  \param filename filename of a truetype file
     *  \param isSymbol whether use a symbol encoding, rather than unicode
     */
    static PdfFontMetricsFreetype* CreateForSubsetting(FT_Library library, const std::string_view& filename,
        bool isSymbol);

    ~PdfFontMetricsFreetype();

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

    unsigned int GetWeight() const override;

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
    PdfFontMetricsFreetype(FT_Library library, const std::shared_ptr<chars>& buffer,
        bool isSymbol);

private:
    /** Initialize this object from an in memory buffer
     *  Called internally by the constructors
      * \param isSymbol Whether use a symbol charset, rather than unicode
     */
    void InitFromBuffer(bool isSymbol);

    /** Load the metric data from the FTFace data
     *		Called internally by the constructors
      * \param isSymbol Whether use a symbol charset, rather than unicode
     */
    void InitFromFace(bool isSymbol);

    void InitFontSizes();

 protected:
    FT_Library m_Library;
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

    std::shared_ptr<chars> m_FontData;
    std::vector<double> m_Widths;
};

};

#endif // PDF_FONT_METRICS_FREETYPE_H
