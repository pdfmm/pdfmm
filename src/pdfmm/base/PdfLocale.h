/*/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDFMM_PDFLOCALE_H
#define PDFMM_PDFLOCALE_H

#include <ios>

namespace mm {

/**
 * The locale to use for PDF I/O . See mm::PdfLocaleImbue() .
 */
static const char PdfIOLocale[] = "C";

/**
 * Imbue the passed stream with a locale that will be safe to do
 * I/O of the low level PDF format with.
 *
 * PDF document structure I/O is done with the C++ standard library
 * IOStreams code. By default, this will adapt to the current locale.
 * That's not good at all when doing I/O of PDF data structures, which
 * follow POSIX/English locale conventions irrespective of runtime locale.
 * Make sure to to call this function on any stream you intend to use for
 * PDF I/O. Avoid using this stream for anything that should be done in the
 * regional locale.
 *
 * \warning This method may throw EPdfError::InvalidDeviceOperation
 *          if your STL does not support the locale string in PdfIOLocale .
 *
 * If you fail to call this on a stream you use for PDF I/O you will encounter
 * problems like German and other European users getting numbers in the format
 * "10110,4" or even "10.110,4" instead of "10110.4" .
 */
void PDFMM_API PdfLocaleImbue(std::ios_base&);

};

#endif
