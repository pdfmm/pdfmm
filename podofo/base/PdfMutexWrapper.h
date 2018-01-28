
#ifndef PODOFO_WRAPPER_PDFMUTEXWRAPPERH
#define PODOFO_WRAPPER_PDFMUTEXWRAPPERH
/*
 * This is a simple wrapper include file that lets you include
 * <podofo/base/PdfMutexWrapper.h> when building against a podofo build directory
 * rather than an installed copy of podofo. You'll probably need
 * this if you're including your own (probably static) copy of podofo
 * using a mechanism like svn:externals .
 */
#include "../../src/base/util/PdfMutexWrapper.h"
#endif
