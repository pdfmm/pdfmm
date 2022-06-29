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
#include "PdfInputStream.h"

namespace mm {

/** This class represents an input device which operates
 * either on a file, a buffer in memory or any arbitrary std::istream
 * Differently from PdfInputStream all its operation are blocking
 * and it can be optionally seekable
 */
class PDFMM_API PdfInputDevice : public PdfInputStream
{
public:
    /** Destruct the PdfInputDevice object and close any open files.
     */
    virtual ~PdfInputDevice();

    /** Get next char from stream.
     * \returns the next character from the stream
     */
    char ReadChar();

    /** Get next char from stream.
     * \param ch the read character
     * \returns false if EOF
     */
    bool Read(char& ch);

    /** Read a certain number of bytes from the input device.
     *
     *  \param buffer store bytes in this buffer.
     *      The buffer has to be large enough.
     *  \param size number of bytes to read.
     *  \returns the number of bytes that have been read.
     *      0 if the device reached eof
     */
    size_t Read(char* buffer, size_t size);

    /** Close the input device.
     *  No further operations may be performed on this device
     *  after calling this function.
     */
    void Close();

    /** Get the current position in file.
     *  /returns the current position in the file
     */
    virtual size_t GetPosition() = 0;

    /** Peek at next char in stream.
     *  /returns the next char in the stream
     */
    virtual int Peek() = 0;

    /** Seek the device to the position offset from the beginning
     *  \param off from the beginning of the file
     *  \param dir where to start (start, cur, end)
     *
     *  A non-seekable input device will throw an InvalidDeviceOperation.
     */
    void Seek(std::streamoff off, std::ios_base::seekdir dir = std::ios_base::beg);

    /**
     * \return True if the stream is at EOF
     */
    virtual bool Eof() const = 0;

    /**
     * \return True if the stream is seekable
     */
    virtual bool CanSeek() const;

protected:
    virtual void seek(std::streamoff off, std::ios_base::seekdir dir);
    virtual void close();

    /**
     * \returns the number of bytes that have been read.
     * 0 if the device reached eof
     */
    virtual size_t readBufferImpl(char* buffer, size_t size) = 0;

    /**
     * \returns true if success, false if EOF
     */
    virtual bool readCharImpl(char& ch);

protected:
    size_t readBuffer(char* buffer, size_t size, bool& eof) override final;
    bool readChar(char& ch, bool& eof) override final;
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

    virtual size_t GetPosition() override;

    int Peek() override;

    bool Eof() const override;

    bool CanSeek() const override;

protected:
    void seek(std::streamoff off, std::ios_base::seekdir dir) override;
    size_t readBufferImpl(char* buffer, size_t size) override;
    bool readCharImpl(char& ch) override;

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

class PDFMM_API PdfMemoryInputDevice final : public PdfInputDevice
{
public:
    /** Construct a new PdfInputDevice that reads all data from a memory buffer.
     *  The buffer is temporarily binded
     */
    PdfMemoryInputDevice(const char* buffer, size_t size);
    PdfMemoryInputDevice(const bufferview& buffer);
    PdfMemoryInputDevice(const std::string_view& view);
    PdfMemoryInputDevice(const std::string& str);
    PdfMemoryInputDevice(const char* str);

    virtual size_t GetPosition() override;

    int Peek() override;

    bool Eof() const override;

    bool CanSeek() const override;

protected:
    void seek(std::streamoff off, std::ios_base::seekdir dir) override;
    size_t readBufferImpl(char* buffer, size_t size) override;
    bool readCharImpl(char& ch) override;

private:
    PdfMemoryInputDevice(std::nullptr_t) = delete;

private:
    const char* m_buffer;
    size_t m_Length;
    size_t m_Position;
};

};

#endif // PDF_INPUT_DEVICE_H
