/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INPUT_DEVICE_H
#define PDF_INPUT_DEVICE_H

#include <istream>
#include <fstream>

#include "PdfStreamDeviceBase.h"
#include "PdfInputStream.h"

namespace mm {

/** This class represents an input device which operates
 * either on a file, a buffer in memory or any arbitrary std::istream
 * It optionally supports peeking
 */
class PDFMM_API InputStreamDevice : virtual public StreamDeviceBase, public InputStream
{
protected:
    InputStreamDevice();
    InputStreamDevice(bool init);

public:
    /** Peek at next char in stream.
     * /returns the next char in the stream, or -1 if EOF
     */
    // REMOVE-ME
    int Peek() const;

    /** Peek at next char in stream.
     * /returns true if success, false if EOF
     */
    bool Peek(char& ch) const;

protected:
    /** Peek at next char in stream.
     *  /returns true if success, false if EOF
     */
    virtual bool peek(char& ch) const = 0;

    virtual size_t readBufferImpl(char* buffer, size_t size, bool& eof) = 0;

    /** Read the next char in stream.
     *  /returns true if success, EOF if false
     */
    virtual bool readCharImpl(char& ch) = 0;

private:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override final;
    bool readChar(char& ch) override final;
};

};

#endif // PDF_INPUT_DEVICE_H
