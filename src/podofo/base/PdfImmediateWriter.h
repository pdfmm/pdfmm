/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_IMMEDIATE_WRITER_H
#define PDF_IMMEDIATE_WRITER_H

#include "PdfDefines.h"
#include "PdfVecObjects.h"
#include "PdfWriter.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfOutputDevice;
class PdfXRef;

/** A kind of PdfWriter that writes objects with streams immediately to
 *  a PdfOutputDevice
 */
class PODOFO_API PdfImmediateWriter : private PdfWriter,
    private PdfVecObjects::Observer,
    private PdfVecObjects::StreamFactory
{
public:
    /** Create a new PdfWriter that writes objects with streams immediately to a PdfOutputDevice
     *
     *  This has the advantage that large documents can be created without
     *  having to keep the whole document in memory.
     *
     *  @param pDevice all stream streams are immediately written to this output device
     *                 while the document is created.
     *  @param pVecObjects a vector of objects containing the objects which are written to disk
     *  @param pTrailer the trailer object
     *  @param eVersion the PDF version of the document to write.
     *                      The PDF version can only be set in the constructor
     *                      as it is the first item written to the document on disk.
     *  @param pEncrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  @param eWriteMode additional options for writing the pdf
     */
    PdfImmediateWriter(PdfVecObjects& pVecObjects, const PdfObject& pTrailer, PdfOutputDevice& pDevice,
        PdfVersion eVersion = PdfVersion::V1_5, PdfEncrypt* pEncrypt = nullptr,
        PdfWriteMode eWriteMode = PdfWriteModeDefault);

    ~PdfImmediateWriter();

public:
    /** Get the write mode used for writing the PDF
     *  \returns the write mode
     */
    PdfWriteMode GetWriteMode() const;

    /** Get the PDF version of the document
     *  The PDF version can only be set in the constructor
     *  as it is the first item written to the document on disk
     *
     *  \returns EPdfVersion version of the pdf document
     */
    PdfVersion GetPdfVersion() const;

private:
    void WriteObject(const PdfObject& obj) override;
    void Finish() override;
    void BeginAppendStream(const PdfStream& pStream) override;
    void EndAppendStream(const PdfStream& pStream) override;
    PdfStream* CreateStream(PdfObject& parent) override;

    /** Assume the stream for the last object has
     *  been written complete.
     *  Therefore close the stream of the object
     *  now so that the next object can be written
     *  to disk
     */
    void FinishLastObject();

private:
    bool m_attached;
    PdfOutputDevice* m_Device;
    std::unique_ptr<PdfXRef> m_XRef;
    PdfObject* m_Last;
    bool m_OpenStream;
};

};

#endif // PDF_IMMEDIATE_WRITER_H
