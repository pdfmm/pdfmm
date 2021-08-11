/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_MEM_STREAM_H
#define PDF_MEM_STREAM_H

#include "PdfDefines.h"

#include "PdfStream.h"
#include "PdfDictionary.h"
#include "PdfSharedBuffer.h"

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
 *  A PdfMemStream is implicitly shared and can therefore be copied very quickly.
 */
class PDFMM_API PdfMemStream final : public PdfStream
{
    friend class PdfIndirectObjectList;
public:

    /** Create a new PdfStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *  \param parent parent object
     */
    PdfMemStream(PdfObject& parent);

    ~PdfMemStream();

    void Write(PdfOutputDevice& device, const PdfEncrypt* encrypt) override;

    // TODO: Make version with std::unique_ptr<char>
    void GetCopy(char** buffer, size_t* len) const override;

    void GetCopy(PdfOutputStream& stream) const override;

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

    const PdfMemStream& operator=(const PdfMemStream& rhs);
    const PdfMemStream& operator=(const PdfStream& rhs);

 protected:
    const char* GetInternalBuffer() const override;
    size_t GetInternalBufferSize() const override;
    void BeginAppendImpl(const PdfFilterList& filters) override;
    void AppendImpl(const char* data, size_t len) override;
    void EndAppendImpl() override;
    void CopyFrom(const PdfStream& rhs) override;

private:
    PdfMemStream(const PdfMemStream& rhs) = delete;
    void copyFrom(const PdfMemStream& rhs);

 private:
    chars m_buffer;
    std::unique_ptr<PdfOutputStream> m_Stream;
    std::unique_ptr<PdfOutputStream> m_BufferStream;
};

};

#endif // PDF_MEM_STREAM_H
