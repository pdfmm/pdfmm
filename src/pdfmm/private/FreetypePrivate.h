#ifndef PDFMM_FREETYPE_PRIVATE_H
#define PDFMM_FREETYPE_PRIVATE_H

// Old freetype versions requires <ft2build.h> to be included first
#include <ft2build.h>
#include FT_FREETYPE_H

#include <pdfmm/base/basedefs.h>

#define CHECK_FT_RC(rc, func) if (rc != 0)\
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FreeType, "Function " #func " failed")

namespace mm
{
    FT_Library GetFreeTypeLibrary();
    bool TryCreateFreeTypeFace(const bufferview& view, FT_Face& face);
    FT_Face CreateFreeTypeFace(const bufferview& view);
    charbuff GetDataFromFace(FT_Face face);
}

// Other legacy TrueType tables defined in Apple documentation
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html
#define TTAG_acnt FT_MAKE_TAG('a', 'c', 'n', 't')
#define TTAG_ankr FT_MAKE_TAG('a', 'n', 'k', 'r')
#define TTAG_kerx FT_MAKE_TAG('k', 'e', 'r', 'x')
#define TTAG_fdsc FT_MAKE_TAG('f', 'd', 's', 'c')
#define TTAG_fmtx FT_MAKE_TAG('f', 'm', 't', 'x')
#define TTAG_fond FT_MAKE_TAG('f', 'o', 'n', 'd')
#define TTAG_gcid FT_MAKE_TAG('g', 'c', 'i', 'd')
#define TTAG_ltag FT_MAKE_TAG('l', 't', 'a', 'g')
#define TTAG_meta FT_MAKE_TAG('m', 'e', 't', 'a')
#define TTAG_xref FT_MAKE_TAG('x', 'r', 'e', 'f')
#define TTAG_Zapf FT_MAKE_TAG('Z', 'a', 'p', 'f')

#endif // PDFMM_FREETYPE_PRIVATE_H
