/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_MEMORY_OBJECT_STREAM_H
#define PDF_MEMORY_OBJECT_STREAM_H

#include "PdfDeclarations.h"

#include "PdfObjectStream.h"
#include "PdfDictionary.h"

namespace mm {

class PdfName;
class PdfObject;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  A PDF memory stream is held completely in memory.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfMemoryObjectStream is implicitly shared and can therefore be copied very quickly.
 */
class PDFMM_API PdfMemoryObjectStream final : public PdfObjectStream
{
    friend class PdfIndirectObjectList;

public:
    PdfMemoryObjectStream(PdfObject& parent);

    ~PdfMemoryObjectStream();

    void Write(OutputStream& stream, const PdfStatefulEncrypt& encrypt) override;

    size_t GetLength() const override;

    /** Get a read-only handle to the current stream data.
     *  The data will not be filtered before being returned, so (eg) calling
     *  Get() on a Flate compressed stream will return a pointer to the
     *  Flate-compressed buffer.
     *
     *  \warning Do not retain pointers to the stream's internal buffer,
     *           as it may be reallocated with any non-const operation.
     *
     *  \returns a read-only handle to the streams data
     */
    const char* Get() const;

    PdfMemoryObjectStream& operator=(const PdfMemoryObjectStream& rhs);

protected:
    std::unique_ptr<InputStream> getInputStream() override;
    std::unique_ptr<OutputStream> getOutputStream() override;
    void CopyFrom(const PdfObjectStream& rhs) override;

private:
    void copyFrom(const PdfMemoryObjectStream& rhs);

 private:
    charbuff m_buffer;
};

};

#endif // PDF_MEMORY_OBJECT_STREAM_H
