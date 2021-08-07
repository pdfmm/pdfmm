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
class PdfVecObjects;

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
     *  \param parent a vector of PdfObject is required
     *                 to create a PdfObject for the XRef
     *  \param pWriter is needed to fill the trailer directory
     *                 correctly which is included into the XRef
     */
    PdfXRefStream(PdfWriter& writer, PdfVecObjects& parent);

    uint64_t GetOffset() const override;

    bool ShouldSkipWrite(const PdfReference& ref) override;

protected:
    void BeginWrite(PdfOutputDevice& device) override;
    void WriteSubSection(PdfOutputDevice& device, uint32_t nFirst, uint32_t nCount) override;
    void WriteXRefEntry(PdfOutputDevice& device, const PdfXRefEntry& entry) override;
    void EndWriteImpl(PdfOutputDevice& device) override;

private:
    PdfVecObjects* m_Parent;
    PdfObject* m_xrefStreamObj;
    PdfArray m_indices;
    int64_t m_offset;
};

};

#endif // PDF_XREF_STREAM_H
