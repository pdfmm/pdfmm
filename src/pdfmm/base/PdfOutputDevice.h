/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OUTPUT_DEVICE_H
#define PDF_OUTPUT_DEVICE_H

#include "PdfDeclarations.h"

#include <ostream>
#include <fstream>

#include "PdfOutputStream.h"

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
class PDFMM_API PdfOutputDevice : public PdfOutputStream
{
public:
    /** Destruct the PdfOutputDevice object and close any open files.
     */
    virtual ~PdfOutputDevice();

    /** Read data from the device
     *  \param buffer a pointer to the data buffer
     *  \param size length of the output buffer
     *  \returns Number of read bytes. Return 0 if EOF
     */
    size_t Read(char* buffer, size_t size);

    /** Seek the device to the position offset from the beginning
     *  \param offset from the beginning of the file
     */
    void Seek(size_t offset);

    /** Flush the output files buffer to disk if this devices
     *  operates on a disk.
     */
    void Flush();

    void Close();

public:
    /**
     * \return True if the stream is at EOF
     */
    virtual bool Eof() const = 0;

    /** The number of bytes written to this object.
     *  \returns the number of bytes written to this object.
     *
     *  \see Init
     */
    virtual size_t GetLength() const = 0;

    /** Get the current offset from the beginning of the file.
     *  \return the offset form the beginning of the file.
     */
    virtual size_t GetPosition() const = 0;

    virtual bool CanSeek() const;

protected:
    virtual size_t readBuffer(char* buffer, size_t size) = 0;
    virtual void seek(size_t offset) = 0;
    virtual void close();
};

class PDFMM_API PdfStreamOutputDevice : public PdfOutputDevice
{
public:
    /** Construct a new PdfOutputDevice that writes all data to a std::ostream.
     *
     *  \param stream write to this std::ostream
     */
    PdfStreamOutputDevice(std::ostream& stream);

    PdfStreamOutputDevice(std::istream& stream);

    /** Construct a new PdfOutputDevice that writes all data to a std::iostream
     *  and reads from it as well.
     *  \param stream read/write from/to this std::iostream
     */
    PdfStreamOutputDevice(std::iostream& stream);

    ~PdfStreamOutputDevice();

public:
    size_t GetLength() const override;

    size_t GetPosition() const override;

    bool CanSeek() const override;

    bool Eof() const override;

protected:
    PdfStreamOutputDevice(std::ios_base* stream, bool streamOwned);
    void writeBuffer(const char* buffer, size_t size) override;
    size_t readBuffer(char* buffer, size_t size) override;
    void seek(size_t offset) override;
    void flush() override;

    inline std::ios_base& GetStream() { return *m_Stream; }

private:
    std::ios_base* m_Stream;
    std::istream* m_istream;
    std::ostream* m_ostream;
    bool m_StreamOwned;
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

protected:
    void close() override;

private:
    std::fstream* getFileStream(const std::string_view& filename, bool truncate);
};

template <typename TContainer>
class PdfContainerOutputDevice final : public PdfOutputDevice
{
public:
    PdfContainerOutputDevice(TContainer& container) :
        m_container(&container), m_Position(container.size()) { }

public:
    size_t GetLength() const override { return m_container->size(); }

    size_t GetPosition() const override { return m_Position; }

    bool CanSeek() const override { return true; }

    bool Eof() const override { return m_Position == m_container->size(); }

protected:
    void writeBuffer(const char* buffer, size_t size) override
    {
        if (m_Position + size > m_container->size())
            m_container->resize(m_Position + size);

        std::memcpy(m_container->data() + m_Position, buffer, size);
        m_Position += size;
    }

    size_t readBuffer(char* buffer, size_t size) override
    {
        size_t readCount = std::min(size, m_container->size() - m_Position);
        std::memcpy(buffer, m_container->data() + m_Position, readCount);
        m_Position += readCount;
        return readCount;
    }

    void seek(size_t offset) override
    {
        if (offset >= m_container->size())
            throw PdfError(PdfErrorCode::InvalidDeviceOperation, __FILE__, __LINE__, "Can't seek past container");

        m_Position = offset;
    }

    void flush() override
    {
        // Do nothing
    }

    void close() override
    {
        // Do nothing
    }

private:
    TContainer* m_container;
    size_t m_Position;
};

using PdfStringOutputDevice = PdfContainerOutputDevice<std::string>;
using PdfVectorOutputDevice = PdfContainerOutputDevice<std::vector<char>>;
using PdfCharsOutputDevice = PdfContainerOutputDevice<charbuff>;

/**
 * An output device that does nothing
 */
class PDFMM_API PdfNullOutputDevice final : public PdfOutputDevice
{
public:
    PdfNullOutputDevice();

public:
    size_t GetLength() const override;

    size_t GetPosition() const override;

    bool Eof() const override;

protected:
    void writeBuffer(const char* buffer, size_t size) override;
    size_t readBuffer(char* buffer, size_t size) override;
    void seek(size_t offset) override;

private:
    size_t m_Length;
    size_t m_Position;
};

};

#endif // PDF_OUTPUT_DEVICE_H
