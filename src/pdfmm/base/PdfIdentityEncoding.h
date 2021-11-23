/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_IDENTITY_ENCODING_H
#define PDF_IDENTITY_ENCODING_H

#include "PdfEncodingMap.h"
#include "PdfObject.h"

namespace mm {

/** Orientation for predefined CID identity encodings
 */
enum class PdfIdentityOrientation
{
    Unkwnown = 0,
    Horizontal, // Corresponts to /Identity-H
    Vertical,   // Corresponts to /Identity-V
};

/** PdfIdentityEncoding is a two-byte encoding which can be
 *  used with TrueType fonts to represent all characters
 *  present in a font. If the font contains all unicode
 *  glyphs, PdfIdentityEncoding will support all unicode
 *  characters.
 */
class PDFMM_API PdfIdentityEncoding final : public PdfEncodingMap
{
public:
    /**
     *  Create a new PdfIdentityEncoding.
     *
     *  \param codeSpaceSize size of the codespace size
     */
    PdfIdentityEncoding(unsigned char codeSpaceSize);

    /**
     *  Create a standard 2 bytes CID PdfIdentityEncoding
     */
    PdfIdentityEncoding(PdfIdentityOrientation orientation);

protected:
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;
    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    void appendBaseFontEntries(PdfStream& stream) const override;

public:
    bool IsCMapEncoding() const override;

private:
    PdfIdentityEncoding(unsigned char codeSpaceSize, PdfIdentityOrientation orientation);

private:
    PdfIdentityOrientation m_orientation;
};

};

#endif // PDF_IDENTITY_ENCODING_H
