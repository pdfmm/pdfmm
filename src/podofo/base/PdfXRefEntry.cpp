/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfXRefEntry.h"

using namespace PoDoFo;

PdfXRefEntry::PdfXRefEntry() :
    Unknown1(0),
    Unknown2(0),
    Type(XRefEntryType::Unknown),
    Parsed(false)
{ }

PdfXRefEntry PdfXRefEntry::CreateFree(uint32_t object, uint16_t generation)
{
    PdfXRefEntry ret;
    ret.ObjectNumber = object;
    ret.Generation = generation;
    ret.Type = XRefEntryType::Free;
    return ret;
}

PdfXRefEntry PdfXRefEntry::CreateInUse(uint64_t offset, uint16_t generation)
{
    PdfXRefEntry ret;
    ret.Offset = offset;
    ret.Generation = generation;
    ret.Type = XRefEntryType::InUse;
    return ret;
}

PdfXRefEntry PdfXRefEntry::CreateCompressed(uint32_t object, unsigned index)
{
    PdfXRefEntry ret;
    ret.ObjectNumber = object;
    ret.Index = index;
    ret.Type = XRefEntryType::Compressed;
    return ret;
}

char PoDoFo::XRefEntryTypeToChar(XRefEntryType type)
{
    switch (type)
    {
        case XRefEntryType::Free:
            return 'f';
        case XRefEntryType::InUse:
            return 'n';
        case XRefEntryType::Unknown:
        case XRefEntryType::Compressed:
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }
}

XRefEntryType PoDoFo::XRefEntryTypeFromChar(char c)
{
    switch (c)
    {
        case 'f':
            return XRefEntryType::Free;
        case 'n':
            return XRefEntryType::InUse;
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidXRef);
    }
}
