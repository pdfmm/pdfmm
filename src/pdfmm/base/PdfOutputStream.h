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

class PdfOutputDevice;

/** An interface for writing blocks of data to
 *  a data source.
 */
class PDFMM_API PdfOutputStream
{
public:
    virtual ~PdfOutputStream();

    /** Write data to the output stream
     *
     *  \param buffer the data is read from this buffer
     *  \param len    the size of the buffer
     */
    void Write(const char* buffer, size_t len);

    /** Write data to the output stream
     *
     *  \param view the data is read from this buffer
     */
    void Write(const std::string_view& view);

    /** Close the PdfOutputStream.
     *  This method may throw exceptions and has to be called
     *  before the destructor to end writing.
     *
     *  No more data may be written to the output device
     *  after calling close.
     */
    virtual void Close() = 0;

protected:
    virtual void WriteImpl(const char* data, size_t len) = 0;
};

/** An output stream that writes data to a memory buffer
 *  If the buffer is to small, it will be enlarged automatically.
 */
class PDFMM_API PdfMemoryOutputStream : public PdfOutputStream
{
public:
    static constexpr size_t INITIAL_CAPACITY = 2048;

    /**
     *  Construct a new PdfMemoryOutputStream
     *  \param lInitial initial size of the buffer
     */
    PdfMemoryOutputStream(size_t initialCapacity = INITIAL_CAPACITY);

    void Close() override;

    const charbuff& GetBuffer() const { return m_Buffer; }

    /**
     *  \returns a handle to the internal buffer.
     *
     *  The internal buffer is now owned by the caller
     */
    charbuff TakeBuffer();

protected:
    void WriteImpl(const char* data, size_t len) override;

private:
    charbuff m_Buffer;
};

/** An output stream that writes to a PdfOutputDevice
 */
class PDFMM_API PdfDeviceOutputStream : public PdfOutputStream
{
public:

    /**
     *  Write to an already opened input device
     *
     *  \param device an output device
     */
    PdfDeviceOutputStream(PdfOutputDevice& device);

    void Close() override;

protected:
    void WriteImpl(const char* data, size_t len) override;

private:
    PdfOutputDevice* m_Device;
};

/**
 * An output stream that writes to a STL container
 */
template <typename TContainer>
class PdfContainerOutputStream : public PdfOutputStream
{
public:
    /**
     *  Write to an already opened input device
     *
     *  \param buffer data is written to this buffer
     */
    PdfContainerOutputStream(TContainer& container)
        : m_container(&container) { }

    void Close() override
    {
        // Do nothing
    }

protected:
    void WriteImpl(const char* data, size_t len) override
    {
        size_t currSize = m_container->size();
        m_container->resize(currSize + len);
        memcpy(m_container->data() + currSize, data, len);
    }

private:
    TContainer* m_container;
};

using PdfStringOutputStream = PdfContainerOutputStream<std::string>;
using PdfCharsOutputStream = PdfContainerOutputStream<charbuff>;
using PdfVectorOutputStream = PdfContainerOutputStream<std::vector<char>>;

};

#endif // PDF_OUTPUT_STREAM_H
