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

#include "PdfDefines.h"
#include "PdfLocale.h"

namespace PoDoFo {

class PdfStream;

// TODO: Refactor, PdfInputDevice should be an interface
// with, implementing classes like PdfMemInputDevice or PdfFileInputDevice
/** This class provides an Input device which operates
 *  either on a file, a buffer in memory or any arbitrary std::istream
 *
 *  This class is suitable for inheritance to provide input
 *  devices of your own for PoDoFo.
 *  Just override the required virtual methods.
 */
class PODOFO_API PdfInputDevice
{
protected:
    PdfInputDevice();

public:
    // TODO: Move it to a PdfFileInputDevice
    /** Construct a new PdfInputDevice that reads all data from a file.
     *
     *  \param pszFilename path to a file that will be opened and all data
     *                     is read from this file.
     */
    explicit PdfInputDevice(const std::string_view& filename);

    // TODO: Move it to a PdfMemInputDevice
    /** Construct a new PdfInputDevice that reads all data from a memory buffer.
     *  The buffer will not be owned by this object - it is COPIED.
     *
     *  \param pBuffer a buffer in memory
     *  \param lLen the length of the buffer in memory
     */
    PdfInputDevice(const char* buffer, size_t len);

    /** Construct a new PdfInputDevice that reads all data from a std::istream.
     *
     *  \param pInStream read from this std::istream
     */
    explicit PdfInputDevice(std::istream& stream);

    // TODO: Move it to a PdfMemInputDevice
    explicit PdfInputDevice(const PdfStream& stream);

    /** Destruct the PdfInputDevice object and close any open files.
     */
    virtual ~PdfInputDevice();

    /** Close the input device.
     *  No further operations may be performed on this device
     *  after calling this function.
     */
    virtual void Close();

    /** Get the current position in file.
     *  /returns the current position in the file
     */
    virtual size_t Tell();

    /** Get next char from stream.
     *  \returns the next character from the stream
     */
    int GetChar();

    /** Get next char from stream.
     *  \returns the next character from the stream
     */
    virtual bool TryGetChar(char& ch);

    /** Peek at next char in stream.
     *  /returns the next char in the stream
     */
    virtual int Look();

    /** Seek the device to the position offset from the beginning
     *  \param off from the beginning of the file
     *  \param dir where to start (start, cur, end)
     *
     *  A non-seekable input device will throw an InvalidDeviceOperation.
     */
    void Seek(std::streamoff off, std::ios_base::seekdir dir = std::ios_base::beg);

    /** Read a certain number of bytes from the input device.
     *
     *  \param buffer store bytes in this buffer.
     *      The buffer has to be large enough.
     *  \param size number of bytes to read.
     *  \returns the number of bytes that have been read.
     *      0 if the device reached eof
     */
    virtual size_t Read(char* buffer, size_t size);

    /**
     * \return True if the stream is at EOF
     */
    virtual bool Eof() const;

    /**
     * \return True if the stream is seekable
     */
    virtual bool IsSeekable() const { return true; }

protected:
    void seek(std::streamoff off, std::ios_base::seekdir dir);

private:
    std::istream* m_Stream;
    bool m_StreamOwned;
};

};

#endif // PDF_INPUT_DEVICE_H
