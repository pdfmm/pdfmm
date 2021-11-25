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
#include "PdfCMapEncoding.h"

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
    PdfFontMetrics();

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

    virtual PdfFontDescriptorFlags GetFlags() const = 0;

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
     * \returns the ascender for this font
     *
     * \see GetAscent
     */
    virtual double GetAscent() const = 0;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
    
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual double GetDescent() const = 0;

    virtual PdfFontFileType GetFontFileType() const = 0;

    /** Get the actual font data for a file imported font, if available
     *
     * For font data coming from the /FontFile keys, use GetFontFileObject()
     * \returns a binary buffer of data containing the font data
     */
    virtual std::string_view GetFontFileData() const;

    /** Get the actual font file object from a /FontFile like key, if available
     *
     * For font data coming from a file imported font, see GetFontFileData()
     * \returns a binary buffer of data containing the font data
     */
    virtual const PdfObject* GetFontFileObject() const;

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

    /** The thickness, measured horizontally, of the dominant vertical stems of glyphs in the font
     */
    virtual double GetStemV() const = 0;

    /** Get the weight of this font.
     *  \returns the weight of this font (400 <= x < 700 means normal, x >= 700 means bold)
     *
     * Negative if absent
     */
    virtual int GetWeight() const = 0;

    /** The vertical coordinate of the top of flat capital letters, measured from the baseline
     *
     * Negative if absent
     */
    virtual double GetCapHeight() const = 0;

    /** The font’s x height: the vertical coordinate of the top of flat nonascending
     * lowercase letters (like the letter x), measured from the baseline, in
     * fonts that have Latin characters
     *
     * Negative if absent
     */
    virtual double GetXHeight() const = 0;

    /** The thickness, measured vertically, of the dominant horizontal stems of glyphs in the font
     *
     * Negative if absent
     */
    virtual double GetStemH() const = 0;

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

    bool IsStandard14FontMetrics() const;

    virtual bool IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const;

    /** Determine if the metrics are for Adobe Type1 like font
     */
    bool IsType1Kind() const;

    /** Determine if the font is non symbolic according to the PDF definition
     *
     * The font is symbolic if "contains glyphs outside the Standard Latin character set"
     */
    bool IsPdfSymbolic() const;

    /** Determine if the font is symbolic according to the PDF definition
     *
     * The font is symbolic if "uses the Standard Latin character set or a subset of it."
     */
    bool IsPdfNonSymbolic() const;

    /** Create a best effort /ToUnicode map based on the
     * character unicode maps of the font
     *
     * Thi is implemented just for PdfFontMetricsFreetype
     * This map may be unreliable because of ligatures,
     * other kind of character subsitutions, or glyphs
     * mapping to multiple unicode codepoints.
     */
    virtual std::unique_ptr<PdfCMapEncoding> CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const;

private:
    PdfFontMetrics(const PdfFontMetrics& rhs) = delete;
    PdfFontMetrics& operator=(const PdfFontMetrics& rhs) = delete;
};

/** Convenience typedef for a const PdfEncoding shared ptr
 */
typedef std::shared_ptr<const PdfFontMetrics> PdfFontMetricsConstPtr;

};

#endif // PDF_FONT_METRICS_H
