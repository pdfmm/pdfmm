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

   // We can use the ISO C++ headers with other compilers
#  include <cstdlib>
#  include <cstdio>
#  include <cmath>
#  include <cstring>
#  include <ctime>


#if PODOFO_HAVE_WINSOCK2_H
#  ifdef PODOFO_MULTI_THREAD
#    if defined(_WIN32) || defined(_WIN64)
#      ifndef _WIN32_WINNT
#        define _WIN32_WINNT 0x0400 // Make the TryEnterCriticalSection method available
#        include <winsock2.h>       // This will include windows.h, so we have to define _WIN32_WINNT
                                    // if we want to use threads later.
#        undef _WIN32_WINNT 
#      else
#        include <winsock2.h>
#      endif // _WIN32_WINNT
#    endif // _WIN32 || _WIN64
#  else
#    include <winsock2.h>
#  endif // PODOFO_MULTI_THREAD
#endif

#if PODOFO_HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif

#ifdef PODOFO_MULTI_THREAD
#  if defined(_WIN32) || defined(_WIN64)
#    if defined(_MSC_VER) && !defined(_WINSOCK2API_)
#      error <winsock2.h> must be included before <windows.h>, config problem?
#    endif
#    ifndef _WIN32_WINNT
#      define _WIN32_WINNT 0x0400 // Make the TryEnterCriticalSection method available
#      include <windows.h>
#      undef _WIN32_WINNT 
#    else
#      include <windows.h>
#    endif // _WIN32_WINNT
#  else
#    include <pthread.h>
#  endif // _WIN32
#endif // PODOFO_MULTI_THREAD

#if defined(_WIN32) || defined(_WIN64)
#  if defined(GetObject)
#    undef GetObject // Horrible windows.h macro definition that breaks things
#  endif
#  if defined(DrawText)
#    undef DrawText // Horrible windows.h macro definition that breaks things
#  endif
#  if defined(CreateFont)
#    undef CreateFont
#  endif
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
inline static int strcasecmp( const char * s1, const char * s2) {
#if defined(_WIN32) || defined (_WIN64)
#   if defined(_MSC_VER)
        // MSVC++
        return ::_stricmp(s1, s2);
#   else
        return ::stricmp(s1, s2);
#   endif
#else
    // POSIX.1-2001
    return ::strcasecmp(s1, s2);
#endif
}

inline static int strncasecmp( const char * s1, const char * s2, size_t n) {
#if defined(_WIN32) || defined(_WIN64)
#   if defined(_MSC_VER)
        // MSVC++
        return ::_strnicmp(s1, s2, n);
#   else
        return ::strnicmp(s1, s2, n);
#   endif
#else
    // POSIX.1-2001
    return ::strncasecmp(s1, s2, n);
#endif
}

};}; // end namespace PoDoFo::compat

/*
 * This is needed to enable compilation with VC++ on Windows, which likes to prefix
 * many functions with underscores.
 *
 * TODO: These should probably be inline wrappers instead, and we need to consolidate
 * hacks from the rest of the code where other _underscore_prefixed_names are checked
 * for here.
 */
#ifdef _MSC_VER
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

#if defined(_WIN64)
#define fseeko _fseeki64
#define ftello _ftelli64
#else
#define fseeko fseek
#define ftello ftell
#endif

#endif
