/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DEFINES_PRIVATE_H
#define PDF_DEFINES_PRIVATE_H

#ifndef BUILDING_PDFMM
#error PdfDefinesPrivate.h is only available for use in the core pdfmm src/ build .cpp files
#endif

#include "PdfCompilerCompat.h"
#include <pdfmm/base/PdfDefines.h>

#include <stdexcept>
#include <limits>

/** \def VERBOSE_DEBUG_DISABLED
 *  Debug define. Enable it, if you need
 *  more debug output to the command line from pdfmm
 *
 *  Setting VERBOSE_DEBUG_DISABLED will make pdfmm
 *  EXTREMELY slow and verbose, so it's not practical
 *  even for regular debugging.
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
#define PDFMM_ASSERT(x) assert(x);
#else
#define PDFMM_ASSERT(x)
#endif // DEBUG

// This is a do nothing macro that can be used to define
// an invariant property without actually checking for it,
// not even in DEBUG build. It's user responsability to
// ensure it's actually satisfied
#define PDFMM_INVARIANT(x)

#define CMAP_REGISTRY_NAME "pdfmm"

namespace usr
{
    // Write the char to the supplied buffer as hexadecimal code
    void WriteCharHexTo(char buf[2], char ch);

    // Append the char to the supplied string as hexadecimal code
    void WriteCharHexTo(std::string& str, char ch, bool clear = true);

    // Append the unicode code point to a big endian encoded utf16 string
    void WriteToUtf16BE(std::u16string& str, char32_t codePoint, bool clear = true);

    // https://stackoverflow.com/a/38140932/213871
    inline void hash_combine(std::size_t& seed) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9E3779B9 + (seed << 6) + (seed >> 2);
        hash_combine(seed, rest...);
    }

    // Returns log(ch) / log(256) + 1
    unsigned char GetCharCodeSize(unsigned ch);

    // Returns pow(2, size * 8) - 1
    unsigned GetCharCodeMaxValue(unsigned char codeSize);

    template<typename T>
    void move(T& in, T& out)
    {
        out = in;
        in = { };
    }
}

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
 * \page <pdfmm PdfDefinesPrivate Header>
 *
 * <b>PdfDefinesPrivate.h</b> contains preprocessor definitions, inline functions, templates,
 * compile-time const variables, and other things that must be visible across the entirety of
 * the pdfmm library code base but should not be visible to users of the library's headers.
 *
 * This header is private to the library build. It is not installed with pdfmm and must not be
 * referenced in any way from any public, installed header.
 */

#endif // PDF_DEFINES_PRIVATE_H
