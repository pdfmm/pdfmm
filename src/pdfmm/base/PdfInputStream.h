/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INPUT_STREAM_H
#define PDF_INPUT_STREAM_H

#include "PdfDeclarations.h"

namespace mm {

class PdfOutputStream;

/** An interface for reading blocks of data from a data source.
 * It supports non-blocking read operations
 */
class PDFMM_API PdfInputStream
{
public:
    virtual ~PdfInputStream();

    /** Get next char from stream.
     *
     * \returns true if success
     */
    bool Read(char& ch, bool& eof);

    /** Read a certain number of bytes from the input device.
     *
     * \param buffer store bytes in this buffer.
     *      The buffer has to be large enough.
     * \param size number of bytes to read
     * \param eof true if eof
     * \returns number of bytes read
     */
    size_t Read(char* buffer, size_t size, bool& eof);

    void CopyTo(PdfOutputStream& stream);

protected:
    static size_t ReadBuffer(PdfInputStream& stream, char* buffer, size_t size, bool& eof);
    static bool ReadChar(PdfInputStream& stream, char& ch, bool& eof);

protected:
    virtual size_t readBuffer(char* buffer, size_t size, bool& eof) = 0;
    // Allow an optmized version for chars
    virtual bool readChar(char& ch, bool& eof);
};

};

#endif // PDF_INPUT_STREAM_H
