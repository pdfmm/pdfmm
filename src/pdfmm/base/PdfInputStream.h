/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INPUT_STREAM_H
#define PDF_INPUT_STREAM_H

#include <fstream>

#include "PdfDefines.h"

namespace mm {

class PdfInputDevice;

/** An interface for reading blocks of data from an
 *  a data source.
 */
class PDFMM_API PdfInputStream
{
public:
    PdfInputStream();
    virtual ~PdfInputStream() { };

    /** Read data from the input stream
     *
     *  \param buffer    the data will be stored into this buffer
     *  \param len       the size of the buffer and number of bytes
     *                    that will be read
     *  \param eof        true if the stream reached eof
     *
     *  \returns the number of bytes read
     */
    size_t Read(char* buffer, size_t len, bool& eof);

public:
    inline bool Eof() const { return m_eof; }

protected:
    virtual size_t ReadImpl(char* buffer, size_t len, bool& eof) = 0;

private:
    bool m_eof;
};

/** An input stream that reads data from a file
 */
class PDFMM_API PdfFileInputStream : public PdfInputStream
{
public:
    /** Open a file for reading data
     *
     *  \param filename the filename of the file to read
     */
    PdfFileInputStream(const std::string_view& filename);

    ~PdfFileInputStream();

protected:
    size_t ReadImpl(char* buffer, size_t len, bool& eof) override;

private:
    std::ifstream m_stream;
};

/** An input stream that reads data from a memory buffer
 */
class PDFMM_API PdfMemoryInputStream : public PdfInputStream
{
public:
    /** Open a file for reading data
     *
     *  \param buffer buffer to read from
     */
    PdfMemoryInputStream(const std::string_view& buffer);
    ~PdfMemoryInputStream();

protected:
    size_t ReadImpl(char* buffer, size_t len, bool& eof) override;

private:
    const char* m_Buffer;
    size_t m_BufferLen;
};

/** An input stream that reads data from an input device
 */
class PDFMM_API PdfDeviceInputStream : public PdfInputStream
{
public:
    /**
     *  Read from an already opened input device
     *
     *  \param device an input device
     */
    PdfDeviceInputStream(PdfInputDevice& device);

protected:
    size_t ReadImpl(char* buffer, size_t len, bool& eof) override;

private:
    PdfInputDevice* m_Device;
};

};

#endif // PDF_INPUT_STREAM_H
