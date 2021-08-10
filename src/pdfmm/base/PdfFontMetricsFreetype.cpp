/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontMetricsFreetype.h"

#include <iostream>
#include <sstream>

#include <wchar.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/tttables.h>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfVariant.h"
#include "PdfFontFactory.h"

#define PDFMM_FIRST_READABLE 31
#define PDFMM_WIDTH_CACHE_SIZE 256

using namespace std;
using namespace mm;

struct Scoped_FT_Face
{
    Scoped_FT_Face() : FTFace(nullptr) { }
    ~Scoped_FT_Face()
    {
        if (FTFace != nullptr)
            FT_Done_Face(FTFace);
    }
    FT_Face FTFace;
};

PdfFontMetricsFreetype* PdfFontMetricsFreetype::CreateForSubsetting(FT_Library* library, const string_view& filename,
    bool isSymbol)
{
    Scoped_FT_Face scoped_face;

    FT_Error err = FT_New_Face(*library, filename.data(), 0, &scoped_face.FTFace);
    if (err != 0)
    {
        // throw an exception
        PdfError::LogMessage(LogSeverity::Critical, "FreeType returned the error %i when calling FT_New_Face for font %s.",
            err, filename.data());
        PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
    }

    FT_ULong length = 0;
    err = FT_Load_Sfnt_Table(scoped_face.FTFace, 0, 0, nullptr, &length);
    if (err == 0)
    {
        PdfSharedBuffer buffer(length);
        err = FT_Load_Sfnt_Table(scoped_face.FTFace, 0, 0, reinterpret_cast<FT_Byte*>(buffer.GetBuffer()), &length);
        if (err == 0)
            return new PdfFontMetricsFreetype(library, buffer, isSymbol);
    }

    // throw an exception
    PdfError::LogMessage(LogSeverity::Critical, "FreeType returned the error %i when calling FT_Load_Sfnt_Table for font %s.",
        err, filename.data());
    PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(FT_Library* library, const string_view& filename,
        bool isSymbol) :
    PdfFontMetrics(PdfFontMetrics::GetFontMetricsTypeFromFilename(filename), filename),
    m_Library(library),
    m_Face(nullptr),
    m_IsSymbol(isSymbol)
{
    FT_Error err = FT_New_Face(*library, filename.data(), 0, &m_Face);
    if (err != 0)
    {
        PdfError::LogMessage(LogSeverity::Critical, "FreeType returned the error %i when calling FT_New_Face for font %s.",
            err, filename.data());
        PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
    }

    InitFromFace(isSymbol);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(FT_Library* library, const char* buffer, size_t size,
        bool isSymbol) :
    PdfFontMetrics(PdfFontMetricsType::Unknown, { }),
    m_Library(library),
    m_Face(nullptr),
    m_IsSymbol(isSymbol)
{
    m_FontData = PdfSharedBuffer(size);
    memcpy(m_FontData.GetBuffer(), buffer, size);
    InitFromBuffer(isSymbol);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(FT_Library* library, const PdfSharedBuffer& buffer,
        bool isSymbol) :
    PdfFontMetrics(PdfFontMetricsType::Unknown, { }),
    m_Library(library),
    m_Face(nullptr),
    m_IsSymbol(isSymbol),
    m_FontData(buffer)
{
    InitFromBuffer(isSymbol);
}

PdfFontMetricsFreetype::PdfFontMetricsFreetype(FT_Library* library, FT_Face face,
    bool isSymbol) :
    PdfFontMetrics(PdfFontMetricsType::TrueType,
        // Try to initialize the pathname from m_face
        // so that font embedding will work
        (face->stream ? reinterpret_cast<char*>(face->stream->pathname.pointer) : "")),
    m_Library(library),
    m_Face(face),
    m_IsSymbol(isSymbol)
{
    if (face == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Face can't be null");

    InitFromFace(isSymbol);
}

PdfFontMetricsFreetype::~PdfFontMetricsFreetype()
{
    FT_Done_Face(m_Face);
}

void PdfFontMetricsFreetype::InitFromBuffer(bool isSymbol)
{
    FT_Open_Args openArgs;
    memset(&openArgs, 0, sizeof(openArgs));
    openArgs.flags = FT_OPEN_MEMORY;
    openArgs.memory_base = reinterpret_cast<FT_Byte*>(m_FontData.GetBuffer());
    openArgs.memory_size = static_cast<FT_Long>(m_FontData.GetSize());
    FT_Error err = FT_Open_Face(*m_Library, &openArgs, 0, &m_Face);
    if (err != 0)
    {
        PdfError::LogMessage(LogSeverity::Critical, "FreeType returned the error %i when calling FT_Open_Face for a buffered font.", err);
        PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
    }

    // Assume true type
    this->SetFontType(PdfFontMetricsType::TrueType);
    InitFromFace(isSymbol);
}

void PdfFontMetricsFreetype::InitFromFace(bool isSymbol)
{
    if (m_FontType == PdfFontMetricsType::Unknown)
    {
        // We need to have identified the font type by this point
        // Unsupported font.
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, m_Filename.c_str());
    }

    FT_Error rc;

    m_Weight = 500;
    m_ItalicAngle = 0;
    m_LineSpacing = 0.0;
    m_UnderlineThickness = 0.0;
    m_UnderlinePosition = 0.0;
    m_StrikeOutPosition = 0.0;
    m_StrikeOutThickness = 0.0;
    m_IsSymbol = isSymbol;

    m_Ascent = m_Face->ascender / m_Face->units_per_EM;
    m_Descent = m_Face->descender / m_Face->units_per_EM;
    m_IsBold = (m_Face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
    m_IsItalic = (m_Face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

    // Try to get a unicode charmap
    rc = FT_Select_Charmap(m_Face, isSymbol ? FT_ENCODING_MS_SYMBOL : FT_ENCODING_UNICODE);
    if (rc != 0)
    {
        PdfError::LogMessage(LogSeverity::Critical, "FreeType returned the error %i when calling FT_Select_Charmap for a buffered font.", rc);
        PDFMM_RAISE_ERROR(PdfErrorCode::FreeType);
    }

    // Try to determine if it is a symbol font
    for (int c = 0; c < m_Face->num_charmaps; c++)
    {
        FT_CharMap charmap = m_Face->charmaps[c];

        if (charmap->encoding == FT_ENCODING_MS_SYMBOL)
        {
            m_IsSymbol = true;
            FT_Set_Charmap(m_Face, charmap);
            break;
        }
        // TODO: Also check for FT_ENCODING_ADOBE_CUSTOM and set it?
    }

    // we cache the 256 first width entries as they
    // are most likely needed quite often
    m_Widths.clear();
    m_Widths.reserve(PDFMM_WIDTH_CACHE_SIZE);
    for (unsigned i = 0; i < PDFMM_WIDTH_CACHE_SIZE; i++)
    {
        if (i < PDFMM_FIRST_READABLE)
        {
            m_Widths.push_back(0.0);
        }
        else
        {
            int index = i;
            // Handle symbol fonts
            if (m_IsSymbol)
            {
                index = index | 0xF000;
            }

            if (FT_Load_Char(m_Face, index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP) == 0)  // | FT_LOAD_NO_RENDER
            {
                m_Widths.push_back(static_cast<double>(m_Face->glyph->metrics.horiAdvance) / m_Face->units_per_EM);
                continue;
            }

            m_Widths.push_back(0.0);
        }
    }

    InitFontSizes();
}

void PdfFontMetricsFreetype::InitFontSizes()
{
    float size = 1.0f;
    // TODO: Maybe we have to set this for charwidth!!!
    FT_Set_Char_Size(m_Face, static_cast<int>(size * 64.0), 0, 72, 72);

    // calculate the line spacing now, as it changes only with the font size
    m_LineSpacing = (static_cast<double>(m_Face->height) / m_Face->units_per_EM);
    m_UnderlineThickness = (static_cast<double>(m_Face->underline_thickness) / m_Face->units_per_EM);
    m_UnderlinePosition = (static_cast<double>(m_Face->underline_position) / m_Face->units_per_EM);
    m_Ascent = static_cast<double>(m_Face->ascender) / m_Face->units_per_EM;
    m_Descent = static_cast<double>(m_Face->descender) / m_Face->units_per_EM;
    // Set default values for strikeout, in case the font has no direct values
    m_StrikeOutPosition = m_Ascent / 2.0;
    m_StrikeOutThickness = m_UnderlineThickness;

    TT_OS2* os2Table = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(m_Face, ft_sfnt_os2));
    if (os2Table != nullptr)
    {
        m_StrikeOutPosition = static_cast<double>(os2Table->yStrikeoutPosition) / m_Face->units_per_EM;
        m_StrikeOutThickness = static_cast<double>(os2Table->yStrikeoutSize) / m_Face->units_per_EM;
    }
}

string PdfFontMetricsFreetype::GetBaseFontName() const
{
    const char* name = FT_Get_Postscript_Name(m_Face);
    return name == nullptr ? string() : string(name);
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
    if (m_IsSymbol)
        codePoint = codePoint | 0xF000;

    gid = FT_Get_Char_Index(m_Face, codePoint);
    return gid != 0;
}

double PdfFontMetricsFreetype::GetDefaultCharWidth() const
{
    // TODO
    return 0.0;
}

void PdfFontMetricsFreetype::GetBoundingBox(vector<double>& bbox) const
{
    bbox.clear();
    bbox.push_back(m_Face->bbox.xMin / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.yMin / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.xMax / (double)m_Face->units_per_EM);
    bbox.push_back(m_Face->bbox.yMax / (double)m_Face->units_per_EM);
}

bool PdfFontMetricsFreetype::IsBold() const
{
    return m_IsBold;
}

bool PdfFontMetricsFreetype::IsItalic() const
{
    return m_IsItalic;
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

string_view PdfFontMetricsFreetype::GetFontData() const
{
    return string_view(m_FontData.GetBuffer(), m_FontData.GetSize());
}

unsigned PdfFontMetricsFreetype::GetWeight() const
{
    return m_Weight;
}

double PdfFontMetricsFreetype::GetItalicAngle() const
{
    return m_ItalicAngle;
}

bool PdfFontMetricsFreetype::IsSymbol() const
{
    return m_IsSymbol;
}
