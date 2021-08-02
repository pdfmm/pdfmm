/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfLocale.h"
#include "PdfError.h"

#include <iostream>
#include <sstream>

using namespace PoDoFo;

void PoDoFo::PdfLocaleImbue(std::ios_base& s)
{
    static const std::locale cachedLocale( PdfIOLocale );
    try
    {
    	s.imbue( cachedLocale );
    }
    catch (const std::runtime_error & e)
    {
        std::ostringstream err;
        err << "Failed to set safe locale on stream being used for PDF I/O.";
        err << "Locale set was: \"" << PdfIOLocale << "\".";
        err << "Error reported by STL std::locale: \"" << e.what() << "\"";
        // The info string is copied by PdfError so we're ok to just:
        PODOFO_RAISE_ERROR_INFO(
            EPdfError::InvalidDeviceOperation,
            err.str().c_str()
            );
    }
}
