/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfXRefEntry.h"

using namespace PoDoFo;

PdfXRefEntry::PdfXRefEntry() :
    Unknown1(0),
    Unknown2(0),
    Type(EXRefEntryType::Unknown),
    Parsed(false)
{ }

PdfXRefEntry PdfXRefEntry::CreateFree(uint32_t object, uint16_t generation)
{
    PdfXRefEntry ret;
    ret.ObjectNumber = object;
    ret.Generation = generation;
    ret.Type = EXRefEntryType::Free;
    return ret;
}

PdfXRefEntry PdfXRefEntry::CreateInUse(uint64_t offset, uint16_t generation)
{
    PdfXRefEntry ret;
    ret.Offset = offset;
    ret.Generation = generation;
    ret.Type = EXRefEntryType::InUse;
    return ret;
}

PdfXRefEntry PdfXRefEntry::CreateCompressed(uint32_t object, unsigned index)
{
    PdfXRefEntry ret;
    ret.ObjectNumber = object;
    ret.Index = index;
    ret.Type = EXRefEntryType::Compressed;
    return ret;
}

char PoDoFo::XRefEntryType(EXRefEntryType type)
{
    switch (type)
    {
        case EXRefEntryType::Free:
            return 'f';
        case EXRefEntryType::InUse:
            return 'n';
        case EXRefEntryType::Unknown:
        case EXRefEntryType::Compressed:
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }
}

EXRefEntryType PoDoFo::XRefEntryTypeFromChar(char c)
{
    switch (c)
    {
        case 'f':
            return EXRefEntryType::Free;
        case 'n':
            return EXRefEntryType::InUse;
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidXRef);
    }
}
