/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontMetrics.h"

#include <sstream>

#include <freetype/freetype.h>
#include <freetype/tttables.h>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfFontFactory.h"

using namespace std;
using namespace mm;

PdfFontMetrics::PdfFontMetrics(PdfFontMetricsType fontType,
    const string_view& filename)
    : m_FontType(fontType), m_Filename(filename) { }

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

PdfFontMetricsType PdfFontMetrics::GetFontMetricsTypeFromFilename(const string_view& filename)
{
    PdfFontMetricsType fontType = PdfFontMetricsType::Unknown;

    // We check by file extension right now
    // which is not quite correct, but still better than before
    if (filename.length() > 3)
    {
        const char* extension = filename.data() + filename.length() - 3;
        if (compat::strncasecmp(extension, "ttf", 3) == 0)
            fontType = PdfFontMetricsType::TrueType;
        else if (compat::strncasecmp(extension, "otf", 3) == 0)
            fontType = PdfFontMetricsType::TrueType;
        else if (compat::strncasecmp(extension, "ttc", 3) == 0)
            fontType = PdfFontMetricsType::TrueType;
        else if (compat::strncasecmp(extension, "pfa", 3) == 0)
            fontType = PdfFontMetricsType::Type1Pfa;
        else if (compat::strncasecmp(extension, "pfb", 3) == 0)
            fontType = PdfFontMetricsType::Type1Pfb;
    }

    if (fontType == PdfFontMetricsType::Unknown)
        PdfError::LogMessage(LogSeverity::Debug, "Warning: Unrecognized FontFormat: {}", filename);

    return fontType;
}

string PdfFontMetrics::GetFontNameSafe() const
{
    string fontName = GetFontName();
    if (!fontName.empty())
        return fontName;

    return GetBaseFontName();
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

bool PdfFontMetrics::FontNameHasBoldItalicInfo() const
{
    return false;
}
