/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_STANDARD14_H
#define PDF_FONT_STANDARD14_H

#include "PdfDefines.h"

#include <unordered_map>

#include "PdfFontSimple.h"

namespace mm {

/**
 * A PdfFont implementation that represents a
 * standard 14 type1 font
 */
class PdfFontStandard14 final : public PdfFontSimple
{
    friend class PdfFont;

private:
    /** Create a new Type1 font object.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *
     */
    PdfFontStandard14(PdfDocument& doc, PdfStandard14FontType fontType,
        const PdfEncoding& encoding);

    /** Create a new Type1 font object based on an existing PdfObject
     *
     *  \param obj an existing PdfObject
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     */
    PdfFontStandard14(PdfObject& obj, PdfStandard14FontType baseFont,
        const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    static std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);

    /** Determine if font name is a Standard14 font
     *
     * By default use both standard names and alternative ones (Arial, TimesNewRoman, CourierNew)
     * \param fontName the unprocessed font name
     */
    static bool IsStandard14Font(const std::string_view& fontName, PdfStandard14FontType& stdFont);

    /** Determine if font name is a Standard14 font
     * \param fontName the unprocessed font name
     */
    static bool IsStandard14Font(const std::string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont);

    /** Try get a standard14 font from a base font name, representing the family and bold/italic characteristic
     *
     * By default use only standard names, *not* alternative ones (Arial, TimesNewRoman, CourierNew) 
     * \param baseFontName the processed font name
     */
    static bool TryGetStandard14Font(const std::string_view& baseFontName, bool bold, bool italic,
        PdfStandard14FontType& stdFont);

    /** Try get a standard14 font from a base font name, representing the family and bold/italic characteristic
     * \param baseFontName the processed font name
     * \param useAltNames use also the alternative names (Arial, TimesNewRoman, CourierNew) 
     */
    static bool TryGetStandard14Font(const std::string_view& baseFontName, bool bold, bool italic,
        bool useAltNames, PdfStandard14FontType& stdFont);

    PdfStandard14FontType GetStd14Type() const { return m_FontType; }

    PdfFontType GetType() const override;

    /** Return the encoding map for the given standard font type or nullptr for unknown
     */
    static PdfEncodingMapConstPtr GetStandard14FontEncodingMap(PdfStandard14FontType stdFont);

protected:
    bool TryMapCIDToGID(unsigned cid, unsigned& gid) const override;
    bool TryMapGIDToCID(unsigned gid, unsigned& cid) const override;
    void initImported() override;

private:
    PdfStandard14FontType m_FontType;
};

};

#endif // PDF_FONT_STANDARD14_H
