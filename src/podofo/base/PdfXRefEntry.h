/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#pragma once
#ifndef PDF_XREF_ENTRY_H
#define PDF_XREF_ENTRY_H

#include "PdfDefines.h"

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

    struct PdfXRefEntry final
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

#endif // PDF_XREF_ENTRY_H
