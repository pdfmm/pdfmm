/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFont.h"

#include <sstream>
#include <regex>

#include <pdfmm/private/PdfEncodingPrivate.h>

#include "PdfArray.h"
#include "PdfEncoding.h"
#include "PdfEncodingFactory.h"
#include "PdfInputStream.h"
#include "PdfStream.h"
#include "PdfWriter.h"
#include "PdfLocale.h"
#include "PdfCharCodeMap.h"
#include "PdfEncodingShim.h"
#include "PdfFontMetrics.h"
#include "PdfPage.h"
#include <pdfmm/private/PdfStandard14FontData.h>

using namespace std;
using namespace mm;

// kind of ABCDEF+
static string_view genSubsetPrefix();

PdfFont::PdfFont(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfDictionaryElement(doc, "Font"), m_Metrics(metrics), m_IsObjectLoaded(false)
{
    this->initBase(encoding);
}

PdfFont::PdfFont(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfDictionaryElement(obj), m_Metrics(metrics), m_IsObjectLoaded(true)
{
    this->initBase(encoding);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    ostringstream out;
    PdfLocaleImbue(out);
    out << "pdfmmFt" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.str().c_str());
}

PdfFont::~PdfFont() { }

void PdfFont::initBase(const PdfEncoding& encoding)
{
    m_IsEmbedded = false;
    m_EmbeddingEnabled = false;
    m_SubsettingEnabled = false;

    if (m_Metrics == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Metrics must me not null");

    if (encoding.GetId() == DynamicEncodingId)
    {
        m_DynCharCodeMap = std::make_shared<PdfCharCodeMap>();
        m_Encoding.reset(new PdfDynamicEncoding(m_DynCharCodeMap, *this));
    }
    else
    {
        m_Encoding.reset(new PdfEncodingShim(encoding, *this));
    }

    ostringstream out;
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ft" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.str().c_str());

    // By default ensure the font has the /BaseFont name read
    // from the loaded metrics or inferred from a font file
    m_Name = m_Metrics->GetFontNameSafe();
}

void PdfFont::WriteStringToStream(PdfStream& stream, const string_view& str) const
{
    stringstream ostream;
    WriteStringToStream(ostream, str);
    stream.Append(ostream.str());
}

void PdfFont::WriteStringToStream(ostream& stream, const string_view& str) const
{
    auto encoded = m_Encoding->ConvertToEncoded(str);
    size_t len = 0;

    unique_ptr<PdfFilter> filter = PdfFilterFactory::Create(PdfFilterType::ASCIIHexDecode);
    unique_ptr<char[]> buffer;
    filter->Encode(encoded.data(), encoded.size(), buffer, len);

    stream << "<";
    stream.write(buffer.get(), len);
    stream << ">";
}

void PdfFont::InitImported(bool embeddingEnabled, bool subsettingEnabled)
{
    PDFMM_ASSERT(!m_IsObjectLoaded);
    if (subsettingEnabled && SupportsSubsetting())
    {
        // Subsetting implies embedded
        m_SubsettingEnabled = true;
        m_EmbeddingEnabled = true;
    }
    else
    {
        m_SubsettingEnabled = false;
        m_EmbeddingEnabled = embeddingEnabled;
    }

    if (m_SubsettingEnabled)
    {
        unsigned gid;
        char32_t spaceCp = U' ';
        if (m_Metrics->TryGetGID(spaceCp, gid))
        {
            // If it exist a glyph for space character
            // always add it for subsetting
            AddUsedGID(gid, cspan<char32_t>(&spaceCp, 1));
        }
    }

    string fontName;
    if (m_Metrics->IsStandard14FontMetrics())
    {
        fontName = m_Metrics->GetFontName();
    }
    else
    {
        fontName = m_Metrics->GetBaseFontName();
        if (m_Metrics->IsBold())
        {
            if (m_Metrics->IsItalic())
                fontName += ",BoldItalic";
            else
                fontName += ",Bold";
        }
        else if (m_Metrics->IsItalic())
        {
            fontName += ",Italic";
        }
    }

    if (m_SubsettingEnabled)
    {
        m_SubsetPrefix = genSubsetPrefix();
        PDFMM_ASSERT(!m_SubsetPrefix.empty());
        fontName = m_SubsetPrefix + fontName;
    }

    m_Name = fontName;
    initImported();

    if (m_EmbeddingEnabled && !m_SubsettingEnabled)
    {
        // Regular embedding is not done if subsetting is enabled
        embedFont();
        m_IsEmbedded = true;
    }
}

void PdfFont::EmbedFontSubset()
{
    if (m_IsEmbedded || !m_EmbeddingEnabled || !m_SubsettingEnabled)
        return;

    embedFontSubset();
    m_IsEmbedded = true;
}

void PdfFont::embedFont()
{
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Embedding not implemented for this font type");
}

void PdfFont::embedFontSubset()
{
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "Subsetting not implemented for this font type");
}

double PdfFont::GetStringWidth(const string_view& view, const PdfTextState& state) const
{
    // Ignore failures
    double width;
    (void)TryGetStringWidth(view, state, width);
    return width;
}

bool PdfFont::TryGetStringWidth(const string_view& view, const PdfTextState& state, double& width) const
{
    vector<PdfCID> cids;
    bool success = true;
    if (!m_Encoding->TryConvertToCIDs(view, cids))
        success = false;

    width = getStringWidth(cids, state);
    return success;
}

double PdfFont::GetStringWidth(const PdfString& encodedStr, const PdfTextState& state) const
{
    // Ignore failures
    double width;
    (void)TryGetStringWidth(encodedStr, state, width);
    return width;
}

bool PdfFont::TryGetStringWidth(const PdfString& encodedStr, const PdfTextState& state, double& width) const
{
    vector<PdfCID> cids;
    bool success = true;
    if (!m_Encoding->TryConvertToCIDs(encodedStr, cids))
        success = false;

    width = getStringWidth(cids, state);
    return success;
}

double PdfFont::GetCharWidth(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing) const
{
    // Ignore failures
    double width;
    if (!TryGetCharWidth(codePoint, state, ignoreCharSpacing, width))
        return GetDefaultCharWidth(state, ignoreCharSpacing);

    return width;
}

bool PdfFont::TryGetCharWidth(char32_t codePoint, const PdfTextState& state, double& width) const
{
    return TryGetCharWidth(codePoint, state, false, width);
}

bool PdfFont::TryGetCharWidth(char32_t codePoint, const PdfTextState& state,
    bool ignoreCharSpacing, double& width) const
{
    bool success = true;
    PdfCID cid;
    if (!m_Encoding->TryGetCID(codePoint, cid))
    {
        width = -1;
        return false;
    }

    width = getCIDWidth(cid.Id, state, ignoreCharSpacing);
    return success;
}

double PdfFont::GetDefaultCharWidth(const PdfTextState& state, bool ignoreCharSpacing) const
{
    if (ignoreCharSpacing)
    {
        return m_Metrics->GetDefaultCharWidth() * state.GetFontSize()
            * state.GetFontScale();
    }
    else
    {
        return (m_Metrics->GetDefaultCharWidth() * state.GetFontSize()
            + state.GetCharSpace()) * state.GetFontScale();
    }
}

double PdfFont::GetCIDWidthRaw(unsigned cid) const
{
    unsigned gid;
    if (m_Encoding->HasCIDMapping())
    {
        if (!TryMapCIDToGID(cid, gid))
            return m_Metrics->GetDefaultCharWidth();
    }
    else
    {
        char32_t mappedCodePoint = m_Encoding->GetCodePoint(cid);
        if (mappedCodePoint == U'\0'
            || !m_Metrics->TryGetGID(mappedCodePoint, gid))
        {
            return m_Metrics->GetDefaultCharWidth();
        }
    }

    return m_Metrics->GetGlyphWidth(gid);
}

void PdfFont::GetBoundingBox(PdfArray& arr) const
{
    // TODO: Handle custom measure units, like for /Type3 fonts ?
    arr.Clear();
    vector<double> bbox;
    m_Metrics->GetBoundingBox(bbox);
    arr.push_back(PdfObject(static_cast<int64_t>(std::round(bbox[0] * 1000))));
    arr.push_back(PdfObject(static_cast<int64_t>(std::round(bbox[1] * 1000))));
    arr.push_back(PdfObject(static_cast<int64_t>(std::round(bbox[2] * 1000))));
    arr.push_back(PdfObject(static_cast<int64_t>(std::round(bbox[3] * 1000))));

}

void PdfFont::FillDescriptor(PdfDictionary& dict) const
{
    // Setting the FontDescriptor paras:
    PdfArray bbox;
    GetBoundingBox(bbox);

    // TODO: Handle custom measure units, like for /Type3 fonts, for Ascent/Descent/CapHeight...
    dict.AddKey("FontName", PdfName(this->GetName()));
    dict.AddKey(PdfName::KeyFlags, PdfObject(static_cast<int64_t>(m_Metrics->GetFlags())));
    dict.AddKey("FontBBox", bbox);
    dict.AddKey("ItalicAngle", static_cast<int64_t>(m_Metrics->GetItalicAngle()));
    dict.AddKey("Ascent", static_cast<int64_t>(std::round(m_Metrics->GetAscent() * 1000)));
    dict.AddKey("Descent", static_cast<int64_t>(m_Metrics->GetDescent() * 1000));
    dict.AddKey("CapHeight", static_cast<int64_t>(m_Metrics->GetCapHeight() * 1000));
    dict.AddKey("XHeight", static_cast<int64_t>(m_Metrics->GetXHeight() * 1000));
    dict.AddKey("StemV", static_cast<int64_t>(m_Metrics->GetStemV() * 1000));
}

void PdfFont::initImported()
{
    // By default do nothing
}

double PdfFont::getStringWidth(const vector<PdfCID>& cids, const PdfTextState& state) const
{
    double width = 0;
    for (auto& cid : cids)
        width += getCIDWidth(cid.Id, state, false);

    return width;
}

// TODO:
// Handle word spacing Tw
// 5.2.2 Word Spacing
// Note: Word spacing is applied to every occurrence of the single-byte character code
// 32 in a string when using a simple font or a composite font that defines code 32 as a
// single - byte code.It does not apply to occurrences of the byte value 32 in multiplebyte
// codes.
double PdfFont::getCIDWidth(unsigned cid, const PdfTextState& state, bool ignoreCharSpacing) const
{
    double charWidth = GetCIDWidthRaw(cid);
    if (ignoreCharSpacing)
        return charWidth * state.GetFontSize() * state.GetFontScale();
    else
        return (charWidth * state.GetFontSize() + state.GetCharSpace()) * state.GetFontScale();
}

double PdfFont::GetLineSpacing(const PdfTextState& state) const
{
    return m_Metrics->GetLineSpacing() * state.GetFontSize();
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetUnderlineThickness(const PdfTextState& state) const
{
    return m_Metrics->GetUnderlineThickness() * state.GetFontSize();
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetUnderlinePosition(const PdfTextState& state) const
{
    return m_Metrics->GetUnderlinePosition() * state.GetFontSize();
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetStrikeOutPosition(const PdfTextState& state) const
{
    return m_Metrics->GetStrikeOutPosition() * state.GetFontSize();
}

// CHECK-ME Should state.GetFontScale() be considered?
double PdfFont::GetStrikeOutThickness(const PdfTextState& state) const
{
    return m_Metrics->GetStrikeOutThickness() * state.GetFontSize();
}

double PdfFont::GetAscent(const PdfTextState& state) const
{
    return m_Metrics->GetAscent() * state.GetFontSize();
}

double PdfFont::GetDescent(const PdfTextState& state) const
{
    return m_Metrics->GetDescent() * state.GetFontSize();
}

PdfCID PdfFont::AddUsedGID(unsigned gid, const cspan<char32_t>& codePoints)
{
    PDFMM_ASSERT(!m_IsObjectLoaded);
    if (m_SubsettingEnabled && m_IsEmbedded)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "Can't add more subsetting glyphs on an already embedded font");
    }

    if (m_DynCharCodeMap == nullptr)
    {
        auto found = m_UsedGIDs.find(gid);
        if (found != m_UsedGIDs.end())
            return found->second;

        PdfCharCode codeUnit;
        if (!m_Encoding->GetToUnicodeMapSafe().TryGetCharCode(codePoints, codeUnit))
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "The encoding doesn't support these characters");

        // We start numberings CIDs from 1 since
        // CID 0 is reserved for fallbacks
        m_UsedGIDs[gid] = PdfCID((unsigned)m_UsedGIDs.size() + 1, codeUnit);
        return codeUnit;
    }
    else
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "TODO");
    }
}

bool PdfFont::SupportsSubsetting() const
{
    return false;
}

PdfObject& PdfFont::GetDescendantFontObject()
{
    auto obj = getDescendantFontObject();
    if (obj == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Descendant font object must not be null");

    return *obj;
}

bool PdfFont::TryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    // By default it's identity with cid
    gid = cid;
    return true;
}

bool PdfFont::TryMapGIDToCID(unsigned gid, unsigned& cid) const
{
    // By default it's identity with gid
    cid = gid;
    return true;
}

PdfObject* PdfFont::getDescendantFontObject()
{
    // By default return null
    return nullptr;
}

string_view genSubsetPrefix()
{
    static constexpr unsigned SUBSET_PREFIX_LEN = 6; // + 2 for "+\0"
    struct SubsetBaseNameCtx
    {
        SubsetBaseNameCtx()
            : Prefix{ }
        {
            char* p = Prefix;
            for (unsigned i = 0; i < SUBSET_PREFIX_LEN; i++, p++)
                *p = 'A';

            Prefix[SUBSET_PREFIX_LEN] = '+';
            Prefix[0]--;
        }
        char Prefix[SUBSET_PREFIX_LEN + 2];
    };

    static SubsetBaseNameCtx s_ctx;

    for (unsigned i = 0; i < SUBSET_PREFIX_LEN; i++)
    {
        s_ctx.Prefix[i]++;
        if (s_ctx.Prefix[i] <= 'Z')
            break;

        s_ctx.Prefix[i] = 'A';
    }

    return s_ctx.Prefix;
}

string PdfFont::ExtractBaseName(const string_view& fontName, bool& isBold, bool& isItalic)
{
    // TABLE H.3 Names of standard fonts
    string name = (string)fontName;
    regex regex;
    smatch matches;
    isBold = false;
    isItalic = false;
    
    // NOTE: For some reasons, "^[A-Z]{6}\+" doesn't work
    regex = std::regex("^[A-Z][A-Z][A-Z][A-Z][A-Z][A-Z]\\+", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        // 5.5.3 Font Subsets: Remove EOODIA+ like prefixes
        name.erase(matches[0].first - name.begin(), 7);
    }

    regex = std::regex("[,-]BoldItalic", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("BoldItalic") + 1);
        isBold = true;
        isItalic = true;
    }

    regex = std::regex("[,-]BoldOblique", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("BoldOblique") + 1);
        isBold = true;
        isItalic = true;
    }

    regex = std::regex("[,-]Bold", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Bold") + 1);
        isBold = true;
    }

    regex = std::regex("[,-]Italic", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Italic") + 1);
        isItalic = true;
    }

    regex = std::regex("[,-]Oblique", regex_constants::ECMAScript);
    if (std::regex_search(name, matches, regex))
    {
        name.erase(matches[0].first - name.begin(), char_traits<char>::length("Oblique") + 1);
        isItalic = true;
    }

    // 5.5.2 TrueType Fonts: If the name contains any spaces, the spaces are removed
    name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
    return name;
}

string PdfFont::ExtractBaseName(const string_view& fontName)
{
    bool isBold;
    bool isItalic;
    return ExtractBaseName(fontName, isBold, isItalic);
}

string_view PdfFont::GetStandard14FontName(PdfStandard14FontType stdFont)
{
    return ::GetStandard14FontName(stdFont);
}

bool PdfFont::IsStandard14Font(const string_view& fontName, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, true, stdFont);
}

bool PdfFont::IsStandard14Font(const string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, useAltNames, stdFont);
}

bool PdfFont::IsCIDKeyed() const
{
    switch (GetType())
    {
        case PdfFontType::CIDTrueType:
        case PdfFontType::CIDType1:
            return true;
        default:
            return false;
    }
}
