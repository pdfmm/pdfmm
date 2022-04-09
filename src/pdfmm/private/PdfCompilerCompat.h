/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_COMPILER_COMPAT_PRIVATE_H
#define PDF_COMPILER_COMPAT_PRIVATE_H

#ifndef PDF_DEFINES_PRIVATE_H
#error Include PdfDeclarationsPrivate.h instead
#endif

#include <cstdint>
#include <cstdlib>

#ifdef _WIN32
// Microsft itself assumes little endian
// https://github.com/microsoft/STL/blob/b11945b73fc1139d3cf1115907717813930cedbf/stl/inc/bit#L336
#define PDFMM_IS_LITTLE_ENDIAN
#else // Unix

#if !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__) || !defined(__ORDER_BIG_ENDIAN__)
#error "Byte order macros are not defined"
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define PDFMM_IS_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define PDFMM_IS_BIG_ENDIAN
#else
#error "__BYTE_ORDER__ macro has not"
#endif

#endif // _WIN32

#ifdef PDFMM_IS_LITTLE_ENDIAN
#define AS_BIG_ENDIAN(n) mm::compat::ByteSwap(n)
#define FROM_BIG_ENDIAN(n) mm::compat::ByteSwap(n)
#else // PDFMM_IS_BIG_ENDIAN
#define AS_BIG_ENDIAN(n) n
#define FROM_BIG_ENDIAN(n) n
#endif

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
}

#endif // PDF_COMPILER_COMPAT_PRIVATE_H
