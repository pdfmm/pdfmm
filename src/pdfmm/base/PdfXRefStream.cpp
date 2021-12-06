/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfXRefStream.h"

#include "PdfObject.h"
#include "PdfObjectStream.h"
#include "PdfWriter.h"
#include "PdfDictionary.h"

using namespace mm;

PdfXRefStream::PdfXRefStream(PdfWriter& writer) :
    PdfXRef(writer),
    m_xrefStreamEntryIndex(-1),
    m_xrefStreamObj(writer.GetObjects().CreateDictionaryObject("XRef")),
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
    // Do nothing
}

void PdfXRefStream::WriteSubSection(PdfOutputDevice&, uint32_t first, uint32_t count)
{
    m_indices.push_back(static_cast<int64_t>(first));
    m_indices.push_back(static_cast<int64_t>(count));
}

void PdfXRefStream::WriteXRefEntry(PdfOutputDevice& device, const PdfReference& ref, const PdfXRefEntry& entry)
{
    (void)device;
    XRefStreamEntry stmEntry;
    stmEntry.Type = static_cast<uint8_t>(entry.Type);

    if (m_xrefStreamObj->GetIndirectReference() == ref)
        m_xrefStreamEntryIndex = (int)m_rawEntries.size();

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
    m_rawEntries.push_back(stmEntry);
}

void PdfXRefStream::EndWriteImpl(PdfOutputDevice& device)
{
    PdfArray wArr;
    wArr.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Type)));
    wArr.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Variant)));
    wArr.push_back(static_cast<int64_t>(sizeof(XRefStreamEntry::Generation)));
 
    m_xrefStreamObj->GetDictionary().AddKey("Index", m_indices);
    m_xrefStreamObj->GetDictionary().AddKey("W", wArr);
 
    // Set the actual offset of the XRefStm object
    uint32_t offset = (uint32_t)device.Tell();
    PDFMM_ASSERT(m_xrefStreamEntryIndex >= 0);
    m_rawEntries[m_xrefStreamEntryIndex].Variant = AS_BIG_ENDIAN(offset);
 
    // Write the actual entries data to the XRefStm object stream
    auto& stream = m_xrefStreamObj->GetOrCreateStream();
    stream.BeginAppend();
    stream.Append((const char*)m_rawEntries.data(), m_rawEntries.size() * sizeof(XRefStreamEntry));
    stream.EndAppend();
    GetWriter().FillTrailerObject(*m_xrefStreamObj, this->GetSize(), false);

    m_xrefStreamObj->Write(device, GetWriter().GetWriteMode(), nullptr); // CHECK-ME: Requires encryption info??
    m_offset = offset;
}
