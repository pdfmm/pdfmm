/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontMetricsStandard14.h"

#include <pdfmm/private/PdfStandard14FontData.h>

#include "PdfArray.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfFontMetricsStandard14::PdfFontMetricsStandard14(
        PdfStandard14FontType fontType, const Standard14FontData& data,
        std::unique_ptr<std::vector<double>> parsedWidths) :
    m_Std14FontType(fontType),
    m_data(data),
    m_parsedWidths(std::move(parsedWidths))
{
    m_LineSpacing = 0.0;
    m_UnderlineThickness = 0.05;
    m_StrikeOutThickness = m_UnderlineThickness;
    m_Ascent = data.Ascent / 1000.0;
    m_Descent = data.Descent / 1000.0;

    m_UnderlinePosition = data.UnderlinePos / 1000.0;
    m_StrikeOutPosition = data.StrikeoutPos / 1000.0;

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = (data.Ascent + abs(data.Descent)) / 1000.0;
}

unique_ptr<PdfFontMetricsStandard14> PdfFontMetricsStandard14::Create(
    PdfStandard14FontType fontType)
{
    return create(fontType, nullptr);
}

unique_ptr<PdfFontMetricsStandard14> PdfFontMetricsStandard14::Create(
    PdfStandard14FontType fontType, const PdfObject& fontObj)
{
    return create(fontType, &fontObj);
}

unique_ptr<PdfFontMetricsStandard14> PdfFontMetricsStandard14::create(
    PdfStandard14FontType fontType, const PdfObject* fontObj)
{
    // CHECK-ME: Some standard14 fonts indeed have a /Widths
    // entry, but is it actually honored by Adobe products?
    unique_ptr<vector<double>> parsedWidths;
    if (fontObj != nullptr)
    {
        auto widthsObj = fontObj->GetDictionary().FindKey("Widths");
        if (widthsObj != nullptr)
        {
            auto& arrWidths = widthsObj->GetArray();
            parsedWidths.reset(new vector<double>(arrWidths.size()));
            for (auto& obj : arrWidths)
                parsedWidths->push_back(obj.GetReal());
        }
    }

    return unique_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
        fontType, GetInstance(fontType)->GetRawData(), std::move(parsedWidths)));
}

unsigned PdfFontMetricsStandard14::GetGlyphCount() const
{
    if (m_parsedWidths == nullptr)
        return m_data.WidthsSize;
    else
        return (unsigned)m_parsedWidths->size();
}

bool PdfFontMetricsStandard14::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (m_parsedWidths == nullptr)
    {
        if (gid >= m_data.WidthsSize)
        {
            width = -1;
            return false;
        }

        width = m_data.Widths[gid].Width / 1000.0; // Convert to PDF units
        return true;
    }
    else
    {
        if (gid >= m_parsedWidths->size())
        {
            width = -1;
            return false;
        }

        width = (*m_parsedWidths)[gid];
        return true;
    }
}

bool PdfFontMetricsStandard14::TryGetGID(char32_t codePoint, unsigned& gid) const
{
    if (codePoint >= 0xFFFF)
    {
        gid = { };
        return false;
    }

    auto& map = GetStd14CPToGIDMap(m_Std14FontType);
    auto found = map.find((unsigned short)codePoint);
    if (found == map.end())
    {
        gid = { };
        return false;
    }

    gid = found->second;
    return true;
}

PdfFontDescriptorFlags PdfFontMetricsStandard14::GetFlags() const
{
    return m_data.Flags;
}

double PdfFontMetricsStandard14::GetDefaultWidth() const
{
    return m_data.DefaultWidth / 1000.0;
}

double PdfFontMetricsStandard14::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsStandard14::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsStandard14::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsStandard14::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsStandard14::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
}

double PdfFontMetricsStandard14::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsStandard14::GetDescent() const
{
    return m_Descent;
}

string PdfFontMetricsStandard14::GetFontName() const
{
    return (string)GetStandard14FontName(m_Std14FontType);
}

string PdfFontMetricsStandard14::GetBaseFontName() const
{
    return (string)GetStandard14FontBaseName(m_Std14FontType);
}

int PdfFontMetricsStandard14::GetWeightRaw() const
{
    return m_data.Weight;
}

double PdfFontMetricsStandard14::GetCapHeight() const
{
    return m_data.CapHeight / 1000.0;
}

double PdfFontMetricsStandard14::GetXHeightRaw() const
{
    return m_data.XHeight / 1000.0;
}

double PdfFontMetricsStandard14::GetStemV() const
{
    return m_data.StemV / 1000.0;
}

double PdfFontMetricsStandard14::GetStemHRaw() const
{
    return m_data.StemH / 1000.0;
}

double PdfFontMetricsStandard14::GetItalicAngle() const
{
    return m_data.ItalicAngle;
}

PdfFontFileType PdfFontMetricsStandard14::GetFontFileType() const
{
    return PdfFontFileType::Type1CCF;
}

bool PdfFontMetricsStandard14::IsStandard14FontMetrics(PdfStandard14FontType& std14Font) const
{
    std14Font = m_Std14FontType;
    return true;
}

void PdfFontMetricsStandard14::GetBoundingBox(std::vector<double>& bbox) const
{
    // Convert to PDF units
    bbox.clear();
    bbox.push_back(m_data.BBox.GetLeft() / 1000.0);
    bbox.push_back(m_data.BBox.GetBottom() / 1000.0);
    bbox.push_back(m_data.BBox.GetWidth() / 1000.0);
    bbox.push_back(m_data.BBox.GetHeight() / 1000.0);
}

cspan<char> PdfFontMetricsStandard14::GetFontFileData() const
{
    return mm::GetStandard14FontFileData(m_Std14FontType);
}

bool PdfFontMetricsStandard14::getIsBoldHint() const
{
    switch (m_Std14FontType)
    {
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetricsStandard14::getIsItalicHint() const
{
    switch (m_Std14FontType)
    {
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBoldOblique:
            return true;
        default:
            return false;
    }
}

