/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_METRICS_H
#define PDF_FONT_METRICS_H

#include "PdfDefines.h"

#include "PdfString.h"
#include "PdfEncoding.h"

namespace mm {

/**
 * This abstract class provides access to font metrics information.
 *
 * The class doesn't know anything about CIDs (Character IDs),
 * it just index glyphs, or GIDs where the terminology applies
 */
class PDFMM_API PdfFontMetrics
{
protected:
    PdfFontMetrics(PdfFontMetricsType fontType, const std::string_view& filename);

public:
    virtual ~PdfFontMetrics();

    virtual unsigned GetGlyphCount() const = 0;

    /** Get the width of a single glyph id
     *
     *  \param gid id of the glyph
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth(unsigned gid) const;
    virtual bool TryGetGlyphWidth(unsigned gid, double& width) const = 0;

    virtual double GetDefaultCharWidth() const = 0;

    /**
     * Some fonts provides a glyph subsitution list, eg. for ligatures.
     * OpenType fonts for example provides GSUB "Glyph Substitution Table"
     * \param gids gids to be substituded
     * \param backwardMap list of gid counts to remap back substituded gids
     *     eg. { 32, 102, 105 } gets substituted in { 32, 174 }
     *     the backward map is { 1, 2 }
     */
    virtual void SubstituteGIDs(std::vector<unsigned>& gids,
        std::vector<unsigned char>& backwardMap) const;

    /** Get the GID by the codePoint
     *
     *  \param codePoint unicode codepoint
     *  \returns the GID
     *  \remarks throw if not found
     */
    unsigned GetGID(char32_t codePoint) const;
    virtual bool TryGetGID(char32_t codePoint, unsigned& gid) const = 0;

    /** Create the bounding box vector in PDF units
     *
     *  \param bbox write the bounding box to this vector
     */
    virtual void GetBoundingBox(std::vector<double>& bbox) const = 0;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    virtual double GetLineSpacing() const = 0;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    virtual double GetUnderlineThickness() const = 0;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetUnderlinePosition() const = 0;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetStrikeOutPosition() const = 0;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    virtual double GetStrikeOutThickness() const = 0;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *
     *  \see GetAscent
     */
    virtual double GetAscent() const = 0;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual double GetDescent() const = 0;

    /** Get a pointer to the path of the font file.
     *  \returns a zero terminated string containing the filename of the font file
     */
    inline const std::string& GetFilename() const { return m_Filename; }

    /** Get the actual font data for a file loaded font, if available
     *
     * For font data coming from the /FontFile keys, use GetFontDataObject()
     * \returns a binary buffer of data containing the font data
     */
    virtual std::string_view GetFontData() const;

    /** Get the actual font data object from a /FontFile like key, if available
     *
     * For font data coming from a file loaded font, see GetFontData()
     * \returns a binary buffer of data containing the font data
     */
    virtual const PdfObject* GetFontDataObject() const;

    /** Get a string with either the actual /FontName or a base font name
     * inferred from a font file
     */
    std::string GetFontNameSafe(bool baseFirst = false) const;

    /** Get a base name for the font that can be used to compose the final name, eg. "Arial"
     *
     *  Return empty string by default
     */
    virtual std::string GetBaseFontName() const;

    /** Get the actual /FontName, eg. "AAAAAA+Arial,Bold", if available
     *
     *  By default return empty string
     *  \returns the postscript name of the font or empty string if no postscript name is available.
     */
    virtual std::string GetFontName() const;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    virtual unsigned GetWeight() const = 0;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    virtual double GetItalicAngle() const = 0;


    /** Get whether the font style is bold
     */
    virtual bool IsBold() const = 0;

    /** Get whether the font style is italic
     */
    virtual bool IsItalic() const = 0;

    /** State whether font name reports if the font is bold or italic, such has in "Helvetica-Bold"
     */
    virtual bool FontNameHasBoldItalicInfo() const;

    /**
     *  \returns the fonttype of the loaded font
     */
    inline PdfFontMetricsType GetType() const { return m_FontType; }

    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     *
     * \returns true if this is a symbol font
     */
    virtual bool IsSymbol() const = 0;

    /** Try to detect the internal fonttype from
     *  the file extension of a fontfile.
     *
     *  \param filename must be the filename of a font file
     *
     *  \return font type
     */
    static PdfFontMetricsType GetFontMetricsTypeFromFilename(const std::string_view& filename);

protected:
    /**
     *  Set the fonttype.
     *  \param eFontType fonttype
     */
    inline void SetFontType(PdfFontMetricsType eFontType) { m_FontType = eFontType; }

private:
    PdfFontMetrics(const PdfFontMetrics& rhs) = delete;
    PdfFontMetrics& operator=(const PdfFontMetrics& rhs) = delete;

protected:
    PdfFontMetricsType m_FontType;
    std::string m_Filename;
};

/** Convenience typedef for a const PdfEncoding shared ptr
 */
typedef std::shared_ptr<const PdfFontMetrics> PdfFontMetricsConstPtr;

};

#endif // PDF_FONT_METRICS_H
