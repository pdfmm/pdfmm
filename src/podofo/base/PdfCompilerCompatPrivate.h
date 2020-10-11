/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_COMPILERCOMPAT_PRIVATE_H
#define _PDF_COMPILERCOMPAT_PRIVATE_H

#ifndef _PDF_DEFINES_PRIVATE_H_
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

#if defined(TEST_BIG)
#  define PODOFO_IS_BIG_ENDIAN
#else
#  define PODOFO_IS_LITTLE_ENDIAN
#endif

 // Different compilers use different format specifiers for 64-bit integers
 // (yay!).  Use these macros with C's automatic string concatenation to handle
 // that ghastly quirk.
 //
 // for example:   printf("Value of signed 64-bit integer: %"PDF_FORMAT_INT64" (more blah)", 128LL)
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

namespace PoDoFo::compat {

#ifdef _MSC_VER
#  define byteswap16(n) _byteswap_ushort(n)
#  define byteswap32(n) _byteswap_ulong(n)
#  define byteswap64(n) _byteswap_uint64(n)
#else
#  define byteswap16(n) __builtin_bswap16(n)
#  define byteswap32(n) __builtin_bswap32(n)
#  define byteswap64(n) __builtin_bswap64(n)
#endif

#ifdef PODOFO_IS_LITTLE_ENDIAN
    inline uint16_t AsBigEndian(uint16_t n)
    {
        return byteswap16(n);
    }

    inline uint32_t AsBigEndian(uint32_t n)
    {
        return byteswap32(n);
    }

    inline uint64_t AsBigEndian(uint64_t n)
    {
        return byteswap64(n);
    }
#else
    inline uint16_t AsBigEndian(uint16_t n)
    {
        return n;
    }

    inline uint32_t AsBigEndian(uint32_t n)
    {
        return n;
    }

    inline uint64_t AsBigEndian(uint64_t n)
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

#endif // _PDF_COMPILERCOMPAT_PRIVATE_H
