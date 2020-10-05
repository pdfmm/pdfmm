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
