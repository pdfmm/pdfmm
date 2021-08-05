/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_FACTORY_H
#define PDF_FONT_FACTORY_H

#include "podofo/base/PdfDefines.h"
#include "PdfFont.h"

namespace PoDoFo {

class PdfFontMetrics;
class PdfVecObjects;

struct PdfFontInitParams
{
    bool Bold = false;
    bool Italic = false;
    bool Embed = false;
    bool Subsetting = false; // Subsetting implies embed
};

/** This is a factory class which knows
 *  which implementation of PdfFont is required
 *  for a certain font type with certain features (like encoding).
 */
class PODOFO_DOC_API PdfFontFactory final
{
public:
    /** Create a new PdfFont object.
     *
     *  \param doc the parent of the created font.
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the created font. In case of an error, it is deleted
     *         here.
     *  \param nFlags font flags or'ed together, specifying the font style and if it should be embedded
     *  \param pEncoding the encoding of this font.
     *
     *  \returns a new PdfFont object or nullptr
     */
    static PdfFont* CreateFontObject(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding, const PdfFontInitParams& params);

    /** Create a new PdfFont from an existing
     *  font in a PDF file.
     *
     *  \param pLibrary handle to the FreeType library, so that a PdfFontMetrics
     *         can be constructed for this font
     *  \param obj a PDF font object
     */
    static PdfFont* CreateFont(PdfObject& obj);

    /**
     *    Creates a new base-14 font object (of class PdfFontType1Base14) if
     *    the font name (has to include variant) is one of the base 14 fonts.
     *    The font name is to be given as specified (with an ASCII hyphen).
     *
     *    \param doc the parent of the created font
     *    \param fontName ASCII C string (zero-terminated) of the font name
     *    \param eFlags one flag for font variant (Bold, Italic or BoldItalic)
     *    \param encoding an encoding compatible with the font
     */
    static PdfFont* CreateBase14Font(PdfDocument& doc, PdfStd14FontType baseFont,
        const PdfEncoding& encoding, const PdfFontInitParams& params);

private:
    PdfFontFactory() = delete;

    static PdfFont* createFontForType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding, PdfFontMetricsType type, bool subsetting);
};

};

#endif // PDF_FONT_FACTORY_H
