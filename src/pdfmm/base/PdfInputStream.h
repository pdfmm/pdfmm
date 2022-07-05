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

class OutputStream;

/** An interface for reading blocks of data from a data source.
 * It supports non-blocking read operations
 */
class PDFMM_API InputStream
{
public:
    virtual ~InputStream();

    // REMOVE-ME
    size_t Read(char* buffer, size_t size);

    /** Read data from the device
     * \param buffer a pointer to the data buffer
     * \param size length of the output buffer
     * \param eof stream reached EOF during the read
     * \returns Number of read bytes
     */
    size_t Read(char* buffer, size_t size, bool& eof);

    /** Get next char from stream.
     * \returns the next character from the stream
     */
    char ReadChar();

    /** Get next char from stream.
     * \param ch the read character
     * \returns false if EOF
     */
    bool Read(char& ch);

    void CopyTo(OutputStream& stream);

protected:
    static size_t ReadBuffer(InputStream& stream, char* buffer, size_t size, bool& eof);
    static bool ReadChar(InputStream& stream, char& ch);

protected:
    /** Read a buffer from the stream
     * /param eof true if the stream reached eof during read
     * /returns number of read bytes
     */
    virtual size_t readBuffer(char* buffer, size_t size, bool& eof) = 0;

    /** Read the next char in stream.
     *  /returns true if success, false if EOF
     */
    virtual bool readChar(char& ch);

    /** Optional checks before reading
     * By default does nothing
     */
    virtual void checkRead() const;
};

};

#endif // PDF_INPUT_STREAM_H
