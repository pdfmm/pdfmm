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
#include <pdfmm/base/PdfRect.h>

namespace mm {

struct Standard14FontChar;

typedef std::unordered_map<unsigned short, unsigned short> Std14CPToGIDMap;

std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);

bool IsStandard14Font(const std::string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont);

bool TryGetStandard14Font(const std::string_view& baseFontName, bool bold, bool italic, bool useAltNames, PdfStandard14FontType& baseFont);

const Standard14FontChar* GetStd14FontChars(PdfStandard14FontType stdFont, unsigned& size);

const Std14CPToGIDMap& GetStd14CPToGIDMap(PdfStandard14FontType stdFont);

std::string_view GetStandard14FontFileData(PdfStandard14FontType stdFont);

std::string_view GetStandard14FontBaseName(PdfStandard14FontType stdFont);

};

#endif // PDF_FONT_STANDARD14_DATA_H
