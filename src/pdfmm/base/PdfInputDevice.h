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

#include "PdfDeclarations.h"

namespace mm {

class PdfObjectStream;

// TODO: Refactor, PdfInputDevice should be an interface
// with, implementing classes like PdfMemInputDevice or PdfFileInputDevice
/** This class provides an Input device which operates
 *  either on a file, a buffer in memory or any arbitrary std::istream
 *
 *  This class is suitable for inheritance to provide input
 *  devices of your own for pdfmm.
 *  Just override the required virtual methods.
 */
class PDFMM_API PdfInputDevice
{
protected:
    PdfInputDevice();

public:
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
    virtual size_t Tell() = 0;

    /** Get next char from stream.
     *  \returns the next character from the stream
     */
    int GetChar();

    /** Get next char from stream.
     *  \returns the next character from the stream
     */
    virtual bool TryGetChar(char& ch) = 0;

    /** Peek at next char in stream.
     *  /returns the next char in the stream
     */
    virtual int Look() = 0;

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
    virtual size_t Read(char* buffer, size_t size) = 0;

    /**
     * \return True if the stream is at EOF
     */
    virtual bool Eof() const = 0;

    /**
     * \return True if the stream is seekable
     */
    virtual bool IsSeekable() const = 0;

protected:
    virtual void seek(std::streamoff off, std::ios_base::seekdir dir);
};

class PDFMM_API PdfStreamInputDevice : public PdfInputDevice
{
protected:
    PdfStreamInputDevice();

public:
    /** Construct a new PdfInputDevice that reads all data from a file.
     *
     *  \param filename path to a file that will be opened and all data
     *                     is read from this file.
     */
    PdfStreamInputDevice(std::istream& stream);

    ~PdfStreamInputDevice();

    virtual size_t Tell() override;

    virtual bool TryGetChar(char& ch) override;

    int Look() override;

    size_t Read(char* buffer, size_t size) override;

    bool Eof() const override;

    bool IsSeekable() const override { return true; }

protected:
    void seek(std::streamoff off, std::ios_base::seekdir dir) override;

protected:
    void SetStream(std::istream* stream, bool streamOwned);

private:
    std::istream* m_Stream;
    bool m_StreamOwned;
};

class PDFMM_API PdfFileInputDevice final : public PdfStreamInputDevice
{
public:
    /** Construct a new PdfInputDevice that reads all data from a file.
     *
     *  \param filename path to a file that will be opened and all data
     *                     is read from this file.
     */
    PdfFileInputDevice(const std::string_view& filename);
};

class PDFMM_API PdfMemoryInputDevice final : public PdfStreamInputDevice
{
public:
    /** Construct a new PdfInputDevice that reads all data from a memory buffer.
     *  The buffer is temporarily binded
     */
    PdfMemoryInputDevice(const char* buffer, size_t len);
    PdfMemoryInputDevice(const bufferview& buffer);
    PdfMemoryInputDevice(const std::string_view& view);
    PdfMemoryInputDevice(const std::string& str);
    PdfMemoryInputDevice(const char* str);
private:
    PdfMemoryInputDevice(std::nullptr_t) = delete;
};

class PDFMM_API PdfObjectStreamInputDevice final : public PdfStreamInputDevice
{
public:
    PdfObjectStreamInputDevice(const PdfObjectStream& stream);
private:
    charbuff m_buffer;
};

};

#endif // PDF_INPUT_DEVICE_H
