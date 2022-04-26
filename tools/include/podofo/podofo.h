#pragma once

// REMOVE-ME: This is a shim to allow tools to compile with
// PoDoFo namespace/defines. Remove it whend and it pdfmm
// has been merged back to PoDoFo

#include <pdfmm/pdfmm.h>

namespace PoDoFo
{
    using namespace mm;
}

#define PODOFO_VERSION_STRING PDFMM_VERSION_STRING
