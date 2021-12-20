/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsFreetype.h"

#include <iostream>
#include <sstream>

#include <pdfmm/private/FreetypePrivate.h>
#include FT_TRUETYPE_TABLES_H

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfFont.h"
#include "PdfCMapEncoding.h"

using namespace std;
using namespace mm;

PdfFontMetricsFreetype::PdfFontMetricsFreetype(
    const shared_ptr<charbuff>& buffer, nullable<const PdfFontMetrics&> refMetrics) :
    m_Face(nullptr),
    m_FontData(buffer)
{
    initFromBuffer(&*refMetrics);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(FT_Face face,
    const shared_ptr<charbuff>& buffer) :
    m_Face(face),
    m_FontData(buffer)
{
    initFromFace(nullptr);
}

PdfFontMetricsFreetype::~PdfFontMetricsFreetype()
{
    FT_Done_Face(m_Face);
}

void PdfFontMetricsFreetype::initFromBuffer(const PdfFontMetrics* refMetrics)
{
    FT_Open_Args openArgs;
    memset(&openArgs, 0, sizeof(openArgs));
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = reinterpret_cast<FT_Byte*>(m_FontData->data());
    openArgs.memory_size = static_cast<FT_Long>(m_FontData->size());
    FT_Error rc = FT_Open_Face(mm::GetFreeTypeLibrary(), &openArgs, 0, &m_Face);
    if (rc != 0)
    {
        PdfError::LogMessage(PdfLogSeverity::Error,
            "FreeType returned the error {} when calling FT_Open_Face for a buffered font", (int)rc);
        PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
    }

    initFromFace(refMetrics);
}

void PdfFontMetricsFreetype::initFromFace(const PdfFontMetrics* refMetrics)
{
    FT_Error rc;

    // Get the postscript name of the font and ensures it has no space:
    // 5.5.2 TrueType Fonts, "If the name contains any spaces, the spaces are removed"
    m_fontName = FT_Get_Postscript_Name(m_Face);
    m_fontName.erase(std::remove(m_fontName.begin(), m_fontName.end(), ' '), m_fontName.end());
    m_baseFontName = PdfFont::ExtractBaseName(m_fontName);

    m_HasSymbolCharset = false;

    // Try to get a unicode charmap
    rc = FT_Select_Charmap(m_Face, FT_ENCODING_UNICODE);
    if (rc != 0)
    {
        // Try to determine if it is a symbol font
        for (int c = 0; c < m_Face->num_charmaps; c++)
        {
            FT_CharMap charmap = m_Face->charmaps[c];
            if (charmap->encoding == FT_ENCODING_MS_SYMBOL)
            {
                m_HasSymbolCharset = true;
                rc = FT_Set_Charmap(m_Face, charmap);
                break;
            }
        }

        if (rc != 0)
        {
            PdfError::LogMessage(PdfLogSeverity::Error, "FreeType returned the error {} when calling FT_Select_Charmap for a buffered font", (int)rc);
            PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
        }
    }

    m_Ascent = m_Face->ascender / (double)m_Face->units_per_EM;
    m_Descent = m_Face->descender / (double)m_Face->units_per_EM;

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = m_Face->height / (double)m_Face->units_per_EM;
    m_UnderlineThickness = m_Face->underline_thickness / (double)m_Face->units_per_EM;
    m_UnderlinePosition = m_Face->underline_position / (double)m_Face->units_per_EM;
    m_Ascent = m_Face->ascender / (double)m_Face->units_per_EM;
    m_Descent = m_Face->descender / (double)m_Face->units_per_EM;

    // Set some default values, in case the font has no direct values
    if (refMetrics == nullptr)
    {
        // Get maximal width and height
        double width = (m_Face->bbox.xMax - m_Face->bbox.xMin) / (double)m_Face->units_per_EM;
        double height = (m_Face->bbox.yMax - m_Face->bbox.yMin) / (double)m_Face->units_per_EM;

        m_ItalicAngle = 0;
        m_DefaultWidth = width;
        m_Weight = -1;
        m_CapHeight = height;
        m_XHeight = 0;
        m_Flags = PdfFontDescriptorFlags::Symbolic;
        m_StrikeOutPosition = m_Ascent / 2.0;
        m_StrikeOutThickness = m_UnderlineThickness;
    }
    else
    {
        m_ItalicAngle = refMetrics->GetItalicAngle();
        m_DefaultWidth = refMetrics->GetDefaultWidth();
        m_Weight = refMetrics->GetWeightRaw();
        m_CapHeight = refMetrics->GetCapHeight();
        m_XHeight = refMetrics->GetXHeightRaw();
        m_Flags = refMetrics->GetFlags();
        m_StrikeOutPosition = refMetrics->GetStrikeOutPosition();
        m_StrikeOutThickness = refMetrics->GetStrikeOutThickness();
    }

    TT_OS2* os2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_Face, FT_SFNT_OS2));
    if (os2Table != nullptr)
    {
        m_StrikeOutPosition = os2Table->yStrikeoutPosition / (double)m_Face->units_per_EM;
        m_StrikeOutThickness = os2Table->yStrikeoutSize / (double)m_Face->units_per_EM;
        m_CapHeight = os2Table->sCapHeight / (double)m_Face->units_per_EM;
        m_XHeight = os2Table->sxHeight / (double)m_Face->units_per_EM;
        m_Weight = os2Table->usWeightClass;
    }

    TT_Postscript* psTable = static_cast<TT_Postscript*>(FT_Get_Sfnt_Table(m_Face, FT_SFNT_POST));
    if (psTable != nullptr)
    {
        m_ItalicAngle = (double)psTable->italicAngle;
        if (psTable->isFixedPitch != 0)
            m_Flags |= PdfFontDescriptorFlags::FixedPitch;
    }


    // NOTE2: It is not correct to write flags ForceBold if the
    // font is already bold. The ForceBold flag is just an hint
    // for the viewer to draw glyphs with more pixels
    // TODO: Infer more characateristics
    if (IsItalic())
        m_Flags |= PdfFontDescriptorFlags::Italic;
}

string PdfFontMetricsFreetype::GetBaseFontName() const
{
    return m_baseFontName;
}

string PdfFontMetricsFreetype::GetFontName() const
{
    return m_fontName;
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::FromBuffer(const bufferview& buffer)
{
    return std::make_unique<PdfFontMetricsFreetype>(std::make_shared<charbuff>(buffer));
}

unique_ptr<PdfFontMetricsFreetype> PdfFontMetricsFreetype::FromFace(FT_Face face)
{
    if (face == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Face can't be null");

    // TODO: Obtain the fontdata from face
    return std::unique_ptr<PdfFontMetricsFreetype>(new PdfFontMetricsFreetype(face, nullptr));
}

unsigned PdfFontMetricsFreetype::GetGlyphCount() const
{
    return (unsigned)m_Face->num_glyphs;
}

bool PdfFontMetricsFreetype::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (FT_Load_Glyph(m_Face, gid, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) != 0)
    {
        width = -1;
        return false;
    }

    // zero return code is success!
    width = m_Face->glyph->metrics.horiAdvance / (double)m_Face->units_per_EM;
    return true;
}

bool PdfFontMetricsFreetype::TryGetGID(char32_t codePoint, unsigned& gid) const
{
    if (m_HasSymbolCharset)
        codePoint = codePoint | 0xF000;

    gid = FT_Get_Char_Index(m_Face, codePoint);
    return gid != 0;
}


unique_ptr<PdfCMapEncoding> PdfFontMetricsFreetype::CreateToUnicodeMap(const PdfEncodingLimits& limitHints) const
{
    PdfCharCodeMap map;
    FT_ULong charcode;
    FT_UInt gid;

    charcode = FT_Get_First_Char(m_Face, &gid);
    while (gid != 0)
    {
        map.PushMapping({ gid, limitHints.MinCodeSize }, (char32_t)charcode);
        charcode = FT_Get_Next_Char(m_Face, charcode, &gid);
    }

    return std::make_unique<PdfCMapEncoding>(std::move(map));
}

PdfFontDescriptorFlags PdfFontMetricsFreetype::GetFlags() const
{
    return m_Flags;
}

double PdfFontMetricsFreetype::GetDefaultWidth() const
{
    return m_DefaultWidth;
}

void PdfFontMetricsFreetype::GetBoundingBox(vector<double>& bbox) const
{
    bbox.clear();
    bbox.push_back(m_Face->bbox.xMin / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.yMin / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.xMax / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.yMax / (double)m_Face->units_per_EM);
}

bool PdfFontMetricsFreetype::getIsBoldHint() const
{
    return (m_Face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
}

bool PdfFontMetricsFreetype::getIsItalicHint() const
{
    return (m_Face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
}

double PdfFontMetricsFreetype::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsFreetype::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsFreetype::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsFreetype::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsFreetype::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
}

double PdfFontMetricsFreetype::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsFreetype::GetDescent() const
{
    return m_Descent;
}

bufferview PdfFontMetricsFreetype::GetFontFileData() const
{
    return bufferview(m_FontData->data(), m_FontData->size());
}

int PdfFontMetricsFreetype::GetWeightRaw() const
{
    return m_Weight;
}

double PdfFontMetricsFreetype::GetCapHeight() const
{
    return m_CapHeight;
}

double PdfFontMetricsFreetype::GetXHeightRaw() const
{
    return m_XHeight;
}

double PdfFontMetricsFreetype::GetStemV() const
{
    // ISO 32000-2:2017, Table 120 — Entries common to all font descriptors
    // says: "A value of 0 indicates an unknown stem thickness". No mention
    // is done about this in ISO 32000-1:2008, but we assume 0 is a safe
    // value for all implementations
    return 0;
}

double PdfFontMetricsFreetype::GetStemHRaw() const
{
    return -1;
}

double PdfFontMetricsFreetype::GetItalicAngle() const
{
    return m_ItalicAngle;
}

PdfFontFileType PdfFontMetricsFreetype::GetFontFileType() const
{
    // TODO
    return PdfFontFileType::TrueType;
}
