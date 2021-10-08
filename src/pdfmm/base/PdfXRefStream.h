/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XREF_STREAM_H
#define PDF_XREF_STREAM_H

#include "PdfDefines.h"

#include "PdfArray.h"
#include "PdfXRef.h"

namespace mm {

class PdfOutputDevice;
class PdfIndirectObjectList;

/**
 * Creates an XRef table that is a stream object.
 * Requires at least PDF 1.5. XRef streams are more
 * compact than normal XRef tables.
 *
 * This is an internal class of pdfmm used by PdfWriter.
 */
class PdfXRefStream : public PdfXRef
{
public:
    /** Create a new XRef table
     *
     *  \param writer is needed to fill the trailer directory
     *                 correctly which is included into the XRef
     *  \param parent a vector of PdfObject is required
     *                 to create a PdfObject for the XRef
     */
    PdfXRefStream(PdfWriter& writer);

    uint64_t GetOffset() const override;

    bool ShouldSkipWrite(const PdfReference& ref) override;

protected:
    void BeginWrite(PdfOutputDevice& device) override;
    void WriteSubSection(PdfOutputDevice& device, uint32_t first, uint32_t count) override;
    void WriteXRefEntry(PdfOutputDevice& device, const PdfReference& ref, const PdfXRefEntry& entry) override;
    void EndWriteImpl(PdfOutputDevice& device) override;

private:
#pragma pack(push, 1)
    // TODO: Handle for different byte size for object number/offset/generation
    struct XRefStreamEntry
    {
        uint8_t Type;
        uint32_t Variant; // Can be an object number or an offset
        uint16_t Generation;
    };
#pragma pack(pop)

private:
    std::vector<XRefStreamEntry> m_rawEntries;
    int m_xrefStreamEntryIndex;
    PdfObject* m_xrefStreamObj;
    PdfArray m_indices;
    int64_t m_offset;
};

};

#endif // PDF_XREF_STREAM_H
