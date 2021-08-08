/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_STANDARD14_DATA_H
#define PDF_FONT_STANDARD14_DATA_H

#include "PdfDefinesPrivate.h"
#include <unordered_map>

namespace mm {

typedef std::unordered_map<unsigned short, unsigned short> Std14CPToGIDMap;

struct Standard14FontData
{
    unsigned short CodePoint;
    unsigned short Width;
};

extern const Standard14FontData CHAR_DATA_TIMES_ROMAN[315];

extern const Standard14FontData CHAR_DATA_TIMES_ITALIC[315];

extern const Standard14FontData CHAR_DATA_TIMES_BOLD[315];

extern const Standard14FontData CHAR_DATA_TIMES_BOLD_ITALIC[315];

extern const Standard14FontData CHAR_DATA_HELVETICA[315];

extern const Standard14FontData CHAR_DATA_HELVETICA_OBLIQUE[315];

extern const Standard14FontData CHAR_DATA_HELVETICA_BOLD[315];

extern const Standard14FontData CHAR_DATA_HELVETICA_BOLD_OBLIQUE[315];

extern const Standard14FontData CHAR_DATA_COURIER[315];

extern const Standard14FontData CHAR_DATA_COURIER_OBLIQUE[315];

extern const Standard14FontData CHAR_DATA_COURIER_BOLD[315];

extern const Standard14FontData CHAR_DATA_COURIER_BOLD_OBLIQUE[315];

extern const Standard14FontData CHAR_DATA_SYMBOL[190];

extern const Standard14FontData CHAR_DATA_ZAPF_DINGBATS[202];

std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);

bool IsStandard14Font(const std::string_view& fontName, PdfStandard14FontType& stdFont);

const Standard14FontData* GetStd14FontData(PdfStandard14FontType stdFont, unsigned& size);

const Std14CPToGIDMap& GetStd14CPToGIDMap(PdfStandard14FontType stdFont);

};

#endif // PDF_FONT_STANDARD14_DATA_H
