/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetrics.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"

using namespace std;
using namespace mm;

// Default matrix: thousands of PDF units
static Matrix2D s_DefaultMatrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

PdfFontMetrics::PdfFontMetrics() { }

PdfFontMetrics::~PdfFontMetrics() { }

double PdfFontMetrics::GetGlyphWidth(unsigned gid) const
{
    double width;
    if (!TryGetGlyphWidth(gid, width))
        return GetDefaultWidth();

    return width;
}

void PdfFontMetrics::SubstituteGIDs(vector<unsigned>& gids, vector<unsigned char>& backwardMap) const
{
    // By default do nothing and return a map to
    backwardMap.resize(gids.size(), 1);
    // TODO: Try to implement the mechanism in some font type
}

const PdfObject* PdfFontMetrics::GetFontFileObject() const
{
    // Return nullptr by default
    return nullptr;
}

bufferview PdfFontMetrics::GetFontFileData() const
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

unsigned PdfFontMetrics::GetWeight() const
{
    int weight = GetWeightRaw();
    if (weight < 0)
    {
        if ((GetStyle() & PdfFontStyle::Bold) == PdfFontStyle::Bold)
            return 700;
        else
            return 400;
    }

    return (unsigned)weight;
}

double PdfFontMetrics::GetLeading() const
{
    double leading = GetLeadingRaw();
    if (leading < 0)
        return 0;

    return leading;
}

double PdfFontMetrics::GetXHeight() const
{
    double xHeight = GetXHeightRaw();
    if (xHeight < 0)
        return 0;

    return xHeight;
}

double PdfFontMetrics::GetStemH() const
{
    double stemH = GetStemHRaw();
    if (stemH < 0)
        return 0;

    return stemH;
}

double PdfFontMetrics::GetAvgWidth() const
{
    double avgWidth = GetAvgWidthRaw();
    if (avgWidth < 0)
        return 0;

    return avgWidth;
}

double PdfFontMetrics::GetMaxWidth() const
{
    double maxWidth = GetMaxWidthRaw();
    if (maxWidth < 0)
        return 0;

    return maxWidth;
}

double PdfFontMetrics::GetDefaultWidth() const
{
    double defaultWidth = GetDefaultWidthRaw();
    if (defaultWidth < 0)
        return 0;

    return defaultWidth;
}

PdfFontStyle PdfFontMetrics::GetStyle() const
{
    if (m_Style.has_value())
        return *m_Style;

    // ISO 32000-1:2008: Table 122 – Entries common to all font descriptors
    // The possible values shall be 100, 200, 300, 400, 500, 600, 700, 800,
    // or 900, where each number indicates a weight that is at least as dark
    // as its predecessor. A value of 400 shall indicate a normal weight;
    // 700 shall indicate bold
    bool isBold = getIsBoldHint()
        || GetWeightRaw() >= 700;
    bool isItalic = getIsItalicHint()
        || (GetFlags() & PdfFontDescriptorFlags::Italic) != PdfFontDescriptorFlags::None
        || GetItalicAngle() != 0;
    PdfFontStyle style = PdfFontStyle::Regular;
    if (isBold)
        style |= PdfFontStyle::Bold;
    if (isItalic)
        style |= PdfFontStyle::Italic;
    const_cast<PdfFontMetrics&>(*this).m_Style = style;
    return *m_Style;
}

bool PdfFontMetrics::IsStandard14FontMetrics() const
{
    PdfStandard14FontType std14Font;
    return IsStandard14FontMetrics(std14Font);
}

bool PdfFontMetrics::IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const
{
    std14Font = PdfStandard14FontType::Unknown;
    return false;
}

const Matrix2D& PdfFontMetrics::GetMatrix() const
{
    return s_DefaultMatrix;
}

bool PdfFontMetrics::IsType1Kind() const
{
    switch (GetFontFileType())
    {
        case PdfFontFileType::Type1:
        case PdfFontFileType::Type1CCF:
        case PdfFontFileType::CIDType1:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetrics::IsPdfSymbolic() const
{
    auto flags = GetFlags();
    return (flags & PdfFontDescriptorFlags::Symbolic) != PdfFontDescriptorFlags::None
        || (flags & PdfFontDescriptorFlags::NonSymbolic) == PdfFontDescriptorFlags::None;
}

bool PdfFontMetrics::IsPdfNonSymbolic() const
{
    auto flags = GetFlags();
    return (flags & PdfFontDescriptorFlags::Symbolic) == PdfFontDescriptorFlags::None
        && (flags & PdfFontDescriptorFlags::NonSymbolic) != PdfFontDescriptorFlags::None;
}

unique_ptr<PdfCMapEncoding> PdfFontMetrics::CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const
{
    (void)limitHints;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

FT_Face PdfFontMetrics::GetFace() const
{
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}
