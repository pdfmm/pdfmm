/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DATA_H
#define PDF_DATA_H

#include "PdfDeclarations.h"

#include "PdfDataProvider.h"

namespace mm {

class PdfOutputDevice;

/** A datatype that allows to write arbitrary data
 *  to a PDF file. 
 *  The user of this class has to ensure that the data
 *  written to the PDF file using this class is valid data
 *  for a PDF file!
 *
 *  This class is used in pdfmm to pad PdfVariants.
 *
 */
class PDFMM_API PdfData final : public PdfDataProvider
{
public:
    PdfData();

    /**
     * Create a new PdfData object with valid PdfData
     *
     * The contained data has to be a valid value in a PDF file.
     * It will be written directly to the PDF file.
     * \param writeBeacon Shared sentinel that will updated
     *                    during writing of the document with
     *                    the current position in the stream
     *
     */
    PdfData(chars&& data, const std::shared_ptr<size_t>& writeBeacon = { });

    /**
     * Create a new PdfData object with valid PdfData
     *
     * The contained data has to be a valid value in a PDF file.
     * It will be written directly to the PDF file.
     * \param writeBeacon Shared sentinel that will updated
     *                    during writing of the document with
     *                    the current position in the stream
     *
     */
    static PdfData Create(const cspan<char>& data, const std::shared_ptr<size_t>& writeBeacon = { });

    /** Copy an existing PdfData
     *  \param rhs another PdfData to copy
     */
    PdfData(const PdfData& rhs) = default;

    void Write(PdfOutputDevice& device, PdfWriteMode writeMode, const PdfEncrypt* encrypt) const override;

    /** Copy an existing PdfData
     *  \param rhs another PdfData to copy
     *  \returns this object
     */
    PdfData& operator=(const PdfData& rhs) = default;

    PdfData& operator=(const cspan<char>& data);

    /**
     * Access the data as a std::string
     * \returns a const reference to the contained data
     */
     inline const std::string& data() const { return m_data; }

private:
    chars m_data;
    std::shared_ptr<size_t> m_writeBeacon;
};

}

#endif // PDF_DATA_H

