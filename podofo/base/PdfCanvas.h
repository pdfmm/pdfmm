
#ifndef PODOFO_WRAPPER_PDFCANVASH
#define PODOFO_WRAPPER_PDFCANVASH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfCanvas.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/PdfCanvas.h"
#endif
