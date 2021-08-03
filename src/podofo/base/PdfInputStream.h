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

namespace PoDoFo {

class PdfInputDevice;

/** An interface for reading blocks of data from an
 *  a data source.
 */
class PODOFO_API PdfInputStream
{
public:
    PdfInputStream();
    virtual ~PdfInputStream() { };

    /** Read data from the input stream
     *
     *  \param pBuffer    the data will be stored into this buffer
     *  \param lLen       the size of the buffer and number of bytes
     *                    that will be read
     *  \param eof        true if the stream reached eof
     *
     *  \returns the number of bytes read
     */
    size_t Read(char* pBuffer, size_t lLen, bool& eof);

public:
    inline bool Eof() const { return m_eof; }

protected:
    virtual size_t ReadImpl(char* pBuffer, size_t lLen, bool& eof) = 0;

private:
    bool m_eof;
};

/** An input stream that reads data from a file
 */
class PODOFO_API PdfFileInputStream : public PdfInputStream
{
public:
    /** Open a file for reading data
     *
     *  \param pszFilename the filename of the file to read
     */
    PdfFileInputStream(const std::string_view& filename);

    ~PdfFileInputStream();

protected:
    size_t ReadImpl(char* pBuffer, size_t lLen, bool& eof) override;

private:
    std::ifstream m_stream;
};

/** An input stream that reads data from a memory buffer
 */
class PODOFO_API PdfMemoryInputStream : public PdfInputStream
{
public:
    /** Open a file for reading data
     *
     *  \param pBuffer buffer to read from
     *  \param lBufferLen length of the buffer
     */
    PdfMemoryInputStream(const char* pBuffer, size_t lBufferLen);
    ~PdfMemoryInputStream();

protected:
    size_t ReadImpl(char* pBuffer, size_t lLen, bool& eof) override;

private:
    const char* m_pBuffer;
    size_t m_lBufferLen;
};

/** An input stream that reads data from an input device
 */
class PODOFO_API PdfDeviceInputStream : public PdfInputStream
{
public:
    /**
     *  Read from an already opened input device
     *
     *  \param pDevice an input device
     */
    PdfDeviceInputStream(PdfInputDevice* pDevice);

protected:
    size_t ReadImpl(char* pBuffer, size_t lLen, bool& eof) override;

private:
    PdfInputDevice* m_pDevice;
};

};

#endif // PDF_INPUT_STREAM_H
