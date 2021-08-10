/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontType1Encoding.h"
#include "PdfInputDevice.h"
#include "PdfPostScriptTokenizer.h"
#include "PdfObject.h"
#include "PdfDifferenceEncoding.h"

using namespace std;
using namespace mm;

PdfFontType1Encoding::PdfFontType1Encoding(const PdfObject& obj) :
    PdfEncodingMapBase(getUnicodeMap(obj)) { }

PdfCharCodeMap PdfFontType1Encoding::getUnicodeMap(const PdfObject& obj)
{
    PdfSharedBuffer buffer;
    PdfBufferOutputStream outputStream(buffer);
    obj.GetStream().GetFilteredCopy(outputStream);

    string_view view(buffer.GetBuffer(), buffer.GetSize());
    // Try to find binary part of the document and exclude it
    auto found = view.find("eexec");
    if (found != string_view::npos)
        view = view.substr(0, found + 5);

    PdfInputDevice device(view.data(), view.length());
    PdfPostScriptTokenizer tokenizer;
    PdfPostScriptTokenType tokenType;
    string_view keyword;
    PdfVariant variant;
    PdfName name;
    int64_t number;
    PdfCharCodeMap ret;

    auto tryReadEntry = [&]()
    {
        while (true)
        {
            // Consume al tokens searching for "dup" keyword
            if (!tokenizer.TryReadNext(device, tokenType, keyword, variant))
                return false;

            if (tokenType == PdfPostScriptTokenType::Keyword
                && keyword == "dup")
            {
                break;
            }
        }

        // Try to read CID
        if (!tokenizer.TryReadNext(device, tokenType, keyword, variant)
            || tokenType != PdfPostScriptTokenType::Variant
            || !variant.TryGetNumber(number))
        {
            return false;
        }

        // Try to read character code
        if (!tokenizer.TryReadNext(device, tokenType, keyword, variant)
            || tokenType != PdfPostScriptTokenType::Variant
            || !variant.TryGetName(name))
        {
            return false;
        }

        unsigned code = (unsigned)number;
        char32_t cp = PdfDifferenceEncoding::NameToUnicodeID(name);
        if (cp != U'\0')
        {
            unsigned char codeSize = usr::GetCharCodeSize(code);
            ret.PushMapping({ code, codeSize }, cp);
        }

        return true;
    };

    while (true)
    {
        // Consume all tokens searching for the /Encoding array
        if (!tokenizer.TryReadNext(device, tokenType, keyword, variant))
            return ret;

        if (tokenType == PdfPostScriptTokenType::Variant
            && variant.TryGetName(name)
            && name.GetString() == "Encoding")
        {
            break;
        }
    }

    while (true)
    {
        // Read all the entries
        if (!tryReadEntry())
            return ret;
    }

    return ret;
}

void PdfFontType1Encoding::getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    (void)objects;
    (void)name;
    (void)obj;
    // Do nothing. PdfFontType1Encoding encoding is implicit in the font program
}
