/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetrics.h"
#include <pdfmm/private/FreetypePrivate.h>

#include "PdfIdentityEncoding.h"

using namespace std;
using namespace mm;

/** A built-in encoding for a /Type1 font program
 */
class PDFMM_API PdfFontBuiltinType1Encoding final : public PdfEncodingMapBase
{
public:
    PdfFontBuiltinType1Encoding(PdfCharCodeMap && map)
        : PdfEncodingMapBase(std::move(map), PdfEncodingMapType::Simple) { }

public:
    bool IsBuiltinEncoding() const override { return true; }

protected:
    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override
    {
        (void)objects;
        (void)name;
        (void)obj;
        // Do nothing. encoding is implicit in the font program
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

    return PdfEncodingMapConstPtr(new PdfFontBuiltinType1Encoding(std::move(codeMap)));
}


void PdfFontMetrics::tryLoadCIDToGIDMap()
{
    if (m_CIDToGIDMapLoaded)
        return;

    FT_Face face;
    if (TryGetOrLoadFace(face) && face->num_charmaps != 0)
    {
        CIDToGIDMap map;

        // ISO 32000-1:2008 9.6.6.4 "Encodings for TrueType Fonts"
        // "A TrueType font program’s built-in encoding maps directly
        // from character codes to glyph descriptions by means of an
        // internal data structure called a 'cmap' "
        FT_Error rc;
        FT_ULong code;
        FT_UInt index;

        rc = FT_Set_Charmap(face, face->charmaps[0]);
        CHECK_FT_RC(rc, FT_Set_Charmap);

        if (face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
        {
            code = FT_Get_First_Char(face, &index);
            while (index != 0)
            {
                // "If the font contains a (3, 0) subtable, the range of character
                // codes shall be one of these: 0x0000 - 0x00FF,
                // 0xF000 - 0xF0FF, 0xF100 - 0xF1FF, or 0xF200 - 0xF2FF"
                // NOTE: we just take the first byte
                map.insert({ (unsigned)(code & 0xFF), index });
                code = FT_Get_Next_Char(face, code, &index);
            }
        }
        else
        {
            code = FT_Get_First_Char(face, &index);
            while (index != 0)
            {
                map.insert({ (unsigned)code, index });
                code = FT_Get_Next_Char(face, code, &index);
            }
        }

        m_CIDToGIDMap.reset(new PdfCIDToGIDMap(std::move(map), PdfGlyphAccess::FontProgram));
    }

    m_CIDToGIDMapLoaded = true;
}
