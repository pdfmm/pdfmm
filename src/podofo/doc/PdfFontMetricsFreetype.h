/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_FONT_METRICS_FREETYPE_H_
#define _PDF_FONT_METRICS_FREETYPE_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/Pdf3rdPtyForwardDecl.h"
#include "podofo/base/PdfString.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfArray;
class PdfObject;
class PdfVariant;

class PODOFO_DOC_API PdfFontMetricsFreetype : public PdfFontMetrics {
 public:
    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pszFilename filename of a truetype file
	  *  \param pIsSymbol whether use a symbol encoding, rather than unicode
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, const char* pszFilename, 
			 bool pIsSymbol, const char* pszSubsetPrefix = nullptr );

    /** Create a font metrics object for a given memory buffer
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pBuffer block of memory representing the font data (PdfFontMetricsFreetype will copy the buffer)
     *  \param nBufLen the length of the buffer
	  *  \param pIsSymbol whether use a symbol encoding, rather than unicode
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, const char* pBuffer, unsigned int nBufLen,
			 bool  pIsSymbol, const char* pszSubsetPrefix = nullptr);

    /** Create a font metrics object for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param rBuffer a buffer containing a font file
	  *  \param pIsSymbol whether use a symbol encoding, rather than unicode
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, const PdfRefCountedBuffer & rBuffer,
		    bool  pIsSymbol, const char* pszSubsetPrefix = nullptr);

    /** Create a font metrics object for a given freetype font.
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param face a valid freetype font face
	  *  \param pIsSymbol whether use a symbol encoding, rather than unicode
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, FT_Face face,
		    bool  pIsSymbol, const char* pszSubsetPrefix = nullptr);

    /** Create a font metrics object based on an existing PdfObject
     *
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pObject an existing font descriptor object
     */
    PdfFontMetricsFreetype( FT_Library* pLibrary, PdfObject* pDescriptor );

    /** Create a font metrics object suitable for subsetting for a given true type file
     *  \param pLibrary handle to an initialized FreeType2 library handle
     *  \param pszFilename filename of a truetype file
	 *  \param pIsSymbol whether use a symbol encoding, rather than unicode
     *  \param pszSubsetPrefix unique prefix for font subsets (see GetFontSubsetPrefix)
     */
    static PdfFontMetricsFreetype* CreateForSubsetting(FT_Library* pLibrary, const char* pszFilename, bool pIsSymbol, const char* pszSubsetPrefix );

    virtual ~PdfFontMetricsFreetype();

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     *  \param pEncoding encoding for correct character widths. If not passed default (latin1) encoding is used
     */
    void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding = nullptr ) const override;

    /** Get the width of a single glyph id
     *
     *  \param nGlyphId id of the glyph
     *  \returns the width of a single glyph id
     */
    double GetGlyphWidth( int nGlyphId ) const override;

    /** Get the width of a single named glyph
     *
     *  \param pszGlyphname name of the glyph
     *  \returns the width of a single named glyph
     */
	double GetGlyphWidth( const char* pszGlyphname ) const override;

    /** Create the bounding box array as required by the PDF reference
     *  so that it can be written directly to a PDF file.
     * 
     *  \param array write the bounding box to this array.
     */
    void GetBoundingBox( PdfArray & array ) const override;
    
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    double CharWidth( unsigned char c ) const override;

    // Peter Petrov 20 March 2009
    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    double UnicodeCharWidth( unsigned short c ) const override;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    double GetLineSpacing() const override;

    /** Get the width of the underline for the current 
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    double GetUnderlineThickness() const override;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetUnderlinePosition() const override;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetStrikeOutPosition() const override;

    /** Get the width of the strikeout for the current 
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    double GetStrikeoutThickness() const override;

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or nullptr string if no postscript name is available.
     */
    const char* GetFontname() const override;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    unsigned int GetWeight() const override;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *  
     *  \see GetPdfAscent
     */
    double GetAscent() const override;

    /** Get the ascent of this font
     *  Used to build the font dictionay
     *  \returns the ascender for this font
     *  
     *  \see GetAscent
     */
    double GetPdfAscent() const override;

    /** Get the descent of this font in PDF 
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetPdfDescent
     */
    double GetDescent() const override;

    /** Get the descent of this font
     *  Used to build the font dictionay
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    double GetPdfDescent() const override;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    int GetItalicAngle() const override;

    /** Get the glyph id for a unicode character
     *  in the current font.
     *
     *  \param lUnicode the unicode character value
     *  \returns the glyhph id for the character or 0 if the glyph was not found.
     */
    long GetGlyphId( long lUnicode ) const override;

    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking 
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     * 
     * \returns true if this is a symbol font
     */
    bool IsSymbol() const override;

    /** Get a pointer to the actual font data - if it was loaded from memory.
     *  \returns a binary buffer of data containing the font data
     */
    const char* GetFontData() const override;

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
    size_t GetFontDataLen() const override;

    /** Get whether the internal font style flags contain the Bold flag.
     *  \returns whether the Bold style flag is set on the font
     */
    bool IsBold(void) const;

    /** Get whether the internal font style flags contain the Italic flag.
     *  \returns whether the Italic style flag is set on the font
     */
    bool IsItalic(void) const;

    /** Get direct access to the internal FreeType handle
     * 
     *  \returns the internal freetype handle
     */
    inline FT_Face GetFace();
 
 private:
    
    /** Initialize this object from an in memory buffer
     *  Called internally by the constructors
	  * \param pIsSymbol Whether use a symbol charset, rather than unicode
     */
    void InitFromBuffer(bool pIsSymbol);

    /** Load the metric data from the FTFace data
     *		Called internally by the constructors
	  * \param pIsSymbol Whether use a symbol charset, rather than unicode
     */
    void InitFromFace(bool pIsSymbol);

    void InitFontSizes();
 protected:
    FT_Library*   m_pLibrary;
    FT_Face       m_pFace;

 private:
    bool          m_bSymbol;  ///< Internal member to singnal a symbol font
    bool          m_bIsBold;
    bool          m_bIsItalic;

    unsigned int  m_nWeight;
    int           m_nItalicAngle;

    double        m_dAscent;
    double        m_dPdfAscent;
    double        m_dDescent;
    double        m_dPdfDescent;

    double        m_dLineSpacing;
    double        m_dUnderlineThickness;
    double        m_dUnderlinePosition;
    double        m_dStrikeOutThickness;
    double        m_dStrikeOutPosition;

    PdfRefCountedBuffer m_bufFontData;
    std::vector<double> m_vecWidth;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
FT_Face PdfFontMetricsFreetype::GetFace() 
{ 
    return m_pFace; 
} 

 
};

#endif // _PDF_FONT_METRICS_FREETYPE_H_

