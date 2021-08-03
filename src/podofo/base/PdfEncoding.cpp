/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfEncoding.h"

#include <sstream>
#include <atomic>
#include <utfcpp/utf8.h>

#include <doc/PdfDocument.h>
#include "PdfDictionary.h"
#include "PdfEncodingPrivate.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace PoDoFo;

static atomic<size_t> s_nextid = CustomEncodingStartId;

static PdfCharCode fetchFallbackCharCode(string_view::iterator& it, const string_view::iterator& end, const PdfEncodingLimits& limits);
static PdfCharCode getFallbackCharCode(char32_t codePoint, const PdfEncodingLimits& limits);
static PdfCID getFallbackCID(char32_t codePoint, const PdfEncodingMap& toUnicode, const PdfEncodingMap& encoding);
static PdfCID getCID(const PdfCharCode& codeUnit, const PdfEncodingMap& map, bool& success);
static void fillCIDToGIDMap(PdfObject& cmapObj, const UsedGIDsMap& usedGIDs, const string_view& baseFont);
static size_t getNextId();

PdfEncoding::PdfEncoding()
    : PdfEncoding(NullEncodingId, PdfEncodingMapFactory::GetDummyEncodingMap(), nullptr)
{
}

PdfEncoding::PdfEncoding(const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode)
    : PdfEncoding(getNextId(), encoding, toUnicode)
{
}

PdfEncoding::PdfEncoding(size_t id, const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode)
    : m_Id(id), m_Encoding(encoding), m_ToUnicode(toUnicode)
{
    if (encoding == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Main encoding must be not null");
}

PdfEncoding::PdfEncoding(const PdfObject& fontObj, const PdfEncodingMapConstPtr& encoding, const PdfEncodingMapConstPtr& toUnicode)
    : PdfEncoding(encoding, toUnicode)
{
    auto firstCharObj = fontObj.GetDictionary().FindKey("FirstChar");
    if (firstCharObj != nullptr)
        m_Limits.FirstChar = PdfCharCode(static_cast<unsigned>(firstCharObj->GetNumber()));

    auto lastCharObj = fontObj.GetDictionary().FindKey("LastChar");
    if (lastCharObj != nullptr)
        m_Limits.LastChar = PdfCharCode(static_cast<unsigned>(lastCharObj->GetNumber()));

    if (m_Limits.LastChar.Code > m_Limits.FirstChar.Code)
    {
        // If found valid /FirstChar and /LastChar, valorize
        //  also the code size limits
        m_Limits.MinCodeSize = usr::GetCharCodeSize(m_Limits.FirstChar.Code);
        m_Limits.MaxCodeSize = usr::GetCharCodeSize(m_Limits.LastChar.Code);
    }
}

PdfEncoding::~PdfEncoding() { }

string PdfEncoding::ConvertToUtf8(const PdfString& encodedStr) const
{
    // Just ignore failures
    string ret;
    (void)tryConvertEncodedToUtf8(encodedStr.GetRawData(), ret);
    return ret;
}

bool PdfEncoding::TryConvertToUtf8(const PdfString& encodedStr, string& str) const
{
    return tryConvertEncodedToUtf8(encodedStr.GetRawData(), str);
}

string PdfEncoding::ConvertToEncoded(const string_view& str) const
{
    string ret;
    if (!TryConvertToEncoded(str, ret))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "The provided string can't be converted to CID encoding");

    return ret;
}

bool PdfEncoding::TryConvertToEncoded(const string_view& str, string& encoded) const
{
    encoded.clear();
    if (str.empty())
        return true;

    auto& font = GetFont();
    if (font.IsLoaded())
    {
        // The font is loaded from object. We will attempt to use
        // just the loaded map to perform the conversion
        auto& map = GetToUnicodeMap();
        auto it = str.begin();
        auto end = str.end();
        while (it != end)
        {
            PdfCharCode code;
            if (!map.TryGetNextCharCode(it, end, code))
                return false;

            code.AppendTo(encoded);
        }

        return true;
    }
    else
    {
        // If the font is not loaded from object but created
        // from scratch, we will attempt first to infer GIDs
        // from Unicode code points using the font metrics
        auto& metrics = font.GetMetrics();
        auto it = str.begin();
        auto end = str.end();
        vector<unsigned> gids;
        vector<char32_t> cps;   // Code points
        while (it != end)
        {
            char32_t cp = utf8::next(it, end);
            unsigned gid;
            if (!metrics.TryGetGID(cp, gid))
                return false;

            cps.push_back(cp);
            gids.push_back(gid);
        }

        // Try to subsistute GIDs for fonts that support
        // a glyph substitution mechanism
        vector<unsigned char> backwardMap;
        metrics.SubstituteGIDs(gids, backwardMap);

        // Add used gid to the font mapping afferent code points,
        // and append the returned code unit to encoded string
        unsigned cpOffset = 0;
        for (unsigned i = 0; i < gids.size(); i++)
        {
            unsigned char cpsSpanSize = backwardMap[i];
            span<char32_t> span(cps.data() + cpOffset, cpsSpanSize);
            auto cid = font.AddUsedGID(gids[i], span);
            cid.Unit.AppendTo(encoded);
            cpOffset += cpsSpanSize;
        }

        return true;
    }
}

bool PdfEncoding::tryConvertEncodedToUtf8(const string_view& encoded, string& str) const
{
    str.clear();
    if (encoded.empty())
        return true;

    auto& map = GetToUnicodeMap();
    bool success = true;
    auto it = encoded.begin();
    auto end = encoded.end();
    vector<char32_t> codePoints;
    while (it != end)
    {
        if (!map.TryGetNextCodePoints(it, end, codePoints))
        {
            success = false;
            codePoints.clear();
            codePoints.push_back((char32_t)fetchFallbackCharCode(it, end, map.GetLimits()).Code);
        }

        for (size_t i = 0; i < codePoints.size(); i++)
        {
            char32_t codePoint = codePoints[i];
            if (codePoint != U'\0' && utf8::internal::is_code_point_valid(codePoint))
            {
                // Validate codepoints to insert
                utf8::unchecked::append((uint32_t)codePoints[i], std::back_inserter(str));
            }
        }
    }

    return success;
}

vector<PdfCID> PdfEncoding::ConvertToCIDs(const PdfString& encodedStr) const
{
    // Just ignore failures
    vector<PdfCID> cids;
    (void)tryConvertEncodedToCIDs(encodedStr.GetRawData(), cids);
    return cids;
}

bool PdfEncoding::TryConvertToCIDs(const PdfString& encodedStr, vector<PdfCID>& cids) const
{
    return tryConvertEncodedToCIDs(encodedStr.GetRawData(), cids);
}

PdfCID PdfEncoding::GetCID(char32_t codePoint) const
{
    // Just ignore failures
    PdfCID cid;
    (void)TryGetCID(codePoint, cid);
    return cid;
}

bool PdfEncoding::TryGetCID(char32_t codePoint, PdfCID& cid) const
{
    auto& toUnicode = GetToUnicodeMap();
    bool success = true;
    PdfCharCode codeUnit;
    if (!toUnicode.TryGetCharCode(codePoint, codeUnit))
    {
        success = false;
        cid = getFallbackCID(codePoint, *m_Encoding, toUnicode);
    }
    else
    {
        cid = getCID(codeUnit, *m_Encoding, success);
    }

    return success;
}

bool PdfEncoding::tryConvertEncodedToCIDs(const string_view& encoded, vector<PdfCID>& cids) const
{
    cids.clear();
    if (encoded.empty())
        return true;

    bool success = true;
    auto it = encoded.begin();
    auto end = encoded.end();
    PdfCID cid;
    while (it != end)
    {
        if (!m_Encoding->TryGetNextCID(it, end, cid))
        {
            success = false;
            PdfCharCode unit = fetchFallbackCharCode(it, end, m_Encoding->GetLimits());
            cid = PdfCID(unit);
        }

        cids.push_back(cid);
    }

    return success;
}

vector<PdfCID> PdfEncoding::ConvertToCIDs(const string_view& str) const
{
    // Just ignore failures
    vector<PdfCID> ret;
    (void)TryConvertToCIDs(str, ret);
    return ret;
}

bool PdfEncoding::TryConvertToCIDs(const string_view& str, vector<PdfCID>& cids) const
{
    cids.clear();
    if (str.empty())
        return true;

    auto& toUnicode = GetToUnicodeMap();
    auto it = str.begin();
    auto end = str.end();
    PdfCharCode codeUnit;
    bool success = true;
    while (it != end)
    {
        if (!toUnicode.TryGetNextCharCode(it, end, codeUnit))
        {
            success = false;
            char32_t codePoint = (char32_t)utf8::next(it, end);
            cids.push_back(getFallbackCID(codePoint, *m_Encoding, toUnicode));
        }
        else
        {
            cids.push_back(getCID(codeUnit, *m_Encoding, success));
        }
    }

    return success;
}

bool PdfEncoding::HasCIDMapping() const
{
    return m_Encoding->HasCIDMapping();
}

const PdfCharCode& PdfEncoding::GetFirstChar() const
{
    auto& limits = getActualLimits();
    if (limits.FirstChar.Code > limits.LastChar.Code)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "FirstChar shall be smaller than LastChar");

    return limits.FirstChar;
}

const PdfCharCode& PdfEncoding::GetLastChar() const
{
    auto& limits = getActualLimits();
    if (limits.FirstChar.Code > limits.LastChar.Code)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "FirstChar shall be smaller than LastChar");

    return limits.LastChar;
}

void PdfEncoding::ExportToDictionary(PdfDictionary& dictionary, PdfEncodingExportFlags flags) const
{
    if ((flags & PdfEncodingExportFlags::ExportCIDCMap) == PdfEncodingExportFlags::ExportCIDCMap)
    {
        auto& font = GetFont();
        auto& usedGids = font.GetUsedGIDs();
        auto cmapObj = dictionary.GetOwner()->GetDocument()->GetObjects().CreateDictionaryObject();
        if (getActualLimits().MaxCodeSize > 1)
            PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "TODO");
        fillCIDToGIDMap(*cmapObj, usedGids, font.GetBaseFont());
        dictionary.AddKeyIndirect("Encoding", cmapObj);
    }
    else
    {
        auto& objects = dictionary.GetOwner()->GetDocument()->GetObjects();
        PdfName name;
        PdfObject* obj;
        m_Encoding->GetExportObject(objects, name, obj);
        if (obj == nullptr)
            dictionary.AddKey("Encoding", name);
        else
            dictionary.AddKeyIndirect("Encoding", obj);
    }

    if ((flags & PdfEncodingExportFlags::SkipToUnicode) == PdfEncodingExportFlags::None)
    {
        auto& toUnicode = GetToUnicodeMap();
        auto cmapObj = dictionary.GetOwner()->GetDocument()->GetObjects().CreateDictionaryObject();
        toUnicode.WriteToUnicodeCMap(*cmapObj);
        dictionary.AddKeyIndirect("ToUnicode", cmapObj);
    }
}

bool PdfEncoding::IsNull() const
{
    return m_Id == NullEncodingId;
}

PdfFont& PdfEncoding::GetFont() const
{
    PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "The encoding has not been binded to a font");
}

char32_t PdfEncoding::GetCodePoint(const PdfCharCode& codeUnit) const
{
    auto& map = GetToUnicodeMap();
    vector<char32_t> codePoints;
    if (!map.TryGetCodePoints(codeUnit, codePoints)
        || codePoints.size() != 1)
    {
        return U'\0';
    }

    return codePoints[0];
}

char32_t PdfEncoding::GetCodePoint(unsigned charCode) const
{
    auto& map = GetToUnicodeMap();
    auto& limits = map.GetLimits();
    vector<char32_t> codePoints;
    for (unsigned char i = limits.MinCodeSize; i <= limits.MaxCodeSize; i++)
    {
        if (map.TryGetCodePoints({ charCode, i }, codePoints)
            && codePoints.size() == 1)
        {
            return codePoints[0];
        }
    }

    return U'\0';
}

const PdfEncodingLimits& PdfEncoding::getActualLimits() const
{
    if (m_Limits.FirstChar.Code > m_Limits.LastChar.Code)
        return m_Encoding->GetLimits();

    return m_Limits;
}

const PdfEncodingMap& PdfEncoding::GetToUnicodeMap() const
{
    if (m_ToUnicode == nullptr)
        return *m_Encoding;
    else
        return *m_ToUnicode;
}

// Handle missing mapped code by just appending current
// extracted raw character of minimum code size. Increment the
// iterator since failure on previous call doesn't do it. This
// is similar to what Adobe reader does for 1 byte encodings
// TODO: See also Pdf Reference 1.7 "Handling Undefined Characters"
// and try to implement all the fallbacks rules that applies here
// properly. Note: CID 0 fallback selection doesn't apply here.
// that is needed only when rendering the glyph
PdfCharCode fetchFallbackCharCode(string_view::iterator& it, const string_view::iterator& end, const PdfEncodingLimits& limits)
{
    unsigned code = (unsigned char)*it;
    unsigned char i = 1;
    PODOFO_ASSERT(i <= limits.MinCodeSize);
    while (true)
    {
        it++;
        if (it == end || i == limits.MinCodeSize)
            break;

        code = code << 8 | (unsigned char)*it;
        i++;
    }

    return { code, i };
}

// Handle missing mapped char code by just appending current
// extracted unicode code point on the minimum char code size.
// This is similar to what Adobe reader does for 1 byte encodings
PdfCharCode getFallbackCharCode(char32_t codePoint, const PdfEncodingLimits& limits)
{
    // Get che code size needed to store the value, clamping
    // on admissible values
    unsigned char codeSize = std::clamp(usr::GetCharCodeSize(codePoint),
        limits.MinCodeSize, limits.MaxCodeSize);
    // Clamp the value to valid range
    unsigned code = std::clamp((unsigned)codePoint, (unsigned)0, usr::GetCharCodeMaxValue((unsigned)codeSize));
    return { code, codeSize };
}

PdfCID getFallbackCID(char32_t codePoint, const PdfEncodingMap& toUnicode, const PdfEncodingMap& encoding)
{
    auto codeUnit = getFallbackCharCode(codePoint, toUnicode.GetLimits());
    bool success = false; // Ignore failure on fallback
    return getCID(codeUnit, encoding, success);
}

PdfCID getCID(const PdfCharCode& codeUnit, const PdfEncodingMap& map, bool& success)
{
    unsigned cidCode;
    if (!map.TryGetCIDId(codeUnit, cidCode))
    {
        // As a fallback, just push back the char code itself
        success = false;
        cidCode = codeUnit.Code;
    }

    return { cidCode, codeUnit };
}

void fillCIDToGIDMap(PdfObject& cmapObj, const UsedGIDsMap& usedGIDs, const string_view& baseFont)
{
    // CMap specification is in Adobe technical node #5014
    auto& cmapDict = cmapObj.GetDictionary();
    auto cmapName = string(baseFont).append("-subset");
    // Table 120: Additional entries in a CMap stream dictionary
    cmapDict.AddKey(PdfName::KeyType, PdfName("CMap"));
    cmapDict.AddKey("CMapName", PdfName(cmapName));
    PdfDictionary cidSystemInfo;
    // Setting the CIDSystemInfo params:
    cidSystemInfo.AddKey("Registry", PdfString(CMAP_REGISTRY_NAME));
    cidSystemInfo.AddKey("Ordering", PdfString(baseFont));
    cidSystemInfo.AddKey("Supplement", PdfObject(static_cast<int64_t>(0)));
    cmapDict.AddKey("CIDSystemInfo", cidSystemInfo);

    auto& stream = cmapObj.GetOrCreateStream();
    stream.BeginAppend();
    stream.Append(
        "/CIDInit /ProcSet findresource begin\n"
        "12 dict begin\n"
        "begincmap\n"
        "/CIDSystemInfo <<\n"
        "   /Registry (" CMAP_REGISTRY_NAME  ")\n"
        "   /Ordering (").Append(baseFont).Append(")\n"
        "   /Supplement 0\n"
        ">> def\n"
        "/CMapName /").Append(cmapName).Append(" def\n"
        "/CMapType 1 def\n"     // As defined in Adobe Technical Notes #5099
        "1 begincodespacerange\n"
        "<00> <FF>\n"
        "endcodespacerange\n");

    stream.Append(std::to_string(usedGIDs.size())).Append(" begincidchar\n");
    string code;
    for (auto& pair : usedGIDs)
    {
        auto& cid = pair.second;
        cid.Unit.WriteHexTo(code);
        stream.Append(code).Append(" ").Append(std::to_string(cid.Id)).Append("\n");;
    }
    stream.Append(
        "endcidchar\n"
        "endcmap\n"
        "CMapName currentdict / CMap defineresource pop\n"
        "end\n"
        "end");
    stream.EndAppend();
}

size_t getNextId()
{
    return s_nextid++;
}
