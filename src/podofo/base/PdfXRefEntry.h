/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#pragma once

#include "PdfDefines.h"
#include <vector>

namespace PoDoFo
{
    // Values cast directly to XRefStm binary representation
    enum class EXRefEntryType : int8_t
    {
        Unknown = -1,
        Free = 0,
        InUse = 1,
        Compressed = 2,
    };

    struct PdfXRefEntry
    {
        PdfXRefEntry();

        static PdfXRefEntry CreateFree(uint32_t object, uint16_t generation);

        static PdfXRefEntry CreateInUse(uint64_t offset, uint16_t generation);

        static PdfXRefEntry CreateCompressed(uint32_t object, unsigned index);

        // The following aliasing should be allowed in C++
        // https://stackoverflow.com/a/15278030/213871
        union
        {
            uint64_t ObjectNumber;  // Object number in Free and Compressed entries
            uint64_t Offset;        // Unsed in InUse entries
            uint64_t Unknown1;
        };
        union
        {
            uint32_t Generation;    // The generation of the object in Free and InUse entries
            uint32_t Index;         // Index of the object in the stream for Compressed entries
            uint32_t Unknown2;
        };
        EXRefEntryType Type;
        bool Parsed;
    };

    char XRefEntryType(EXRefEntryType type);
    EXRefEntryType XRefEntryTypeFromChar(char c);

    typedef std::vector<PdfXRefEntry> TVecEntries;
    typedef TVecEntries::iterator TIVecEntries;
    typedef TVecEntries::const_iterator TCIVecEntries;
};
