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
}

PdfFontConfigWrapper::~PdfFontConfigWrapper()
{
    unique_lock<mutex> lock(m_mutex);
    if (m_FcConfig != nullptr)
        FcConfigDestroy(m_FcConfig);
}

void PdfFontConfigWrapper::InitializeFontConfig()
{
    unique_lock<mutex> lock(m_mutex);
    if (m_FcConfig != nullptr)
        return;

    // Default initialize FontConfig
    FcInit();
    m_FcConfig = FcConfigGetCurrent();
}

std::string PdfFontConfigWrapper::GetFontConfigFontPath(const string_view fontName, bool bold, bool italic)
{
    InitializeFontConfig();

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
        PdfError::LogMessage(eLogSeverity_Debug,
            "Got Font %s for for %s\n", path.c_str(), pszFontName);
#endif // PDFMM_DEBUG
    }

    FcPatternDestroy(pattern);
    FcPatternDestroy(matched);
    return path;
}

FcConfig* PdfFontConfigWrapper::GetFcConfig()
{
    InitializeFontConfig();
    return m_FcConfig;
}

PdfFontConfigWrapper* PdfFontConfigWrapper::GetInstance()
{
    static PdfFontConfigWrapper wrapper;
    return &wrapper;
}
