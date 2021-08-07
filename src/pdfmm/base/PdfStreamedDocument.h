/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_STREAMED_DOCUMENT_H
#define PDF_STREAMED_DOCUMENT_H

#include "PdfDefines.h"

#include "PdfDocument.h"
#include "PdfImmediateWriter.h"

namespace mm {

class PdfOutputDevice;

/** PdfStreamedDocument is the preferred class for
 *  creating new PDF documents.
 *
 *  Page contents, fonts and images are written to disk
 *  as soon as possible and are not kept in memory.
 *  This results in faster document generation and
 *  less memory being used.
 *
 *  Please use PdfMemDocument if you intend to work
 *  on the object structure of a PDF file.
 *
 *  One of the design goals of PdfStreamedDocument was
 *  to hide the underlying object structure of a PDF
 *  file as far as possible.
 *
 *  \see PdfDocument
 *  \see PdfMemDocument
 *
 *  Example of using PdfStreamedDocument:
 *
 *  PdfStreamedDocument document("outputfile.pdf");
 *  PdfPage* page = document.CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
 *  PdfFont* font = document.CreateFont("Arial");
 *
 *  PdfPainter painter;
 *  painter.SetPage(page);
 *  painter.SetFont(font);
 *  painter.DrawText(56.69, page->GetRect().GetHeight() - 56.69, "Hello World!");
 *  painter.FinishPage();
 *
 *  document.Close();
 */
class PDFMM_API PdfStreamedDocument : public PdfDocument
{
    friend class PdfImage;
    friend class PdfElement;

public:
    /** Create a new PdfStreamedDocument.
     *  All data is written to an output device
     *  immediately.
     *
     *  \param device an output device
     *  \param version the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param encrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  \param writeMode additional options for writing the pdf
     */
    PdfStreamedDocument(PdfOutputDevice& device, PdfVersion version = PdfVersionDefault, PdfEncrypt* encrypt = nullptr, PdfWriteMode writeMode = PdfWriteModeDefault);

    /** Create a new PdfStreamedDocument.
     *  All data is written to a file immediately.
     *
     *  \param filename resulting PDF file
     *  \param version the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param encrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  \param writeMode additional options for writing the pdf
     */
    PdfStreamedDocument(const std::string_view& filename, PdfVersion version = PdfVersionDefault, PdfEncrypt* encrypt = nullptr, PdfWriteMode writeMode = PdfWriteModeDefault);

    ~PdfStreamedDocument();

    /** Close the document. The PDF file on disk is finished.
     *  No other member function of this class maybe called
     *  after calling this function.
     */
    void Close();

    PdfWriteMode GetWriteMode() const override;

    PdfVersion GetPdfVersion() const override;

    bool IsLinearized() const override;

    bool IsPrintAllowed() const override;

    bool IsEditAllowed() const override;

    bool IsCopyAllowed() const override;

    bool IsEditNotesAllowed() const override;

    bool IsFillAndSignAllowed() const override;

    bool IsAccessibilityAllowed() const override;

    bool IsDocAssemblyAllowed() const override;

    bool IsHighPrintAllowed() const override;

private:
    /** Initialize the PdfStreamedDocument with an output device
     *  \param device write to this device
     *  \param version the PDF version of the document to write.
     *                  The PDF version can only be set in the constructor
     *                  as it is the first item written to the document on disk.
     *  \param encrypt pointer to an encryption object or nullptr. If not nullptr
     *                  the PdfEncrypt object will be copied and used to encrypt the
     *                  created document.
     *  \param writeMode additional options for writing the pdf
     */
    void Init(PdfOutputDevice& device, PdfVersion version = PdfVersionDefault,
        PdfEncrypt* encrypt = nullptr, PdfWriteMode writeMode = PdfWriteModeDefault);

private:
    PdfImmediateWriter* m_Writer;
    PdfOutputDevice* m_Device;

    PdfEncrypt* m_Encrypt;

    bool m_OwnDevice; // If true m_Device is owned by this object and has to be deleted
};

};

#endif // PDF_STREAMED_DOCUMENT_H
