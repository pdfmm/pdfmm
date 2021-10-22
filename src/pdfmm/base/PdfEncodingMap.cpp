/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfEncodingMap.h"

#include <utfcpp/utf8.h>

#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfEncodingMap::PdfEncodingMap(const PdfEncodingLimits& limits)
    : m_limits(limits)
{
}

PdfEncodingMap::~PdfEncodingMap() { }

void PdfEncodingMap::GetExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    name = { };
    obj = nullptr;
    getExportObject(objects, name, obj);
}

void PdfEncodingMap::getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    (void)objects;
    (void)name;
    (void)obj;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfEncodingMap::TryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end, PdfCharCode& code) const
{
    if (it == end)
    {
        code = { };
        return false;
    }

    if (HasLigaturesSupport())
    {
        auto temp = it;
        if (!tryGetNextCharCode(temp, end, code))
        {
            code = { };
            return false;
        }

        it = temp;
        return true;
    }
    else
    {
        char32_t cp = (char32_t)utf8::next(it, end);
        return tryGetCharCode(cp, code);
    }
}

bool PdfEncodingMap::tryGetNextCharCode(std::string_view::iterator& it, const std::string_view::iterator& end, PdfCharCode& codeUnit) const
{
    (void)it;
    (void)end;
    (void)codeUnit;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfEncodingMap::tryGetCharCodeSpan(const cspan<char32_t>& ligature, PdfCharCode& codeUnit) const
{
    (void)ligature;
    (void)codeUnit;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

bool PdfEncodingMap::TryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    return tryGetCharCode(codePoint, codeUnit);
}

bool PdfEncodingMap::TryGetCharCode(const cspan<char32_t>& codePoints, PdfCharCode& codeUnit) const
{
    if (codePoints.size() == 1)
    {
        return tryGetCharCode(codePoints[0], codeUnit);
    }
    else if (codePoints.size() == 0 || !HasLigaturesSupport())
    {
        codeUnit = { };
        return false;
    }

    // Try to lookup the ligature
    PDFMM_INVARIANT(codePoints.size() > 1);
    return tryGetCharCodeSpan(codePoints, codeUnit);
}

bool PdfEncodingMap::TryGetCharCode(unsigned cid, PdfCharCode& codeUnit) const
{
    // NOTE: getting the char code from a cid on this map is the same operation
    // as getting it from an unicode code point
    return tryGetCharCode((char32_t)cid, codeUnit);
}

bool PdfEncodingMap::TryGetNextCID(string_view::iterator& it,
    const string_view::iterator& end, PdfCID& cid) const
{
    if (HasCIDMapping())
    {
        vector<char32_t> codePoints;
        bool success = tryGetNextCodePoints(it, end, cid.Unit, codePoints);
        if (!success || codePoints.size() != 1)
        {
            // Return false on missing lookup or malformed multiple code points found
            cid = { };
            return false;
        }

        // char32_t is also unsigned
        cid.Id = (unsigned)codePoints[0];
        return true;
    }
    else
    {
        // If there's no CID mapping, we just iterate character codes
        // and convert to a CID using /FirstChar

        // Save current iterator in the case the search is unsuccessful
        string_view::iterator curr = it;

        unsigned code = 0;
        unsigned char i = 1;
        PDFMM_ASSERT(i <= m_limits.MaxCodeSize);
        while (curr != end)
        {
            // Iterate the string and accumulate a
            // code of the appropriate code size
            code <<= 8;
            code |= (unsigned char)*curr;
            curr++;
            if (i == m_limits.MaxCodeSize)
            {
                // Map the character code to a CID with /FirstChar
                cid.Unit = { code, m_limits.MaxCodeSize };
                cid.Id = code - m_limits.FirstChar.Code;
                it = curr;
                return true;
            }
            i++;
        }

        cid = { };
        return false;
    }
}

bool PdfEncodingMap::TryGetNextCodePoints(string_view::iterator& it,
    const string_view::iterator& end, vector<char32_t>& codePoints) const
{
    codePoints.clear();
    PdfCharCode code;
    return tryGetNextCodePoints(it, end, code, codePoints);
}

bool PdfEncodingMap::TryGetCIDId(const PdfCharCode& codeUnit, unsigned& id) const
{
    if (HasCIDMapping())
    {
        vector<char32_t> codePoints;
        bool success = tryGetCodePoints(codeUnit, codePoints);
        if (!success || codePoints.size() != 1)
        {
            // Return false on missing lookup or malformed multiple code points found
            return false;
        }

        // char32_t is also unsigned
        id = (unsigned)codePoints[0];
        return true;
    }
    else
    {
        // If there's no CID mapping, we just convert to a CID using /FirstChar
        id = codeUnit.Code - m_limits.FirstChar.Code;
        return true;
    }
}

bool PdfEncodingMap::TryGetCodePoints(const PdfCharCode& codeUnit, vector<char32_t>& codePoints) const
{
    codePoints.clear();
    return tryGetCodePoints(codeUnit, codePoints);
}

bool PdfEncodingMap::HasCIDMapping() const
{
    return false;
}

bool PdfEncodingMap::HasLigaturesSupport() const
{
    return false;
}

void PdfEncodingMap::WriteToUnicodeCMap(PdfObject& cmapObj) const
{
    if (m_limits.MaxCodeSize > 1)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "TODO");

    // CMap specification is in Adobe technical node #5014
    // The /ToUnicode dictionary doesn't need /CMap type, /CIDSystemInfo or /CMapName
    auto& stream = cmapObj.GetOrCreateStream();
    stream.BeginAppend();
    stream.Append(
        "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo <<\n"
        "   /Registry (Adobe)\n"
        "   /Ordering (UCS)\n"
        "   /Supplement 0\n"
        ">> def\n"
        "/CMapName /Adobe-Identity-UCS def\n"
        "/CMapType 2 def\n"     // As defined in Adobe Technical Notes #5099
        "1 begincodespacerange\n"
        "<00> <FF>\n"
        "endcodespacerange\n");

    appendBaseFontEntries(stream);
    stream.Append(
        "\nendcmap\n"
        "CMapName currentdict / CMap defineresource pop\n"
        "end\n"
        "end");
    stream.EndAppend();
}

// NOTE: Don't clear the result on failure. It is done externally
bool PdfEncodingMap::tryGetNextCodePoints(string_view::iterator& it, const string_view::iterator& end,
    PdfCharCode& codeUnit, vector<char32_t>& codePoints) const
{
    // Save current iterator in the case the search is unsuccessful
    string_view::iterator curr = it;

    unsigned code = 0;
    unsigned char i = 1;
    while (curr != end)
    {
        if (i > m_limits.MaxCodeSize)
            return false;

        // CMap Mapping, PDF Reference 1.7, pg. 453
        // A sequence of one or more bytes is extracted from the string and matched against
        // the codespace ranges in the CMap. That is, the first byte is matched against 1-byte
        // codespace ranges; if no match is found, a second byte is extracted, and the 2-byte
        // srcCode is matched against 2-byte codespace ranges. This process continues for successively
        // longer codes until a match is found or all codespace ranges have been
        // tested. There will be at most one match because codespace ranges do not overlap.

        code <<= 8;
        code |= (uint8_t)*curr;
        curr++;
        codeUnit = { code, i };
        if (i < m_limits.MinCodeSize || !tryGetCodePoints(codeUnit, codePoints))
        {
            i++;
            continue;
        }

        it = curr;
        return true;
    }

    return false;
}

PdfEncodingMapBase::PdfEncodingMapBase(PdfCharCodeMap&& map, const nullable<PdfEncodingLimits>& limits) :
    PdfEncodingMap(limits.has_value() ? *limits : findLimits(map)),
    m_charMap(std::make_shared<PdfCharCodeMap>(std::move(map)))
{
}

PdfEncodingMapBase::PdfEncodingMapBase(const shared_ptr<PdfCharCodeMap>& map)
    : PdfEncodingMap({ }), m_charMap(map)
{
    if (map == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Map must be not null");
}

bool PdfEncodingMapBase::tryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end,
    PdfCharCode& codeUnit) const
{
    return m_charMap->TryGetNextCharCode(it, end, codeUnit);
}

bool PdfEncodingMapBase::tryGetCharCodeSpan(const cspan<char32_t>& codePoints, PdfCharCode& codeUnit) const
{
    return m_charMap->TryGetCharCode(codePoints, codeUnit);
}

bool PdfEncodingMapBase::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    return m_charMap->TryGetCharCode(codePoint, codeUnit);
}

bool PdfEncodingMapBase::tryGetCodePoints(const PdfCharCode& code, vector<char32_t>& codePoints) const
{
    return m_charMap->TryGetCodePoints(code, codePoints);
}

void PdfEncodingMapBase::appendBaseFontEntries(PdfStream& stream) const
{
    // Very easy, just do a list of bfchar
    // Use PdfEncodingMap::AppendUTF16CodeTo
    (void)stream;
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "TODO");
}

PdfEncodingLimits PdfEncodingMapBase::findLimits(const PdfCharCodeMap& map)
{
    PdfEncodingLimits limits;
    for (auto& pair : map)
    {
        if (pair.first.Code < limits.FirstChar.Code)
            limits.FirstChar = pair.first;

        if (pair.first.Code > limits.LastChar.Code)
            limits.LastChar = pair.first;

        if (pair.first.CodeSpaceSize < limits.MinCodeSize)
            limits.MinCodeSize = pair.first.CodeSpaceSize;

        if (pair.first.CodeSpaceSize > limits.MaxCodeSize)
            limits.MaxCodeSize = pair.first.CodeSpaceSize;
    }

    return limits;
}

void PdfEncodingMap::AppendUTF16CodeTo(PdfStream& stream, const cspan<char32_t>& codePoints, u16string& u16tmp)
{
    char hexbuf[2];
    stream.Append("<");
    bool first = true;
    for (unsigned i = 0; i < codePoints.size(); i++)
    {
        if (first)
            first = false;
        else
            stream.Append(" "); // Separate each character in the ligatures

        char32_t cp = codePoints[i];
        utls::WriteToUtf16BE(u16tmp, cp);

        auto data = (const char*)u16tmp.data();
        size_t size = u16tmp.size() * sizeof(char16_t);
        for (unsigned l = 0; l < size; l++)
        {
            // Append hex codes of the converted utf16 string
            utls::WriteCharHexTo(hexbuf, data[l]);
            stream.Append(hexbuf, std::size(hexbuf));
        }
    }
    stream.Append(">");
}

void PdfEncodingMapSimple::appendBaseFontEntries(PdfStream& stream) const
{
    auto& limits = GetLimits();
    PDFMM_ASSERT(limits.MaxCodeSize == 1);
    vector<char32_t> codePoints;
    unsigned code = limits.FirstChar.Code;
    unsigned lastCode = limits.LastChar.Code;
    string codeStr;
    stream.Append("1 beginbfrange\n");
    limits.FirstChar.WriteHexTo(codeStr);
    stream.Append(codeStr).Append(" ");
    limits.LastChar.WriteHexTo(codeStr);
    stream.Append(codeStr).Append(" [\n");
    u16string u16tmp;
    for (; code < lastCode; code++)
    {
        if (!TryGetCodePoints(PdfCharCode(code), codePoints))
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Unable to find character code");

        AppendUTF16CodeTo(stream, codePoints, u16tmp);
        stream.Append("\n");
    }
    stream.Append("]\n");
    stream.Append("endbfrange");
}

PdfDummyEncodingMap::PdfDummyEncodingMap() : PdfEncodingMap({ }) { }

bool PdfDummyEncodingMap::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    (void)codePoint;
    (void)codeUnit;
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "PdfDynamicEncoding can't be used only from a PdfFont");
}

bool PdfDummyEncodingMap::tryGetCodePoints(const PdfCharCode& codeUnit, vector<char32_t>& codePoints) const
{
    (void)codeUnit;
    (void)codePoints;
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "PdfDynamicEncoding can't be used only from a PdfFont");
}

void PdfDummyEncodingMap::appendBaseFontEntries(PdfStream& stream) const
{
    (void)stream;
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "PdfDynamicEncoding can't be used only from a PdfFont");
}

PdfEncodingLimits::PdfEncodingLimits(unsigned char minCodeSize, unsigned char maxCodeSize,
        const PdfCharCode& firstChar, const PdfCharCode& lastChar) :
    MinCodeSize(minCodeSize),
    MaxCodeSize(maxCodeSize),
    FirstChar(firstChar),
    LastChar(lastChar)
{
}

PdfEncodingLimits::PdfEncodingLimits() :
    PdfEncodingLimits(numeric_limits<unsigned char>::max(), 0, PdfCharCode(numeric_limits<unsigned>::max()), PdfCharCode(0))
{
}

PdfCID::PdfCID()
    : Id(0)
{
}

PdfCID::PdfCID(unsigned id)
    : Id(id), Unit(id)
{
}

PdfCID::PdfCID(unsigned id, const PdfCharCode& unit)
    : Id(id), Unit(unit)
{
}

PdfCID::PdfCID(const PdfCharCode& unit)
    : Id(unit.Code), Unit(unit)
{
}
