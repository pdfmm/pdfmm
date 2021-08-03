/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfCMapEncoding.h"

#include <utfcpp/utf8.h>

#include "PdfDictionary.h"
#include "PdfStream.h"
#include "PdfPostScriptTokenizer.h"
#include "PdfArray.h"

using namespace std;
using namespace PoDoFo;

static void readNextVariantSequence(PdfPostScriptTokenizer& tokenizer, PdfInputDevice& device,
    PdfVariant& variant, const string_view& endSequenceKeyword, bool& endOfSequence);
static uint32_t getCodeFromVariant(const PdfVariant& var, PdfEncodingLimits& limits);
static uint32_t getCodeFromVariant(const PdfVariant& var, PdfEncodingLimits& limits, unsigned char& codeSize);
static vector<char32_t> handleNameMapping(const PdfName& name);
static vector<char32_t> handleStringMapping(const PdfString& str);
static void handleRangeMapping(PdfCharCodeMap& map, PdfEncodingLimits& limits,
    uint32_t srcCodeLo, const vector<char32_t>& dstCodeLo,
    unsigned char codeSize, unsigned rangeSize);
static vector<char32_t> handleUtf8String(const string& str);
static void pushMapping(PdfCharCodeMap& map, PdfEncodingLimits& limits,
    const PdfCharCode& codeUnit,
    const std::vector<char32_t>& codePoints);

PdfCMapEncoding::PdfCMapEncoding(const PdfObject& cmapObj)
    : PdfCMapEncoding(parseCMapObject(cmapObj.GetStream()))
{
}

PdfCMapEncoding::PdfCMapEncoding(MapIdentity map)
    : PdfEncodingMapBase(std::move(map.Map), map.Limits)
{
}

bool PdfCMapEncoding::HasCIDMapping() const
{
    // CMap encodings can represent proper CID encoding
    return true;
}

bool PdfCMapEncoding::HasLigaturesSupport() const
{
    // CMap encodings may have ligatures
    return true;
}

PdfCMapEncoding::MapIdentity PdfCMapEncoding::parseCMapObject(const PdfStream& stream)
{
    MapIdentity ret;
    unique_ptr<char> streamBuffer;
    size_t streamBufferLen;
    stream.GetFilteredCopy(streamBuffer, streamBufferLen);

    PdfInputDevice device(streamBuffer.get(), streamBufferLen);
    PdfPostScriptTokenizer tokenizer;
    deque<unique_ptr<PdfVariant>> tokens;
    PdfString str;
    auto var = make_unique<PdfVariant>();
    EPdfPostScriptTokenType tokenType;
    string_view token;
    bool endOfSequence;
    while (tokenizer.TryReadNext(device, tokenType, token, *var))
    {
        switch (tokenType)
        {
            case EPdfPostScriptTokenType::Keyword:
            {
                if (token == "begincodespacerange")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endcodespacerange", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t lowerBound = getCodeFromVariant(*var, ret.Limits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        uint32_t upperBound = getCodeFromVariant(*var, ret.Limits, codeSize);
                    }
                }
                // NOTE: "bf" in "beginbfrange" stands for Base Font
                // see Adobe tecnichal notes #5014
                else if (token == "beginbfrange")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endbfrange", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t srcCodeLo = getCodeFromVariant(*var, ret.Limits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        uint32_t srcCodeHi = getCodeFromVariant(*var, ret.Limits);
                        unsigned rangeSize = srcCodeHi - srcCodeLo + 1;
                        tokenizer.ReadNextVariant(device, *var);
                        if (var->IsArray())
                        {
                            PdfArray& arr = var->GetArray();
                            for (unsigned i = 0; i < rangeSize; i++)
                            {
                                auto& dst = arr[i];
                                if (dst.TryGetString(str) && str.IsHex()) // pp. 475 PdfReference 1.7
                                    pushMapping(ret.Map, ret.Limits, { srcCodeLo + i, codeSize }, handleStringMapping(dst.GetString()));
                                else if (dst.IsName()) // Not mentioned in tecnincal document #5014 but seems safe
                                    pushMapping(ret.Map, ret.Limits, { srcCodeLo + i, codeSize }, handleNameMapping(dst.GetName()));
                                else
                                    PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "beginbfrange: expected string or name inside array");
                            }
                        }
                        else if (var->TryGetString(str) && str.IsHex())
                        {
                            // pp. 474 PdfReference 1.7
                            auto dstCodeLo = handleStringMapping(var->GetString());
                            handleRangeMapping(ret.Map, ret.Limits, srcCodeLo, dstCodeLo, codeSize, rangeSize);
                        }
                        else if (var->IsName())
                        {
                            // As found in tecnincal document #5014
                            handleRangeMapping(ret.Map, ret.Limits, srcCodeLo, handleNameMapping(var->GetName()), codeSize, rangeSize);
                        }
                        else
                            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "beginbfrange: expected array, string or array");
                    }
                }
                // NOTE: "bf" in "beginbfchar" stands for Base Font
                // see Adobe tecnichal notes #5014
                else if (token == "beginbfchar")
                {
                    vector<char32_t> mappedCodes;
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endbfchar", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t srcCode = getCodeFromVariant(*var, ret.Limits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        if (var->IsNumber())
                        {
                            char32_t dstCode = (char32_t)getCodeFromVariant(*var, ret.Limits);
                            mappedCodes.clear();
                            mappedCodes.push_back(dstCode);
                        }
                        else if (var->TryGetString(str) && str.IsHex())
                        {
                            // pp. 474 PdfReference 1.7
                            mappedCodes = handleStringMapping(var->GetString());
                        }
                        else if (var->IsName())
                        {
                            // As found in tecnincal document #5014
                            mappedCodes = handleNameMapping(var->GetName());
                        }
                        else
                            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "beginbfchar: expected number or name");

                        pushMapping(ret.Map, ret.Limits, { srcCode, codeSize }, mappedCodes);
                    }
                }
                else if (token == "begincidrange")
                {
                    while (true)
                    {
                        readNextVariantSequence(tokenizer, device, *var, "endcidrange", endOfSequence);
                        if (endOfSequence)
                            break;

                        unsigned char codeSize;
                        uint32_t srcCodeLo = getCodeFromVariant(*var, ret.Limits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        uint32_t srcCodeHi = getCodeFromVariant(*var, ret.Limits);
                        tokenizer.ReadNextVariant(device, *var);
                        char32_t dstCIDLo = (char32_t)getCodeFromVariant(*var, ret.Limits);

                        unsigned rangeSize = srcCodeHi - srcCodeLo + 1;
                        vector<char32_t> mappedCodes;
                        for (unsigned i = 0; i < rangeSize; i++)
                        {
                            char32_t newbackchar = dstCIDLo + i;
                            mappedCodes.clear();
                            mappedCodes.push_back(newbackchar);
                            pushMapping(ret.Map, ret.Limits, { srcCodeLo + i, codeSize }, mappedCodes);
                        }
                    }
                }
                else if (token == "begincidchar")
                {
                    if (tokens.size() != 1)
                        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidStream, "CMap missing object number before begincidchar");

                    int charCount = (int)tokens.front()->GetNumber();
                    vector<char32_t> mappedCodes;
                    for (int i = 0; i < charCount; i++)
                    {
                        tokenizer.TryReadNext(device, tokenType, token, *var);
                        unsigned char codeSize;
                        uint32_t srcCode = getCodeFromVariant(*var, ret.Limits, codeSize);
                        tokenizer.ReadNextVariant(device, *var);
                        char32_t dstCode = (char32_t)getCodeFromVariant(*var, ret.Limits);
                        mappedCodes.clear();
                        mappedCodes.push_back(dstCode);
                        pushMapping(ret.Map, ret.Limits, { srcCode, codeSize }, mappedCodes);
                    }
                }

                tokens.clear();
                break;
            }
            case EPdfPostScriptTokenType::Variant:
            {
                tokens.push_front(std::move(var));
                var.reset(new PdfVariant());
                break;
            }
            default:
                PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
        }
    }

    return ret;
}

// Base Font 3 type CMap interprets strings as found in
// beginbfchar and beginbfrange as UTF-16BE, see PdfReference 1.7
// page 472. NOTE: Before UTF-16BE there was UCS-2 but UTF-16
// is backward compatible with UCS-2
vector<char32_t> handleStringMapping(const PdfString& str)
{
    auto& rawdata = str.GetRawData();
    string utf8;
    utf8::utf16to8(utf8::endianess::big_endian,
        (char16_t*)rawdata.data(), (char16_t*)(rawdata.data() + rawdata.size()),
        std::back_inserter(utf8));

    return handleUtf8String(utf8);
}

// Handle a range in begingbfrage "srcCodeLo srcCodeHi dstCodeLo" clause
void handleRangeMapping(PdfCharCodeMap& map, PdfEncodingLimits& limits,
    uint32_t srcCodeLo, const vector<char32_t>& dstCodeLo,
    unsigned char codeSize, unsigned rangeSize)
{
    auto it = dstCodeLo.begin();
    auto end = dstCodeLo.end();
    PODOFO_INVARIANT(it != end);
    char32_t back = dstCodeLo.back();
    vector<char32_t> newdstbase;
    // Compute a destination string that has all code points except the last one
    if (dstCodeLo.size() != 1)
    {
        do
        {
            newdstbase.push_back(*it);
            it++;
        } while (it != end);
    }

    // Compute new destination string with last chracter/code point incremented by one
    vector<char32_t> newdst;
    for (unsigned i = 0; i < rangeSize; i++)
    {
        newdst = newdstbase;
        char32_t newbackchar = back + i;
        newdst.push_back(newbackchar);
        pushMapping(map, limits, { srcCodeLo + i, codeSize }, newdst);
    }
}

// codeSize is the number of the octets in the string or the minimum number
// of bytes to represent the humber, example <cd> -> 1, <00cd> -> 2
static uint32_t getCodeFromVariant(const PdfVariant& var, unsigned char& codeSize)
{
    if (var.IsNumber())
    {
        int64_t temp = var.GetNumber();
        uint32_t ret = (uint32_t)temp;
        codeSize = 0;
        while (temp != 0)
        {
            temp >>= 8;
            codeSize++;
        }

        if (codeSize > 2)
            PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "PdfEncoding: unsupported code bigger than 16 bits");

        return ret;
    }

    const PdfString& str = var.GetString();
    uint32_t ret = 0;
    auto& rawstr = str.GetRawData();
    unsigned len = (unsigned)rawstr.length();
    for (unsigned i = 0; i < len; i++)
    {
        uint8_t code = (uint8_t)rawstr[len - 1 - i];
        ret += code << i * 8;
    }

    codeSize = len;
    return ret;
}

void pushMapping(PdfCharCodeMap& map, PdfEncodingLimits& limits, const PdfCharCode& codeUnit, const vector<char32_t>& codePoints)
{
    if (codePoints.size() == 0)
        return;

    map.PushMapping(codeUnit, codePoints);
    if (codeUnit.CodeSpaceSize < limits.MinCodeSize)
        limits.MinCodeSize = codeUnit.CodeSpaceSize;
    if (codeUnit.CodeSpaceSize > limits.MaxCodeSize)
        limits.MaxCodeSize = codeUnit.CodeSpaceSize;
    if (codeUnit.Code < limits.FirstChar.Code)
        limits.FirstChar = codeUnit;
    if (codeUnit.Code > limits.LastChar.Code)
        limits.LastChar = codeUnit;
}

uint32_t getCodeFromVariant(const PdfVariant& var, PdfEncodingLimits& limits, unsigned char& codeSize)
{
    uint32_t ret = getCodeFromVariant(var, codeSize);
    if (codeSize < limits.MinCodeSize)
        limits.MinCodeSize = codeSize;
    if (codeSize > limits.MaxCodeSize)
        limits.MaxCodeSize = codeSize;

    return ret;
}

uint32_t getCodeFromVariant(const PdfVariant& var, PdfEncodingLimits& limits)
{
    unsigned char codeSize;
    return getCodeFromVariant(var, limits, codeSize);
}

vector<char32_t> handleNameMapping(const PdfName& name)
{
    return handleUtf8String(name.GetString());
}

vector<char32_t> handleUtf8String(const string& str)
{
    vector<char32_t> ret;
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
        ret.push_back(utf8::next(it, end));

    return ret;
}

// Read variant from a sequence, unless it's the end of it
// We found Pdf(s) that have mistmatching sequence length and
// end of sequence marker, and Acrobat preflight treats them as valid,
// so we must determine end of sequnce only on the end of
// sequence keyword
void readNextVariantSequence(PdfPostScriptTokenizer& tokenizer, PdfInputDevice& device,
    PdfVariant& variant, const string_view& endSequenceKeyword, bool& endOfSequence)
{
    EPdfPostScriptTokenType tokenType;
    string_view token;

    if (!tokenizer.TryReadNext(device, tokenType, token, variant))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidStream, "CMap unable to read a token");

    switch (tokenType)
    {
        case EPdfPostScriptTokenType::Keyword:
        {
            if (token == endSequenceKeyword)
            {
                endOfSequence = true;
                break;
            }

            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidStream, string("CMap unable to read an end of sequence keyword ") + (string)endSequenceKeyword);
        }
        case EPdfPostScriptTokenType::Variant:
        {
            endOfSequence = false;
            break;
        }
        default:
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEnumValue, "Unexpected token type");
        }
    }
}
