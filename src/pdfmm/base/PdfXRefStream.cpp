/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfXRefStream.h"

#include "PdfObject.h"
#include "PdfStream.h"
#include "PdfWriter.h"
#include "PdfDictionary.h"

using namespace mm;

#pragma pack(push, 1)

// TODO: Handle for different byte size for object number/offset/generation
struct XRefStreamEntry
{
    uint8_t Type;
    uint32_t Variant; // Can be an object number or an offset
    uint16_t Generation;
};

#pragma pack(pop)

PdfXRefStream::PdfXRefStream(PdfWriter& writer, PdfIndirectObjectList& parent) :
    PdfXRef(writer),
    m_Parent(&parent),
    m_xrefStreamObj(parent.CreateDictionaryObject("XRef")),
    m_offset(-1)
{
}

uint64_t PdfXRefStream::GetOffset() const
{
    if (m_offset < 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "XRefStm has not been written yet");

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

void PdfXRefStream::BeginWrite(PdfOutputDevice&)
{
    m_xrefStreamObj->GetOrCreateStream().BeginAppend();
}

void PdfXRefStream::WriteSubSection(PdfOutputDevice&, uint32_t first, uint32_t count)
{
    m_indices.push_back(static_cast<int64_t>(first));
    m_indices.push_back(static_cast<int64_t>(count));
}

void PdfXRefStream::WriteXRefEntry(PdfOutputDevice&, const PdfXRefEntry& entry)
{
    XRefStreamEntry stmEntry;
    stmEntry.Type = static_cast<uint8_t>(entry.Type);

    switch (entry.Type)
    {
        case XRefEntryType::Free:
            stmEntry.Variant = AS_BIG_ENDIAN(static_cast<uint32_t>(entry.ObjectNumber));
            break;
        case XRefEntryType::InUse:
            stmEntry.Variant = AS_BIG_ENDIAN(static_cast<uint32_t>(entry.Offset));
            break;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    stmEntry.Generation = AS_BIG_ENDIAN(static_cast<uint16_t>(entry.Generation));
    m_xrefStreamObj->GetOrCreateStream().Append((char*)&stmEntry, sizeof(XRefStreamEntry));
}

void PdfXRefStream::EndWriteImpl(PdfOutputDevice& device)
{
    m_xrefStreamObj->GetOrCreateStream().EndAppend();
    GetWriter().FillTrailerObject(*m_xrefStreamObj, this->GetSize(), false);

    PdfArray w;
    w.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Type)));
    w.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Variant)));
    w.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Generation)));

    m_xrefStreamObj->GetDictionary().AddKey("Index", m_indices);
    m_xrefStreamObj->GetDictionary().AddKey("W", w);

    int64_t offset = (int64_t)device.Tell();
    m_xrefStreamObj->Write(device, GetWriter().GetWriteMode(), nullptr); // CHECK-ME: Requires encryption info??
    m_offset = offset;
}
