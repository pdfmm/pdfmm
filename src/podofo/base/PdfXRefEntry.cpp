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
