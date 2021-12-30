/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

#include <pdfmm/private/FreetypePrivate.h>
#include <fontconfig/fontconfig.h>

using namespace std;
using namespace mm;

#ifdef PDFMM_HAVE_FONTCONFIG

static bool getFontInfo(FcPattern* font, string& fontFamily, string& fontPath,
    bool& bold, bool& italic);
static void testSingleFont(FcPattern* font);

TEST_CASE("testFonts")
{
    FcObjectSet* objectSet = nullptr;
    FcFontSet* fontSet = nullptr;
    FcPattern* pattern = nullptr;

    // Get all installed fonts
    pattern = FcPatternCreate();
    objectSet = FcObjectSetBuild(FC_FAMILY, FC_STYLE, FC_FILE, FC_SLANT, FC_WEIGHT, nullptr);
    fontSet = FcFontList(nullptr, pattern, objectSet);

    FcObjectSetDestroy(objectSet);
    FcPatternDestroy(pattern);

    if (fontSet)
    {
        INFO(cmn::Format("Testing {} fonts", fontSet->nfont));
        for (int i = 0; i < fontSet->nfont; i++)
            testSingleFont(fontSet->fonts[i]);

        FcFontSetDestroy(fontSet);
    }

    // Shut fontconfig down
    // Causes an assertion in fontconfig FcFini();
}

TEST_CASE("testCreateFontFtFace")
{
    FT_Face face;
    FT_Error error;
    PdfMemDocument doc;

    // TODO: Find font file on disc!
    error = FT_New_Face(mm::GetFreeTypeLibrary(), "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf", 0, &face);

    if (error == 0)
    {
        PdfFont* font = doc.GetFontManager().GetFont(face);

        INFO("Cannot create font from FT_Face");
        REQUIRE(font != nullptr);
    }
}

void testSingleFont(FcPattern* font)
{
    PdfMemDocument doc;
    string fontFamily;
    string fontPath;
    bool bold;
    bool italic;
    auto& fcWrapper = PdfFontManager::GetFontConfigWrapper();

    if (getFontInfo(font, fontFamily, fontPath, bold, italic))
    {
        string fontPath = fcWrapper.GetFontConfigFontPath(fontFamily, bold, italic);
        if (fontPath.length() != 0)
        {
            PdfFontCreationParams params;
            params.SearchParams.Bold = bold;
            params.SearchParams.Italic = italic;
            auto font = doc.GetFontManager().GetFont(fontFamily, params);
            INFO(cmn::Format("Font failed: {}", fontPath));
            REQUIRE(font != nullptr);
        }
    }
}

bool getFontInfo(FcPattern* pFont, string& fontFamily, string& fontPath,
    bool& bold, bool& italic)
{
    FcChar8* family = nullptr;
    FcChar8* path = nullptr;
    int slant;
    int weight;

    if (FcPatternGetString(pFont, FC_FAMILY, 0, &family) == FcResultMatch)
    {
        fontFamily = reinterpret_cast<char*>(family);
        if (FcPatternGetString(pFont, FC_FILE, 0, &path) == FcResultMatch)
        {
            fontPath = reinterpret_cast<char*>(path);

            if (FcPatternGetInteger(pFont, FC_SLANT, 0, &slant) == FcResultMatch)
            {
                if (slant == FC_SLANT_ROMAN)
                    italic = false;
                else if (slant == FC_SLANT_ITALIC)
                    italic = true;
                else
                    return false;

                if (FcPatternGetInteger(pFont, FC_WEIGHT, 0, &weight) == FcResultMatch)
                {
                    if (weight == FC_WEIGHT_MEDIUM)
                        bold = false;
                    else if (weight == FC_WEIGHT_BOLD)
                        bold = true;
                    else
                        return false;

                    return true;
                }
            }
            //free( file );
        }
        //free( family );
    }

    return false;
}

#endif // PDFMM_HAVE_FONTCONFIG
