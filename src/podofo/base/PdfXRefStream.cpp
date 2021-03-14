/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfXRefStream.h"

#include "PdfObject.h"
#include "PdfStream.h"
#include "PdfWriter.h"
#include "PdfDefinesPrivate.h"
#include "PdfDictionary.h"

using namespace PoDoFo;

#pragma pack(push, 1)

// TODO: Handle for different byte size for object number/offset/generation
struct XRefStreamEntry
{
    uint8_t Type;
    uint32_t Variant; // Can be an object number or an offset
    uint16_t Generation;
};

#pragma pack(pop)

PdfXRefStream::PdfXRefStream(PdfWriter& writer, PdfVecObjects& parent) :
    PdfXRef(writer),
    m_pParent(&parent),
    m_xrefStreamObj(parent.CreateDictionaryObject("XRef")),
    m_offset(-1)
{
}

uint64_t PdfXRefStream::GetOffset() const
{
    if (m_offset < 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "XRefStm has not been written yet");

    return (uint64_t)m_offset;
}

bool PdfXRefStream::ShouldSkipWrite(const PdfReference& ref)
{
    // We handle writing for the XRefStm object
    if (m_xrefStreamObj->GetIndirectReference() == ref)
        return true;
    else
        return false;
}

void PdfXRefStream::BeginWrite( PdfOutputDevice& )
{
    m_xrefStreamObj->GetOrCreateStream().BeginAppend();
}

void PdfXRefStream::WriteSubSection(PdfOutputDevice&, uint32_t first, uint32_t count )
{
    m_indeces.push_back( static_cast<int64_t>(first) );
    m_indeces.push_back( static_cast<int64_t>(count) );
}

void PdfXRefStream::WriteXRefEntry(PdfOutputDevice&, const PdfXRefEntry& entry)
{
    XRefStreamEntry stmEntry;
    stmEntry.Type = static_cast<uint8_t>(entry.Type);

    switch (entry.Type)
    {
    case EXRefEntryType::Free:
        stmEntry.Variant = compat::AsBigEndian(static_cast<uint32_t>(entry.ObjectNumber));
        break;
    case EXRefEntryType::InUse:
        stmEntry.Variant = compat::AsBigEndian(static_cast<uint32_t>(entry.Offset));
        break;
    default:
        PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }

    stmEntry.Generation = compat::AsBigEndian(static_cast<uint16_t>(entry.Generation));
    m_xrefStreamObj->GetOrCreateStream().Append((char *)&stmEntry, sizeof(XRefStreamEntry));
}

void PdfXRefStream::EndWriteImpl(PdfOutputDevice& device)
{
    m_xrefStreamObj->GetOrCreateStream().EndAppend();
    GetWriter().FillTrailerObject(*m_xrefStreamObj, this->GetSize(), false );

    PdfArray w;
    w.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Type)));
    w.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Variant)));
    w.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Generation)));

    m_xrefStreamObj->GetDictionary().AddKey( "Index", m_indeces );
    m_xrefStreamObj->GetDictionary().AddKey( "W", w );

    int64_t offset = (int64_t)device.Tell();
    m_xrefStreamObj->Write(device, GetWriter().GetWriteMode(), nullptr); // CHECK-ME: Requires encryption info??
    m_offset = offset;
}
