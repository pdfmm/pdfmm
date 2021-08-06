/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_FACTORY_BASE14_DATA_H
#define PDF_FONT_FACTORY_BASE14_DATA_H

#include "PdfDefinesPrivate.h"
#include <unordered_map>

namespace PoDoFo {

typedef std::unordered_map<unsigned short, unsigned short> Std14CPToGIDMap;

struct Base14FontData
{
    unsigned short CodePoint;
    unsigned short Width;
};

extern const Base14FontData CHAR_DATA_TIMES_ROMAN[315];

extern const Base14FontData CHAR_DATA_TIMES_ITALIC[315];

extern const Base14FontData CHAR_DATA_TIMES_BOLD[315];

extern const Base14FontData CHAR_DATA_TIMES_BOLD_ITALIC[315];

extern const Base14FontData CHAR_DATA_HELVETICA[315];

extern const Base14FontData CHAR_DATA_HELVETICA_OBLIQUE[315];

extern const Base14FontData CHAR_DATA_HELVETICA_BOLD[315];

extern const Base14FontData CHAR_DATA_HELVETICA_BOLD_OBLIQUE[315];

extern const Base14FontData CHAR_DATA_COURIER[315];

extern const Base14FontData CHAR_DATA_COURIER_OBLIQUE[315];

extern const Base14FontData CHAR_DATA_COURIER_BOLD[315];

extern const Base14FontData CHAR_DATA_COURIER_BOLD_OBLIQUE[315];

extern const Base14FontData CHAR_DATA_SYMBOL[190];

extern const Base14FontData CHAR_DATA_ZAPF_DINGBATS[202];

std::string_view GetStandard14FontName(PdfStd14FontType stdFont);

bool IsStandard14Font(const std::string_view& fontName, PdfStd14FontType& stdFont);

const Base14FontData* GetStd14FontData(PdfStd14FontType stdFont, unsigned& size);

const Std14CPToGIDMap& GetStd14CPToGIDMap(PdfStd14FontType stdFont);

};

#endif // PDF_FONT_FACTORY_BASE14_DATA_H
