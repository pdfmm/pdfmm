/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontMetricsStandard14.h"

#include <pdfmm/private/PdfStandard14FontsData.h>

#include "PdfArray.h"

using namespace std;
using namespace mm;

PdfFontMetricsStandard14::PdfFontMetricsStandard14(PdfStandard14FontType fontType,
        const Standard14FontData* data, unsigned dataSize, bool isSymbol,
        int16_t ascent, int16_t descent, uint16_t x_height,
        uint16_t cap_height, int16_t strikeout_pos,
        int16_t underline_pos, const PdfRect& mbbox) :
    PdfFontMetrics(PdfFontMetricsType::Type1Standard14),
    m_Std14FontType(fontType),
    m_data(data),
    m_dataSize(dataSize),
    m_x_height(x_height),
    m_cap_height(cap_height),
    m_BBox(mbbox),
    m_IsSymbol(isSymbol)
{
    m_Weight = 500;
    m_ItalicAngle = 0;
    m_LineSpacing = 0.0;
    m_UnderlineThickness = 0.05;
    m_StrikeOutThickness = m_UnderlineThickness;
    m_Ascent = ascent / 1000.0;
    m_Descent = descent / 1000.0;

    m_UnderlinePosition = underline_pos / 1000.0;
    m_StrikeOutPosition = strikeout_pos / 1000.0;

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = (ascent + abs(descent)) / 1000.0;
}

unsigned PdfFontMetricsStandard14::GetGlyphCount() const
{
    return m_dataSize;
}

bool PdfFontMetricsStandard14::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (gid >= m_dataSize)
    {
        width = -1;
        return false;
    }

    width = m_data[gid].Width / 1000.0; // Convert to PDF units
    return true;
}

bool PdfFontMetricsStandard14::TryGetGID(char32_t codePoint, unsigned& gid) const
{
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

double PdfFontMetricsStandard14::GetDefaultCharWidth() const
{
    // Just assume there is no default width
    return 0;
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

string PdfFontMetricsStandard14::GetBaseFontName() const
{
    return (string)GetStandard14FontName(m_Std14FontType);
}

unsigned PdfFontMetricsStandard14::GetWeight() const
{
    return m_Weight;
}

double PdfFontMetricsStandard14::GetItalicAngle() const
{
    return m_ItalicAngle;
}

bool PdfFontMetricsStandard14::IsSymbol() const
{
    return m_IsSymbol;
}

void PdfFontMetricsStandard14::GetBoundingBox(std::vector<double>& bbox) const
{
    // Convert to PDF units
    bbox.clear();
    bbox.push_back(m_BBox.GetLeft() / 1000.0);
    bbox.push_back(m_BBox.GetBottom() / 1000.0);
    bbox.push_back(m_BBox.GetWidth() / 1000.0);
    bbox.push_back(m_BBox.GetHeight() / 1000.0);
}

string_view PdfFontMetricsStandard14::GetFontData() const
{
    // TODO: We should return standard14 font data taken from PDFium
    // https://github.com/mozilla/pdf.js/tree/master/external/standard_fonts
    // https://opensource.google/projects/pdfium
    return { };
}

bool PdfFontMetricsStandard14::IsBold() const
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

bool PdfFontMetricsStandard14::IsItalic() const
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

bool PdfFontMetricsStandard14::FontNameHasBoldItalicInfo() const
{
    // All font names states if they are bold or italic ("oblique"
    //  corresponds to italic), except in symbolic fonts
    return !m_IsSymbol;
}

shared_ptr<const PdfFontMetricsStandard14> PdfFontMetricsStandard14::GetInstance(PdfStandard14FontType baseFont)
{
    // The following are the Standard 14 fonts data copied from libharu.
    static vector<shared_ptr<PdfFontMetricsStandard14>> PDFMM_BUILTIN_FONTS = {
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesRoman,
            CHAR_DATA_TIMES_ROMAN,
            (unsigned)std::size(CHAR_DATA_TIMES_ROMAN),
            false,
            727,
            -273,
            450,
            662,
            262,
            -100,
            PdfRect(-168, -218, 1000, 898)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesItalic,
            CHAR_DATA_TIMES_ITALIC,
            (unsigned)std::size(CHAR_DATA_TIMES_ITALIC),
            false,
            727,
            -273,
            441,
            653,
            262,
            -100,
            PdfRect(-169, -217, 1010, 883)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesBold,
            CHAR_DATA_TIMES_BOLD,
            (unsigned)std::size(CHAR_DATA_TIMES_BOLD),
            false,
            727,
            -273,
            461,
            676,
            262,
            -100,
            PdfRect(-168, -218, 1000, 935)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::TimesBoldItalic,
            CHAR_DATA_TIMES_BOLD_ITALIC,
            (unsigned)std::size(CHAR_DATA_TIMES_BOLD_ITALIC),
            false,
            727,
            -273,
            462,
            669,
            262,
            -100,
            PdfRect(-200, -218, 996, 921)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::Helvetica,
            CHAR_DATA_HELVETICA,
            (unsigned)std::size(CHAR_DATA_HELVETICA),
            false,
            750,
            -250,
            523,
            718,
            290,
            -100,
            PdfRect(-166, -225, 1000, 931)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::HelveticaOblique,
            CHAR_DATA_HELVETICA_OBLIQUE,
            (unsigned)std::size(CHAR_DATA_HELVETICA_OBLIQUE),
            false,
            750,
            -250,
            532,
            718,
            290,
            -100,
            PdfRect(-170, -225, 1116, 931)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::HelveticaBold,
            CHAR_DATA_HELVETICA_BOLD,
            (unsigned)std::size(CHAR_DATA_HELVETICA_BOLD),
            false,
            750,
            -250,
            532,
            718,
            290,
            -100,
            PdfRect(-170, -228, 1003, 962)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::HelveticaBoldOblique,
            CHAR_DATA_HELVETICA_BOLD_OBLIQUE,
            (unsigned)std::size(CHAR_DATA_HELVETICA_BOLD_OBLIQUE),
            false,
            750,
            -250,
            532,
            718,
            290,
            -100,
            PdfRect(-174, -228, 1114, 962)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::Courier,
            CHAR_DATA_COURIER,
            315,
            false,
            627,
            -373,
            426,
            562,
            261,
            -224,
            PdfRect(-23, -250, 715, 805)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::CourierOblique,
            CHAR_DATA_COURIER_OBLIQUE,
            315,
            false,
            627,
            -373,
            426,
            562,
            261,
            -224,
            PdfRect(-27, -250, 849, 805)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::CourierBold,
            CHAR_DATA_COURIER_BOLD,
            315,
            false,
            627,
            -373,
            439,
            562,
            261,
            -224,
            PdfRect(-113, -250, 749, 801)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::CourierBoldOblique,
            CHAR_DATA_COURIER_BOLD_OBLIQUE,
            315,
            false,
            627,
            -373,
            439,
            562,
            261,
            -224,
            PdfRect(-57, -250, 869, 801)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::Symbol,
            CHAR_DATA_SYMBOL,
            (unsigned)std::size(CHAR_DATA_SYMBOL),
            true,
            683,
            -217,
            462,
            669,
            341,
            -100,
           PdfRect(-180, -293, 1090, 1010)
        )),
        shared_ptr<PdfFontMetricsStandard14>(new PdfFontMetricsStandard14(
            PdfStandard14FontType::ZapfDingbats,
            CHAR_DATA_ZAPF_DINGBATS,
            (unsigned)std::size(CHAR_DATA_ZAPF_DINGBATS),
            true,
            683,
            -217,
            462,
            669,
            341,
            -100,
            PdfRect(-1, -143, 981, 820)
        ))
    };

    switch (baseFont)
    {
        case PdfStandard14FontType::TimesRoman:
            return PDFMM_BUILTIN_FONTS[0];
        case PdfStandard14FontType::TimesItalic:
            return PDFMM_BUILTIN_FONTS[1];
        case PdfStandard14FontType::TimesBold:
            return PDFMM_BUILTIN_FONTS[2];
        case PdfStandard14FontType::TimesBoldItalic:
            return PDFMM_BUILTIN_FONTS[3];
        case PdfStandard14FontType::Helvetica:
            return PDFMM_BUILTIN_FONTS[4];
        case PdfStandard14FontType::HelveticaOblique:
            return PDFMM_BUILTIN_FONTS[5];
        case PdfStandard14FontType::HelveticaBold:
            return PDFMM_BUILTIN_FONTS[6];
        case PdfStandard14FontType::HelveticaBoldOblique:
            return PDFMM_BUILTIN_FONTS[7];
        case PdfStandard14FontType::Courier:
            return PDFMM_BUILTIN_FONTS[8];
        case PdfStandard14FontType::CourierOblique:
            return PDFMM_BUILTIN_FONTS[9];
        case PdfStandard14FontType::CourierBold:
            return PDFMM_BUILTIN_FONTS[10];
        case PdfStandard14FontType::CourierBoldOblique:
            return PDFMM_BUILTIN_FONTS[11];
        case PdfStandard14FontType::Symbol:
            return PDFMM_BUILTIN_FONTS[12];
        case PdfStandard14FontType::ZapfDingbats:
            return PDFMM_BUILTIN_FONTS[13];
        case PdfStandard14FontType::Unknown:
        default:
            return nullptr;
    }
}
