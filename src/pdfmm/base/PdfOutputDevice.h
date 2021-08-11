/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OUTPUT_DEVICE_H
#define PDF_OUTPUT_DEVICE_H

#include "PdfDefines.h"

#include <string_view>
#include <ostream>
#include <iostream>
#include <fstream>

namespace mm {

// TODO: Remove use of C style variadic functions
/** This class provides an output device which operates
 *  either on a file or on a buffer in memory.
 *  Additionally it can count the bytes written to the device.
 *
 *  This class is suitable for inheritance to provide output
 *  devices of your own for pdfmm.
 *  Just override the required virtual methods.
 */
class PDFMM_API PdfOutputDevice
{
public:
    PdfOutputDevice();

    /** Destruct the PdfOutputDevice object and close any open files.
     */
    virtual ~PdfOutputDevice();

    /** Write to the PdfOutputDevice. Usage is as the usage of printf.
     *
     *  WARNING: Do not use this for doubles or floating point values
     *           as the output might depend on the current locale.
     *
     *  \param format a format string as you would use it with printf
     *
     *  \see Write
     */
    void Print(const char* format, ...);

    void Print(const char* format, va_list args);

    /** Write to the PdfOutputDevice. Usage is as the usage of printf.
     *
     *  WARNING: Do not use this for doubles or floating point values
     *           as the output might depend on the current locale.
     *
     *  \param format a format string as you would use it with printf
     *  \param size length of the format string in bytes when written
     *  \param args variable argument list
     *
     *  \see Write
     */
    void Print(const char* format, size_t size, va_list args);

    /** Write data to the buffer. Use this call instead of Print if you
     *  want to write binary data to the PdfOutputDevice.
     *
     *  \param buffer a pointer to the data buffer
     *  \param len write len bytes of buffer to the PdfOutputDevice
     *
     *  \see Print
     */
    void Write(const char* buffer, size_t len);

    /** Write the character in the device
     *
     *  \param ch the character to wrte
     */
    void Put(char ch);

    /** Read data from the device
     *  \param buffer a pointer to the data buffer
     *  \param len length of the output buffer
     *  \returns Number of read bytes. Return 0 if EOF
     */
    size_t Read(char* buffer, size_t len);

    /** Seek the device to the position offset from the beginning
     *  \param offset from the beginning of the file
     */
    void Seek(size_t offset);

    /** The number of bytes written to this object.
     *  \returns the number of bytes written to this object.
     *
     *  \see Init
     */
    inline size_t GetLength() const { return m_Length; }

    /** Get the current offset from the beginning of the file.
     *  \return the offset form the beginning of the file.
     */
    inline size_t Tell() const { return m_Position; }

    /** Flush the output files buffer to disk if this devices
     *  operates on a disk.
     */
    void Flush();

protected:
    void SetLength(size_t length);
    void SetPosition(size_t position);
    virtual void print(const char* format, size_t size, va_list args) = 0;
    virtual void write(const char* buffer, size_t len) = 0;
    virtual void seek(size_t offset) = 0;
    virtual size_t read(char* buffer, size_t len) = 0;
    virtual void flush() = 0;

private:
    size_t m_Length;
    size_t m_Position;
};

class PDFMM_API PdfStreamOutputDevice : public PdfOutputDevice
{
public:
    /** Construct a new PdfOutputDevice that writes all data to a std::ostream.
     *
     *  WARNING: pdfmm will change the stream's locale.  It will be restored when
     *  the PdfOutputStream controlling the stream is destroyed.
     *
     *  \param pOutStream write to this std::ostream
     */
    PdfStreamOutputDevice(std::ostream& stream);

    /** Construct a new PdfOutputDevice that writes all data to a std::iostream
     *  and reads from it as well.
     *
     *  WARNING: pdfmm will change the stream's locale.  It will be restored when
     *  the PdfOutputStream controlling the stream is destroyed.
     *
     *  \param stream read/write from/to this std::iostream
     */
    PdfStreamOutputDevice(std::iostream& stream);

    ~PdfStreamOutputDevice();

protected:
    PdfStreamOutputDevice(std::ostream* out, std::istream* in, bool streamOwned);

    void print(const char* format, size_t size, va_list args) override;
    void write(const char* buffer, size_t len) override;
    void seek(size_t offset) override;
    size_t read(char* buffer, size_t len) override;
    void flush() override;

    std::ostream& GetStream() { return *m_Stream; }
    std::istream& GetReadStream() { return *m_ReadStream; }

private:
    std::ostream* m_Stream;
    std::istream* m_ReadStream;
    bool m_StreamOwned;
    std::string  m_printBuffer;
};

class PDFMM_API PdfFileOutputDevice final : public PdfStreamOutputDevice
{
public:
    /** Construct a new PdfOutputDevice that writes all data to a file.
     *
     *  \param filename path to a file that will be opened and all data
     *                     is written to this file.
     *  \param truncate whether to truncate the file after open. This is useful
     *                   for incremental updates, to not truncate the file when
     *                   writing to the same file as the loaded. Default is true.
     *
     *  When the truncate is false, the device is automatically positioned
     *  to the end of the file.
     */
    PdfFileOutputDevice(const std::string_view& filename, bool truncate = true);

private:
    PdfFileOutputDevice(std::fstream* stream);
    std::fstream* getFileStream(const std::string_view& filename, bool truncate);
};

class PDFMM_API PdfMemoryOutputDevice final : public PdfOutputDevice
{
public:
    /** Construct a new PdfOutputDevice that writes all data to a memory buffer.
     *  The buffer will not be owned by this object and has to be allocated before.
     *
     *  \param buffer a buffer in memory
     *  \param len the length of the buffer in memory
     */
    PdfMemoryOutputDevice(char* buffer, size_t len);

protected:
    void print(const char* format, size_t size, va_list args) override;
    void write(const char* buffer, size_t len) override;
    void seek(size_t offset) override;
    size_t read(char* buffer, size_t len) override;
    void flush() override;

private:
    char* m_Buffer;
    size_t m_BufferLen;
};

class PDFMM_API PdfStringOutputDevice final : public PdfOutputDevice
{
public:
    PdfStringOutputDevice(std::string& str);

protected:
    void print(const char* format, size_t size, va_list args) override;
    void write(const char* buffer, size_t len) override;
    void seek(size_t offset) override;
    size_t read(char* buffer, size_t len) override;
    void flush() override;

private:
    std::string* m_str;
};

/**
 * An output device that does nothing
 */
class PDFMM_API PdfNullOutputDevice : public PdfOutputDevice
{
public:
    PdfNullOutputDevice();

protected:
    void print(const char* format, size_t size, va_list args) override;
    void write(const char* buffer, size_t len) override;
    void seek(size_t offset) override;
    size_t read(char* buffer, size_t len) override;
    void flush() override;
};

};

#endif // PDF_OUTPUT_DEVICE_H
