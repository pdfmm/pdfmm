#ifndef PDFMM_FREETYPE_PRIVATE_H
#define PDFMM_FREETYPE_PRIVATE_H

// Old freetype versions requires <ft2build.h> to be included first
#include <ft2build.h>
#include FT_FREETYPE_H

#include <pdfmm/base/basedefs.h>

namespace mm
{
    FT_Library PDFMM_API GetFreeTypeLibrary();
}

#endif // PDFMM_FREETYPE_PRIVATE_H
