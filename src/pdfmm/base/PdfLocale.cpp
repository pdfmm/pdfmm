/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfLocale.h"
#include "PdfError.h"

#include <iostream>
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
        ostringstream err;
        err << "Failed to set safe locale on stream being used for PDF I/O.";
        err << "Locale set was: \"" << PdfIOLocale << "\".";
        err << "Error reported by STL std::locale: \"" << e.what() << "\"";
        // The info string is copied by PdfError so we're ok to just:
        PDFMM_RAISE_ERROR_INFO(
            EPdfError::InvalidDeviceOperation,
            err.str().c_str()
        );
    }
}
