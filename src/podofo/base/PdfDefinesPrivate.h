#ifndef _PDF_DEFINES_PRIVATE_H_
#define _PDF_DEFINES_PRIVATE_H_

#ifndef BUILDING_PODOFO
#error PdfDefinesPrivate.h is only available for use in the core PoDoFo src/ build .cpp files
#endif

#include "PdfDefines.h"

/** \def VERBOSE_DEBUG_DISABLED
 *  Debug define. Enable it, if you need
 *  more debuf output to the commandline from PoDoFo
 *
 *  Setting VERBOSE_DEBUG_DISABLED will make PoDoFo
 *  EXTREMELY slow and verbose, so it's not practical
 *  even for regular debuggin.
 */
#define VERBOSE_DEBUG_DISABLED

 // Should we do lots of extra (expensive) sanity checking?  You should not
 // define this on production builds because of the runtime cost and because it
 // might cause the library to abort() if it notices something nasty.
 // It may also change the size of some objects, and is thus not binary
 // compatible.
 //
 // If you don't know you need this, avoid it.
 //
#define EXTRA_CHECKS_DISABLED

#ifdef DEBUG
#include <cassert>
#define PODOFO_ASSERT( x ) assert( x );
#else
#define PODOFO_ASSERT( x ) do { if (!(x)) PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, #x); } while (false)
#endif // DEBUG

#include "PdfCompilerCompatPrivate.h"

namespace io
{
    size_t FileSize(const std::string_view& filename);
    size_t Read(std::istream& stream, char* buffer, size_t count);

    std::ifstream open_ifstream(const std::string_view& filename, std::ios_base::openmode mode);
    std::ofstream open_ofstream(const std::string_view& filename, std::ios_base::openmode mode);
    std::fstream open_fstream(const std::string_view& filename, std::ios_base::openmode mode);

    // NOTE: Never use this function unless you really need a C FILE descriptor,
    // as in PdfImage.cpp . For all the other I/O, use an STL stream
    FILE* fopen(const std::string_view& view, const std::string_view& mode);
}

/**
 * \page <PoDoFo PdfDefinesPrivate Header>
 *
 * <b>PdfDefinesPrivate.h</b> contains preprocessor definitions, inline functions, templates, 
 * compile-time const variables, and other things that must be visible across the entirety of
 * the PoDoFo library code base but should not be visible to users of the library's headers.
 *
 * This header is private to the library build. It is not installed with PoDoFo and must not be
 * referenced in any way from any public, installed header.
 */

#endif
