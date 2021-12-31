/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfLocale.h"
#include "PdfError.h"

#include <sstream>

using namespace std;
using namespace mm;

void mm::PdfLocaleImbue(ios_base& s)
{
    static const locale cachedLocale(PdfIOLocale);
    try
    {
        s.imbue(cachedLocale);
    }
    catch (const runtime_error& e)
    {
        PDFMM_RAISE_ERROR_INFO(
            PdfErrorCode::InvalidDeviceOperation,
            "Failed to set safe locale on stream being used for PDF I/O. "
            "Locale set was: \"{}\". Error reported by STL std::locale: \"{}\"",
            PdfIOLocale, e.what()
        );
    }
}
