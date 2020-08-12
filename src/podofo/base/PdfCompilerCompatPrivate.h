/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#include <cstdlib>
#include <cstdio>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <ctime>
#include <cinttypes>

#ifndef WIN32
#include <strings.h>
#endif

namespace PoDoFo {
namespace compat {

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

// Case-insensitive string compare functions aren't very portable, and we must account
// for several flavours.
inline static int strcasecmp( const char * s1, const char * s2)
{
#if defined(WIN32)
    return ::_stricmp(s1, s2);
#else
    return ::strcasecmp(s1, s2);
#endif
}

inline static int strncasecmp( const char * s1, const char * s2, size_t n)
{
#if defined(_MSC_VER)
    return ::_strnicmp(s1, s2, n);
#else
    // POSIX.1-2001
    return ::strncasecmp(s1, s2, n);
#endif
}

};}; // end namespace PoDoFo::compat

#if defined(_WIN64)
#define fseeko _fseeki64
#define ftello _ftelli64
#else
#define fseeko fseek
#define ftello ftell
#endif

#endif
