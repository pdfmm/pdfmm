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

string PdfFontConfigWrapper::GetFontConfigFontPath(const string_view fontName,
    PdfFontStyle style, unsigned& faceIndex)
{
    FcPattern* pattern;
    FcPattern* matched;
    FcResult result = FcResultMatch;
    FcValue value;

    bool isItalic = (style & PdfFontStyle::Italic) == PdfFontStyle::Italic;
    bool isBold = (style & PdfFontStyle::Bold) == PdfFontStyle::Bold;

    // Build a pattern to search using postscript name, bold and italic
    pattern = FcPatternBuild(0, FC_POSTSCRIPT_NAME, FcTypeString, fontName.data(),
        FC_WEIGHT, FcTypeInteger, (isBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM),
        FC_SLANT, FcTypeInteger, (isItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN),
        static_cast<char*>(0));

    FcDefaultSubstitute(pattern);

    if (!FcConfigSubstitute(m_FcConfig, pattern, FcMatchFont))
    {
        FcPatternDestroy(pattern);
        faceIndex = 0;
        return { };
    }

    string path;
    matched = FcFontMatch(m_FcConfig, pattern, &result);
    if (result != FcResultNoMatch)
    {
        (void)FcPatternGet(matched, FC_FILE, 0, &value);
        path = reinterpret_cast<const char*>(value.u.s);
        (void)FcPatternGet(matched, FC_INDEX, 0, &value);
        faceIndex = (unsigned)value.u.i;
#ifdef PDFMM_VERBOSE_DEBUG
        PdfError::LogMessage(PdfLogSeverity::Debug,
            "Got Font {}, face index {} for {}", path, faceIndex, fontName);
#endif // PDFMM_VERBOSE_DEBUG
    }

    FcPatternDestroy(pattern);
    FcPatternDestroy(matched);
    return path;
}

FcConfig* PdfFontConfigWrapper::GetFcConfig()
{
    return m_FcConfig;
}
