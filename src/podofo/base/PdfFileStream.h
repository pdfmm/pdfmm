/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FILE_STREAM_H
#define PDF_FILE_STREAM_H

#include "PdfDefines.h"

#include "PdfStream.h"

namespace PoDoFo {

class PdfOutputStream;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  A PdfFileStream writes all data directly to an output device
 *  without keeping it in memory.
 *  PdfFileStream is used automatically when creating PDF files
 *  using PdfImmediateWriter.
 *
 *  \see PdfVecObjects
 *  \see PdfStream
 *  \see PdfMemoryStream
 *  \see PdfFileStream
 */
class PODOFO_API PdfFileStream final : public PdfStream
{
public:
    /** Create a new PdfFileStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *
     *  \param parent parent object
     *  \param device output device
     */
    PdfFileStream(PdfObject& parent, PdfOutputDevice& device);

    ~PdfFileStream();

    /** Set an encryption object which is used to encrypt
     *  all data written to this stream.
     *
     *  \param encrypt an encryption object or nullptr if no encryption should be done
     */
    void SetEncrypted(PdfEncrypt* encrypt);

    void Write(PdfOutputDevice& device, const PdfEncrypt* encrypt) override;

    void GetCopy(char** buffer, size_t* len) const override;

    void GetCopy(PdfOutputStream& stream) const override;

    size_t GetLength() const override;

protected:
    const char* GetInternalBuffer() const override;
    size_t GetInternalBufferSize() const override;
    void BeginAppendImpl(const PdfFilterList& filters) override;
    void AppendImpl(const char* data, size_t len) override;
    void EndAppendImpl() override;

private:
    PdfFileStream(const PdfFileStream& stream) = delete;
    const PdfFileStream& operator=(const PdfFileStream& stream) = delete;

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

#endif // PDF_FILE_STREAM_H
