/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontManager.h"

#include <algorithm>

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)
#include <pdfmm/common/WindowsLeanMean.h>
#endif // defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)

#include <pdfmm/private/FreetypePrivate.h>
#include FT_TRUETYPE_TABLES_H
#include <utfcpp/utf8.h>

#include "PdfDictionary.h"
#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"
#include "PdfFont.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontStandard14.h"
#include "PdfFontType1.h"

using namespace std;
using namespace mm;

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)

static std::unique_ptr<chars> getFontData(const LOGFONTW& inFont);
static bool getFontData(chars& buffer, HDC hdc, HFONT hf);
static void getFontDataTTC(chars& buffer, const chars& fileBuffer, const chars& ttcBuffer);

#endif // defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)

static unique_ptr<chars> getFontData(const string_view& filename, unsigned short faceIndex);

#if defined(PDFMM_HAVE_FONTCONFIG)
shared_ptr<PdfFontConfigWrapper> PdfFontManager::m_fontConfig;
#endif

PdfFontManager::PdfFontManager(PdfDocument& doc)
    : m_doc(&doc) { }

PdfFontManager::~PdfFontManager()
{
    this->EmptyCache();
}

void PdfFontManager::EmptyCache()
{
    m_fontMap.clear();
    m_loadedFontMap.clear();
}

PdfFont* PdfFontManager::GetLoadedFont(PdfObject& obj)
{
    // TODO: We should check that the font being loaded
    // is not present in the imported font cache
    if (!obj.IsIndirect())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Object is not indirect");

    auto found = m_loadedFontMap.find(obj.GetIndirectReference());
    if (found != m_loadedFontMap.end())
        return found->second.get();

    // Create a new font
    unique_ptr<PdfFont> font;
    if (!PdfFont::TryCreateFromObject(obj, font))
        return nullptr;

    auto inserted = m_loadedFontMap.insert({ obj.GetIndirectReference(), std::move(font) });
    return inserted.first->second.get();
}

PdfFont* PdfFontManager::GetFont(const string_view& fontName, const PdfFontCreationParams& params)
{
    if (params.Encoding.IsNull())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid encoding");

    // NOTE: We don't support standard 14 fonts on subset
    PdfStandard14FontType stdFont;
    if (params.FontInitOpts != PdfFontInitOptions::Subset &&
        params.SearchParams.AutoSelectOpts != PdfAutoSelectFontOptions::None
        && PdfFontStandard14::IsStandard14Font(fontName,
            params.SearchParams.AutoSelectOpts == PdfAutoSelectFontOptions::Standard14Alt, stdFont))
    {
        // Create a special element cache that just specify the standard type and encoding
        Element element(
            { },
            stdFont,
            params.Encoding,
            false,
            false);
        auto found = m_fontMap.find(element);
        if (found != m_fontMap.end())
            return found->second.get();

        auto font = PdfFont::CreateStandard14(*m_doc, stdFont, params.Encoding,
            params.FontInitOpts);
        auto inserted = m_fontMap.insert({ element, std::move(font) });
        return inserted.first->second.get();
    }

    string baseFontName;
    PdfFontCreationParams newParams = params;
    if (params.SearchParams.NormalizeFontName)
        baseFontName = PdfFont::ExtractBaseName(fontName, newParams.SearchParams.Bold, newParams.SearchParams.Italic);
    else
        baseFontName = fontName;
    return getFont(baseFontName, newParams);
}

// baseFontName is already normalized and cleaned from known suffixes
PdfFont* PdfFontManager::getFont(const string_view& baseFontName, const PdfFontCreationParams& params)
{
    auto found = m_fontMap.find(Element(
        baseFontName,
        PdfStandard14FontType::Unknown,
        params.Encoding,
        params.SearchParams.Bold,
        params.SearchParams.Italic));
    if (found != m_fontMap.end())
        return found->second.get();

    shared_ptr<chars> buffer = getFontData(baseFontName, params.SearchParams);
    if (buffer == nullptr)
        return nullptr;

    auto metrics = std::make_shared<PdfFontMetricsFreetype>(buffer);
    return this->createFontObject(baseFontName, metrics,
        params.Encoding, params.FontInitOpts);
}

PdfFontMetricsConstPtr PdfFontManager::GetFontMetrics(const string_view& fontName, const PdfFontSearchParams& params)
{
    // Early intercept Standard14 fonts
    PdfStandard14FontType stdFont;
    if (params.AutoSelectOpts != PdfAutoSelectFontOptions::None
        && PdfFontStandard14::IsStandard14Font(fontName,
            params.AutoSelectOpts == PdfAutoSelectFontOptions::Standard14Alt, stdFont))
    {
        return PdfFontMetricsStandard14::GetInstance(stdFont);
    }

    string baseFontName;
    PdfFontSearchParams newParams = params;
    if (params.NormalizeFontName)
        baseFontName = PdfFont::ExtractBaseName(fontName, newParams.Bold, newParams.Italic);
    else
        baseFontName = fontName;

    auto fontData = getFontData(baseFontName, params);
    if (fontData == nullptr)
        return nullptr;

    return PdfFontMetricsConstPtr(new PdfFontMetricsFreetype(std::move(fontData)));
}

unique_ptr<chars> PdfFontManager::getFontData(const string_view& fontName,
    const PdfFontSearchParams& params)
{
    return getFontData(fontName, { }, -1, params);
}

unique_ptr<chars> PdfFontManager::getFontData(const string_view& fontName,
    string filepath, int faceIndex, const PdfFontSearchParams& params)
{
    faceIndex = 0; // TODO, implement searching the face index
    if (filepath.empty())
    {
#ifdef PDFMM_HAVE_FONTCONFIG
        auto fc = ensureInitializedFontConfig();
        filepath = m_fontConfig->GetFontConfigFontPath(fontName, params.Bold, params.Italic);
#endif
    }

    unique_ptr<chars> ret;
    if (!filepath.empty())
        ret = ::getFontData(filepath, faceIndex);

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)
    if (ret == nullptr)
        ret = getWin32FontData(fontName, params);
#endif

    return ret;
}

PdfFont* PdfFontManager::GetFont(FT_Face face, const PdfEncoding& encoding,
    PdfFontInitOptions initOptions)
{
    if (encoding.IsNull())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid encoding");

    string name = PdfFont::ExtractBaseName(FT_Get_Postscript_Name(face));
    if (name.empty())
    {
        PdfError::LogMessage(LogSeverity::Error, "Could not retrieve fontname for font!");
        return nullptr;
    }

    bool bold = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
    bool italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

    auto found = m_fontMap.find(Element(
        name,
        PdfStandard14FontType::Unknown,
        encoding,
        bold,
        italic));
    if (found != m_fontMap.end())
        return found->second.get();

    shared_ptr<PdfFontMetricsFreetype> metrics = PdfFontMetricsFreetype::FromFace(face);
    return this->createFontObject(name, metrics, encoding, initOptions);
}

void PdfFontManager::EmbedSubsetFonts()
{
    for (auto& pair : m_fontMap)
    {
        if (pair.second->IsSubsettingEnabled())
            pair.second->EmbedFontSubset();
    }
}

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)

PdfFont* PdfFontManager::GetFont(HFONT font,
    const PdfEncoding& encoding, PdfFontInitOptions initOptions)
{
    if (font == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Font must be non null");

    if (encoding.IsNull())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Invalid encoding");

    LOGFONTW logFont;
    if (::GetObjectW(font, sizeof(LOGFONTW), &logFont) == 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Invalid font");

    // CHECK-ME: Normalize name or not?
    string fontname;
    utf8::utf16to8((char16_t*)logFont.lfFaceName, (char16_t*)logFont.lfFaceName + LF_FACESIZE, std::back_inserter(fontname));

    auto found = m_fontMap.find(Element(
        fontname,
        PdfStandard14FontType::Unknown,
        encoding,
        logFont.lfWeight >= FW_BOLD ? true : false,
        logFont.lfItalic == 0 ? false : true));

    if (found != m_fontMap.end())
        return found->second.get();

    shared_ptr<chars> buffer = ::getFontData(logFont);
    if (buffer == nullptr)
        return nullptr;

    auto metrics = std::make_shared<PdfFontMetricsFreetype>(buffer);
    return this->createFontObject(fontname, metrics, encoding, initOptions);
}

std::unique_ptr<chars> PdfFontManager::getWin32FontData(
    const string_view& fontName, const PdfFontSearchParams& params)
{
    u16string fontnamew;
    utf8::utf8to16(fontName.begin(), fontName.end(), std::back_inserter(fontnamew));

    // The length of this fontname must not exceed LF_FACESIZE,
    // including the terminating NULL
    if (fontnamew.length() >= LF_FACESIZE)
        return nullptr;

    LOGFONTW lf;
    lf.lfHeight = 0;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = params.Bold ? FW_BOLD : 0;
    lf.lfItalic = params.Italic;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    // NOTE: ANSI_CHARSET should give a consistent result among
    // different locale configurations but sometimes dont' match fonts.
    // We prefer OEM_CHARSET over DEFAULT_CHARSET because it configures
    // the mapper in a way that will match more fonts
    lf.lfCharSet = OEM_CHARSET;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

    memset(lf.lfFaceName, 0, LF_FACESIZE * sizeof(char16_t));
    memcpy(lf.lfFaceName, fontnamew.c_str(), fontnamew.length() * sizeof(char16_t));
    return ::getFontData(lf);
}

#endif // defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)

PdfFont* PdfFontManager::createFontObject(const string_view& fontName,
    const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding,
    PdfFontInitOptions initOptions)
{
    try
    {
        auto font = PdfFont::Create(*m_doc, metrics, encoding, initOptions);
        if (font == nullptr)
            return nullptr;

        auto inserted = m_fontMap.insert({ Element(fontName,
            PdfStandard14FontType::Unknown,
            encoding,
            metrics->IsBold(),
            metrics->IsItalic()), std::move(font) });
        return inserted.first->second.get();
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME(e);
        e.PrintErrorMsg();
        PdfError::LogMessage(LogSeverity::Error, "Cannot initialize font: {}", fontName);
        return nullptr;
    }
}

#ifdef PDFMM_HAVE_FONTCONFIG

void PdfFontManager::SetFontConfigWrapper(const shared_ptr<PdfFontConfigWrapper>& fontConfig)
{
    if (m_fontConfig == fontConfig)
        return;

    if (fontConfig == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Fontconfig wrapper can't be null");

    m_fontConfig = fontConfig;
}

PdfFontConfigWrapper& PdfFontManager::GetFontConfigWrapper()
{
    auto ret = ensureInitializedFontConfig();
    return *ret;
}

shared_ptr<PdfFontConfigWrapper> PdfFontManager::ensureInitializedFontConfig()
{
    auto ret = m_fontConfig;
    if (ret == nullptr)
    {
        ret.reset(new PdfFontConfigWrapper());
        m_fontConfig = ret;
    }

    return ret;
}

#endif // PDFMM_HAVE_FONTCONFIG

PdfFontManager::Element::Element(const string_view& fontname, PdfStandard14FontType stdType,
    const PdfEncoding& encoding, bool bold, bool italic) :
    FontName(fontname),
    StdType(stdType),
    EncodingId(encoding.GetId()),
    Bold(bold),
    Italic(italic) { }

size_t PdfFontManager::HashElement::operator()(const Element& elem) const
{
    size_t hash = 0;
    utls::hash_combine(hash, elem.FontName, elem.StdType, elem.EncodingId, elem.Bold, elem.Italic);
    return hash;
};

bool PdfFontManager::EqualElement::operator()(const Element& lhs, const Element& rhs) const
{
    return lhs.EncodingId == rhs.EncodingId && lhs.Bold == rhs.Bold
        && lhs.Italic == rhs.Italic && lhs.FontName == rhs.FontName;
}

unique_ptr<chars> getFontData(const string_view& filename, unsigned short faceIndex)
{
    FT_Error rc;
    FT_ULong length = 0;
    unique_ptr<chars> buffer;
    FT_Face face;
    rc = FT_New_Face(mm::GetFreeTypeLibrary(), filename.data(), faceIndex, &face);
    if (rc != 0)
    {
        // throw an exception
        PdfError::LogMessage(LogSeverity::Error, "FreeType returned the error {} when calling FT_New_Face for font {}",
            (int)rc, filename);
        return nullptr;
    }

    unique_ptr<FT_FaceRec_, decltype(&FT_Done_Face)> face_(face, FT_Done_Face);
    rc = FT_Load_Sfnt_Table(face, 0, 0, nullptr, &length);
    if (rc != 0)
        goto Error;

    buffer = std::make_unique<chars>(length);
    rc = FT_Load_Sfnt_Table(face, 0, 0, reinterpret_cast<FT_Byte*>(buffer->data()), &length);
    if (rc != 0)
        goto Error;

    return buffer;
Error:
    PdfError::LogMessage(LogSeverity::Error, "FreeType returned the error {} when calling FT_Load_Sfnt_Table for font {}",
        (int)rc, filename);
    return nullptr;
}

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)

unique_ptr<chars> getFontData(const LOGFONTW& inFont)
{
    bool success = false;
    chars buffer;
    HDC hdc = ::CreateCompatibleDC(nullptr);
    HFONT hf = CreateFontIndirectW(&inFont);
    if (hf != nullptr)
    {
        success = getFontData(buffer, hdc, hf);
        DeleteObject(hf);
    }
    ReleaseDC(0, hdc);

    if (success)
        return std::make_unique<chars>(std::move(buffer));
    else
        return nullptr;
}

bool getFontData(chars& buffer, HDC hdc, HFONT hf)
{
    HGDIOBJ oldFont = SelectObject(hdc, hf);
    bool sucess = false;

    // try get data from true type collection
    constexpr DWORD ttcf_const = 0x66637474;
    unsigned fileLen = GetFontData(hdc, 0, 0, nullptr, 0);
    unsigned ttcLen = GetFontData(hdc, ttcf_const, 0, nullptr, 0);

    if (fileLen != GDI_ERROR)
    {
        if (ttcLen == GDI_ERROR)
        {
            buffer.resize(fileLen);
            sucess = GetFontData(hdc, 0, 0, buffer.data(), (DWORD)fileLen) != GDI_ERROR;
        }
        else
        {
            chars fileBuffer(fileLen);
            if (GetFontData(hdc, ttcf_const, 0, fileBuffer.data(), fileLen) == GDI_ERROR)
            {
                sucess = false;
                goto Exit;
            }

            chars ttcBuffer(ttcLen);
            if (GetFontData(hdc, 0, 0, ttcBuffer.data(), ttcLen) == GDI_ERROR)
            {
                sucess = false;
                goto Exit;
            }

            getFontDataTTC(buffer, fileBuffer, ttcBuffer);
            sucess = true;
        }
    }

Exit:
    // clean up
    SelectObject(hdc, oldFont);
    return sucess;
}

// This function will recieve the device context for the
// TrueType Collection font, it will then extract necessary,
// tables and create the correct buffer.
void getFontDataTTC(chars& buffer, const chars& fileBuffer, const chars& ttcBuffer)
{
    uint16_t numTables = FROM_BIG_ENDIAN(*(uint16_t*)(ttcBuffer.data() + 4));
    unsigned outLen = 12 + 16 * numTables;
    const char* entry = ttcBuffer.data() + 12;

    //us: see "http://www.microsoft.com/typography/otspec/otff.htm"
    for (unsigned i = 0; i < numTables; i++)
    {
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(entry + 12));
        length = (length + 3) & ~3;
        entry += 16;
        outLen += length;
    }

    buffer.resize(outLen);

    // copy font header and table index (offsets need to be still adjusted)
    memcpy(buffer.data(), ttcBuffer.data(), 12 + 16 * numTables);
    uint32_t dstDataOffset = 12 + 16 * numTables;

    // process tables
    const char* srcEntry = ttcBuffer.data() + 12;
    char* dstEntry = buffer.data() + 12;
    for (unsigned i = 0; i < numTables; i++)
    {
        // read source entry
        uint32_t offset = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 8));
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 12));
        length = (length + 3) & ~3;

        // adjust offset
        // U can use FromBigEndian() also to convert _to_ big endian
        *(uint32_t*)(dstEntry + 8) = FROM_BIG_ENDIAN(dstDataOffset);

        //copy data
        memcpy(buffer.data() + dstDataOffset, fileBuffer.data() + offset, length);
        dstDataOffset += length;

        // adjust table entry pointers for loop
        srcEntry += 16;
        dstEntry += 16;
    }
}

#endif // defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)
