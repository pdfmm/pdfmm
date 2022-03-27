/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetrics.h"
#include <pdfmm/private/FreetypePrivate.h>

using namespace std;
using namespace mm;

/** A built-in encoding for a /Type1 font program
 */
class PDFMM_API PdfFontType1Encoding final : public PdfEncodingMapBase
{
public:
    PdfFontType1Encoding(PdfCharCodeMap && map)
        : PdfEncodingMapBase(std::move(map)) { }

public:
    PdfEncodingMapType GetType() const override { return PdfEncodingMapType::Simple; }

protected:
    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override
    {
        (void)objects;
        (void)name;
        (void)obj;
        // Do nothing. PdfFontType1Encoding encoding is implicit in the font program
    }
};

PdfEncodingMapConstPtr PdfFontMetrics::getFontType1Encoding(FT_Face face)
{
    PdfCharCodeMap codeMap;
    FT_Error rc;
    FT_ULong code;
    FT_UInt index;

    auto oldCharmap = face->charmap;

    map<FT_UInt, FT_ULong> unicodeMap;
    rc = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    CHECK_FT_RC(rc, FT_Select_Charmap);

    code = FT_Get_First_Char(face, &index);
    while (index != 0)
    {
        unicodeMap[index] = code;
        code = FT_Get_Next_Char(face, code, &index);
    }

    // Search for a custom char map
    map<FT_UInt, FT_ULong> customMap;
    rc = FT_Select_Charmap(face, FT_ENCODING_ADOBE_CUSTOM);
    if (rc == 0)
    {
        code = FT_Get_First_Char(face, &index);
        while (index != 0)
        {
            customMap[index] = code;
            code = FT_Get_Next_Char(face, code, &index);
        }

        rc = FT_Set_Charmap(face, oldCharmap);
        CHECK_FT_RC(rc, FT_Select_Charmap);

        for (auto& pair : customMap)
        {
            auto found = unicodeMap.find(pair.first);
            if (found == unicodeMap.end())
            {
                // Some symbol characters may have no unicode representation
                codeMap.PushMapping(PdfCharCode((unsigned)pair.second), U'\0');
                continue;
            }

            codeMap.PushMapping(PdfCharCode((unsigned)pair.second), (char32_t)found->second);
        }
    }
    else
    {
        // NOTE: Some very strange CCF fonts just supply an unicode map
        // For these, we just assume code identity with Unicode codepoint
        for (auto& pair : unicodeMap)
            codeMap.PushMapping(PdfCharCode((unsigned)pair.second), (char32_t)pair.second);
    }

    return PdfEncodingMapConstPtr(new PdfFontType1Encoding(std::move(codeMap)));
}
