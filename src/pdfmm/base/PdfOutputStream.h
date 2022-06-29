/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OUTPUT_STREAM_H
#define PDF_OUTPUT_STREAM_H

#include "PdfDeclarations.h"

namespace mm {

/** An interface for writing blocks of data to
 *  a data source.
 */
class PDFMM_API PdfOutputStream
{
public:
    virtual ~PdfOutputStream();

    /** Write the character in the device
     *
     *  \param ch the character to wrte
     */
    void Write(char ch);

    /** Write the view to the PdfOutputDevice
     *
     *  \param view the view to be written
     */
    void Write(const std::string_view& view);

    /** Write data to the output stream
     *
     *  \param buffer the data is read from this buffer
     *  \param len    the size of the buffer
     */
    void Write(const char* buffer, size_t size);

    void Flush();

protected:
    virtual void writeBuffer(const char* buffer, size_t size) = 0;
    virtual void flush();
};

};

#endif // PDF_OUTPUT_STREAM_H
