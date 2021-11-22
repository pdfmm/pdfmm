/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontMetrics.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"

using namespace std;
using namespace mm;

PdfFontMetrics::PdfFontMetrics() { }

PdfFontMetrics::~PdfFontMetrics() { }

double PdfFontMetrics::GetGlyphWidth(unsigned gid) const
{
    double width;
    if (!TryGetGlyphWidth(gid, width))
        return GetDefaultCharWidth();

    return width;
}

void PdfFontMetrics::SubstituteGIDs(vector<unsigned>& gids, vector<unsigned char>& backwardMap) const
{
    // By default do nothing and return a map to
    backwardMap.resize(gids.size(), 1);
    // TODO: Try to implement the mechanism in some font type
}

unsigned PdfFontMetrics::GetGID(char32_t codePoint) const
{
    unsigned gid;
    if (!TryGetGID(codePoint, gid))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Can't find a gid");

    return gid;
}

const PdfObject* PdfFontMetrics::GetFontFileObject() const
{
    // Return nullptr by default
    return nullptr;
}

string_view PdfFontMetrics::GetFontFileData() const
{
    // Return nothing by default
    return { };
}

string PdfFontMetrics::GetFontNameSafe(bool baseFirst) const
{
    if (baseFirst)
    {
        string baseFontName = GetBaseFontName();
        if (!baseFontName.empty())
            return baseFontName;

        return GetFontName();
    }
    else
    {
        string fontName = GetFontName();
        if (!fontName.empty())
            return fontName;

        return GetBaseFontName();
    }
}

string PdfFontMetrics::GetBaseFontName() const
{
    // By default return empty string
    return string();
}

string PdfFontMetrics::GetFontName() const
{
    // By default return empty string
    return string();
}

bool PdfFontMetrics::IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const
{
    std14Font = PdfStandard14FontType::Unknown;
    return false;
}

bool PdfFontMetrics::FontNameHasBoldItalicInfo() const
{
    return false;
}

bool PdfFontMetrics::IsType1Kind() const
{
    switch (GetFontFileType())
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::Type1CCF:
        case PdfFontFileType::CIDType1CCF:
            return true;
        default:
            return false;
    }
}

unique_ptr<PdfEncodingMap> PdfFontMetrics::CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const
{
    (void)limitHints;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}
