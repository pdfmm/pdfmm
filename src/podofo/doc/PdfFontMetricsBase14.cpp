/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfFontMetricsBase14.h"

#include "base/PdfArray.h"

#include "PdfFontFactoryBase14Data.h"

using namespace std;
using namespace PoDoFo;

PdfFontMetricsBase14::PdfFontMetricsBase14(PdfStd14FontType fontType,
        const Base14FontData* data, unsigned dataSize, bool isSymbol,
        int16_t ascent, int16_t descent, uint16_t x_height,
        uint16_t cap_height, int16_t strikeout_pos,
        int16_t underline_pos, const PdfRect& mbbox) :
    PdfFontMetrics(PdfFontMetricsType::Type1Base14, { }),
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

unsigned PdfFontMetricsBase14::GetGlyphCount() const
{
    return m_dataSize;
}

bool PdfFontMetricsBase14::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (gid >= m_dataSize)
    {
        width = -1;
        return false;
    }

    width = m_data[gid].Width / 1000.0; // Convert to PDF units
    return true;
}

bool PdfFontMetricsBase14::TryGetGID(char32_t codePoint, unsigned& gid) const
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

double PdfFontMetricsBase14::GetDefaultCharWidth() const
{
    // Just assume there is no default width
    return 0;
}

double PdfFontMetricsBase14::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsBase14::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsBase14::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsBase14::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsBase14::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
}

double PdfFontMetricsBase14::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsBase14::GetDescent() const
{
    return m_Descent;
}

string PdfFontMetricsBase14::GetBaseFontName() const
{
    return (string)GetStandard14FontName(m_Std14FontType);
}

unsigned PdfFontMetricsBase14::GetWeight() const
{
    return m_Weight;
}

double PdfFontMetricsBase14::GetItalicAngle() const
{
    return m_ItalicAngle;
}

bool PdfFontMetricsBase14::IsSymbol() const
{
    return m_IsSymbol;
}

void PdfFontMetricsBase14::GetBoundingBox(std::vector<double>& bbox) const
{
    // Convert to PDF units
    bbox.clear();
    bbox.push_back(m_BBox.GetLeft() / 1000.0);
    bbox.push_back(m_BBox.GetBottom() / 1000.0);
    bbox.push_back(m_BBox.GetWidth() / 1000.0);
    bbox.push_back(m_BBox.GetHeight() / 1000.0);
}

string_view PdfFontMetricsBase14::GetFontData() const
{
    return { };
}

bool PdfFontMetricsBase14::IsBold() const
{
    switch (m_Std14FontType)
    {
        case PdfStd14FontType::TimesBold:
        case PdfStd14FontType::TimesBoldItalic:
        case PdfStd14FontType::HelveticaBold:
        case PdfStd14FontType::HelveticaBoldOblique:
        case PdfStd14FontType::CourierBold:
        case PdfStd14FontType::CourierBoldOblique:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetricsBase14::IsItalic() const
{
    switch (m_Std14FontType)
    {
        case PdfStd14FontType::TimesItalic:
        case PdfStd14FontType::TimesBoldItalic:
        case PdfStd14FontType::HelveticaOblique:
        case PdfStd14FontType::HelveticaBoldOblique:
        case PdfStd14FontType::CourierOblique:
        case PdfStd14FontType::CourierBoldOblique:
            return true;
        default:
            return false;
    }
}

bool PdfFontMetricsBase14::FontNameHasBoldItalicInfo() const
{
    // All font names states if they are bold or italic ("oblique"
    //  corresponds to italic), except in symbolic fonts
    return !m_IsSymbol;
}

shared_ptr<const PdfFontMetricsBase14> PdfFontMetricsBase14::GetInstance(PdfStd14FontType baseFont)
{
    // The following are the Base 14 fonts data copied from libharu.
    static vector<shared_ptr<PdfFontMetricsBase14>> PODOFO_BUILTIN_FONTS = {
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::TimesRoman,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::TimesItalic,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::TimesBold,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::TimesBoldItalic,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::Helvetica,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::HelveticaOblique,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::HelveticaBold,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::HelveticaBoldOblique,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::Courier,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::CourierOblique,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::CourierBold,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::CourierBoldOblique,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::Symbol,
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
        shared_ptr<PdfFontMetricsBase14>(new PdfFontMetricsBase14(
            PdfStd14FontType::ZapfDingbats,
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
        case PdfStd14FontType::TimesRoman:
            return PODOFO_BUILTIN_FONTS[0];
        case PdfStd14FontType::TimesItalic:
            return PODOFO_BUILTIN_FONTS[1];
        case PdfStd14FontType::TimesBold:
            return PODOFO_BUILTIN_FONTS[2];
        case PdfStd14FontType::TimesBoldItalic:
            return PODOFO_BUILTIN_FONTS[3];
        case PdfStd14FontType::Helvetica:
            return PODOFO_BUILTIN_FONTS[4];
        case PdfStd14FontType::HelveticaOblique:
            return PODOFO_BUILTIN_FONTS[5];
        case PdfStd14FontType::HelveticaBold:
            return PODOFO_BUILTIN_FONTS[6];
        case PdfStd14FontType::HelveticaBoldOblique:
            return PODOFO_BUILTIN_FONTS[7];
        case PdfStd14FontType::Courier:
            return PODOFO_BUILTIN_FONTS[8];
        case PdfStd14FontType::CourierOblique:
            return PODOFO_BUILTIN_FONTS[9];
        case PdfStd14FontType::CourierBold:
            return PODOFO_BUILTIN_FONTS[10];
        case PdfStd14FontType::CourierBoldOblique:
            return PODOFO_BUILTIN_FONTS[11];
        case PdfStd14FontType::Symbol:
            return PODOFO_BUILTIN_FONTS[12];
        case PdfStd14FontType::ZapfDingbats:
            return PODOFO_BUILTIN_FONTS[13];
        case PdfStd14FontType::Unknown:
        default:
            return nullptr;
    }
}
