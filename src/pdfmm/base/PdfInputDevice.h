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

/** This class represents an input device
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

    void checkRead() const override;
};

};

#endif // PDF_INPUT_DEVICE_H
