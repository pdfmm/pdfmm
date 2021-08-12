/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_COMPILERCOMPAT_PRIVATE_H
#define PDF_COMPILERCOMPAT_PRIVATE_H

#ifndef PDF_DEFINES_PRIVATE_H
#error Include PdfDefinesPrivate.h instead
#endif

#define _USE_MATH_DEFINES

#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <ctime>
#include <cinttypes>
#include <climits>

#if defined(TEST_BIG)
#  define PDFMM_IS_BIG_ENDIAN
#else
#  define PDFMM_IS_LITTLE_ENDIAN
#endif

 // Different compilers use different format specifiers for 64-bit integers
 // (yay!).  Use these macros with C's automatic string concatenation to handle
 // that ghastly quirk.
 //
 // for example:   printf("Value of signed 64-bit integer: %"PDF_FORMAT_INT64" (more blah)", 128)
 //
#if defined(_MSC_VER)
#  define PDF_FORMAT_INT64 "I64d"
#  define PDF_FORMAT_UINT64 "I64u"
#  define PDF_SIZE_FORMAT "Iu"
#else
#  define PDF_FORMAT_INT64 "lld"
#  define PDF_FORMAT_UINT64 "llu"
#  define PDF_SIZE_FORMAT "zu"
#endif

#define AS_BIG_ENDIAN(n) mm::compat::HandleBigEndian(n)
#define FROM_BIG_ENDIAN(n) mm::compat::HandleBigEndian(n)

namespace mm::compat
{
#ifdef _MSC_VER
    inline uint16_t ByteSwap(uint16_t n)
    {
        return _byteswap_ushort(n);
    }

    inline uint32_t ByteSwap(uint32_t n)
    {
        return _byteswap_ulong(n);
    }

    inline uint64_t ByteSwap(uint64_t n)
    {
        return _byteswap_uint64(n);
    }

    inline int16_t ByteSwap(int16_t n)
    {
        return (int16_t)_byteswap_ushort((uint16_t)n);
    }

    inline int32_t ByteSwap(int32_t n)
    {
        return (int32_t)_byteswap_ulong((uint32_t)n);
    }

    inline int64_t ByteSwap(int64_t n)
    {
        return (int64_t)_byteswap_uint64((uint64_t)n);
    }
#else
    inline uint16_t ByteSwap(uint16_t n)
    {
        return __builtin_bswap16(n);
    }

    inline uint32_t ByteSwap(uint32_t n)
    {
        return __builtin_bswap32(n);
    }

    inline uint64_t ByteSwap(uint64_t n)
    {
        return __builtin_bswap64(n);
    }

    inline int16_t ByteSwap(int16_t n)
    {
        return (int16_t)__builtin_bswap16((uint16_t)n);
    }

    inline int32_t ByteSwap(int32_t n)
    {
        return (int32_t)__builtin_bswap32((uint32_t)n);
    }

    inline int64_t ByteSwap(int64_t n)
    {
        return (int64_t)__builtin_bswap64((uint64_t)n);
    }
#endif

#ifdef PDFMM_IS_LITTLE_ENDIAN
    inline uint16_t HandleBigEndian(uint16_t n)
    {
        return ByteSwap(n);
    }

    inline uint32_t HandleBigEndian(uint32_t n)
    {
        return ByteSwap(n);
    }

    inline uint64_t HandleBigEndian(uint64_t n)
    {
        return ByteSwap(n);
    }

    inline int16_t HandleBigEndian(int16_t n)
    {
        return ByteSwap(n);
    }

    inline int32_t HandleBigEndian(int32_t n)
    {
        return ByteSwap(n);
    }

    inline int64_t HandleBigEndian(int64_t n)
    {
        return ByteSwap(n);
    }
#else
    inline uint16_t HandleBigEndian(uint16_t n)
    {
        return n;
    }

    inline uint32_t HandleBigEndian(uint32_t n)
    {
        return n;
    }

    inline uint64_t HandleBigEndian(uint64_t n)
    {
        return n;
    }

    inline int16_t HandleBigEndian(int16_t n)
    {
        return n;
    }

    inline int32_t HandleBigEndian(int32_t n)
    {
        return n;
    }

    inline int64_t HandleBigEndian(int64_t n)
    {
        return n;
    }
#endif

    // Locale invariant vsnprintf
    int vsnprintf(char* buffer, size_t count, const char* format, va_list argptr);

    // Case-insensitive string compare functions aren't very portable
    int strcasecmp(const char* s1, const char* s2);
    int strncasecmp(const char* s1, const char* s2, size_t n);
}

#endif // PDF_COMPILERCOMPAT_PRIVATE_H
