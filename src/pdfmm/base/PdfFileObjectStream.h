/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FILE_OBJECT_STREAM_H
#define PDF_FILE_OBJECT_STREAM_H

#include "PdfDeclarations.h"

#include "PdfObjectStream.h"

namespace mm {

class PdfOutputStream;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfFileObjectStream writes all data directly to an output device
 *  without keeping it in memory.
 *  PdfFileObjectStream is used automatically when creating PDF files
 *  using PdfImmediateWriter.
 *
 *  \see PdfIndirectObjectList
 *  \see PdfObjectStream
 */
class PDFMM_API PdfFileObjectStream final : public PdfObjectStream
{
public:
    /** Create a new PdfFileObjectStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *
     *  \param parent parent object
     *  \param device output device
     */
    PdfFileObjectStream(PdfObject& parent, PdfOutputDevice& device);

    ~PdfFileObjectStream();

    /** Set an encryption object which is used to encrypt
     *  all data written to this stream.
     *
     *  \param encrypt an encryption object or nullptr if no encryption should be done
     */
    void SetEncrypted(PdfEncrypt* encrypt);

    void Write(PdfOutputDevice& device, const PdfEncrypt* encrypt) override;

    void GetCopy(std::unique_ptr<char[]>& buffer, size_t& len) const override;

    void GetCopy(PdfOutputStream& stream) const override;

    size_t GetLength() const override;

    PdfFileObjectStream& operator=(const PdfFileObjectStream& rhs);

protected:
    const char* GetInternalBuffer() const override;
    size_t GetInternalBufferSize() const override;
    void BeginAppendImpl(const PdfFilterList& filters) override;
    void AppendImpl(const char* data, size_t len) override;
    void EndAppendImpl() override;

private:
    PdfOutputDevice* m_Device;
    std::unique_ptr<PdfOutputStream> m_Stream;
    std::unique_ptr<PdfOutputStream> m_DeviceStream;
    std::unique_ptr<PdfOutputStream> m_EncryptStream;

    size_t m_initialLength;
    size_t m_Length;

    PdfObject* m_LengthObj;

    PdfEncrypt* m_CurrEncrypt;
};

};

#endif // PDF_FILE_OBJECT_STREAM_H
