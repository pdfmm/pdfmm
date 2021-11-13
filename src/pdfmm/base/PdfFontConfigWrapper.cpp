/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfFontConfigWrapper.h"

#include <fontconfig/fontconfig.h>

using namespace std;
using namespace mm;

PdfFontConfigWrapper::PdfFontConfigWrapper(FcConfig* fcConfig)
    : m_FcConfig(fcConfig)
{
    if (fcConfig == nullptr)
    {
        // Default initialize FontConfig
        FcInit();
        m_FcConfig = FcConfigGetCurrent();
    }
}

PdfFontConfigWrapper::~PdfFontConfigWrapper()
{
    FcConfigDestroy(m_FcConfig);
}

string PdfFontConfigWrapper::GetFontConfigFontPath(const string_view fontName, bool bold, bool italic)
{
    FcPattern* pattern;
    FcPattern* matched;
    FcResult result = FcResultMatch;
    FcValue v;

    string path;
    // Build a pattern to search using fontname, bold and italic
    pattern = FcPatternBuild(0, FC_FAMILY, FcTypeString, fontName.data(),
        FC_WEIGHT, FcTypeInteger, (bold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM),
        FC_SLANT, FcTypeInteger, (italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN),
        static_cast<char*>(0));

    FcDefaultSubstitute(pattern);

    if (!FcConfigSubstitute(m_FcConfig, pattern, FcMatchFont))
    {
        FcPatternDestroy(pattern);
        return path;
    }

    matched = FcFontMatch(m_FcConfig, pattern, &result);
    if (result != FcResultNoMatch)
    {
        result = FcPatternGet(matched, FC_FILE, 0, &v);
        path = reinterpret_cast<const char*>(v.u.s);
#ifdef PDFMM_VERBOSE_DEBUG
        PdfError::LogMessage(LogSeverity::Debug,
            "Got Font {} for for {}\n", path, fontName);
#endif // PDFMM_DEBUG
    }

    FcPatternDestroy(pattern);
    FcPatternDestroy(matched);
    return path;
}

FcConfig* PdfFontConfigWrapper::GetFcConfig()
{
    return m_FcConfig;
}
