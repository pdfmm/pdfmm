/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OUTPUT_DEVICE_H
#define PDF_OUTPUT_DEVICE_H

#include <string_view>
#include <ostream>
#include <iostream>

#include "PdfDefines.h"
#include "PdfLocale.h"
#include "PdfRefCountedBuffer.h"

namespace mm {

// TODO1: Evaluate if this class should implement PdfInputStream/PdfOutputStream
// TODO2: Remove use of C style variadic functions
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
    /** Construct a new PdfOutputDevice that does not write any data. Only the length
     *  of the data is counted.
     *
     */
    PdfOutputDevice();

    /** Construct a new PdfOutputDevice that writes all data to a file.
     *
     *  \param filename path to a file that will be opened and all data
     *                     is written to this file.
     *  \param bTruncate whether to truncate the file after open. This is useful
     *                   for incremental updates, to not truncate the file when
     *                   writing to the same file as the loaded. Default is true.
     *
     *  When the bTruncate is false, the device is automatically positioned
     *  to the end of the file.
     */
    PdfOutputDevice(const std::string_view& filename, bool truncate = true);

    /** Construct a new PdfOutputDevice that writes all data to a memory buffer.
     *  The buffer will not be owned by this object and has to be allocated before.
     *
     *  \param buffer a buffer in memory
     *  \param len the length of the buffer in memory
     */
    PdfOutputDevice(char* buffer, size_t len);

    /** Construct a new PdfOutputDevice that writes all data to a std::ostream.
     *
     *  WARNING: pdfmm will change the stream's locale.  It will be restored when
     *  the PdfOutputStream controlling the stream is destroyed.
     *
     *  \param pOutStream write to this std::ostream
     */
    PdfOutputDevice(std::ostream& stream);

    /** Construct a new PdfOutputDevice that writes all data to a PdfRefCountedBuffer.
     *  This output device has the advantage that the PdfRefCountedBuffer will resize itself
     *  if more memory is needed to hold all data.
     *
     *  \param pOutBuffer write to this PdfRefCountedBuffer
     *
     *  \see PdfRefCountedBuffer
     */
    PdfOutputDevice(PdfRefCountedBuffer& buffer);

    /** Construct a new PdfOutputDevice that writes all data to a std::iostream
     *  and reads from it as well.
     *
     *  WARNING: pdfmm will change the stream's locale.  It will be restored when
     *  the PdfOutputStream controlling the stream is destroyed.
     *
     *  \param stream read/write from/to this std::iostream
     */
    PdfOutputDevice(std::iostream& stream);

    /** Destruct the PdfOutputDevice object and close any open files.
     */
    virtual ~PdfOutputDevice();

    /** The number of bytes written to this object.
     *  \returns the number of bytes written to this object.
     *
     *  \see Init
     */
    inline size_t GetLength() const { return m_Length; }

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
    void PrintV(const char* format, size_t size, va_list args);

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

    /** Get the current offset from the beginning of the file.
     *  \return the offset form the beginning of the file.
     */
    inline size_t Tell() const { return m_Position; }

    /** Flush the output files buffer to disk if this devices
     *  operates on a disk.
     */
    void Flush();

private:
    /** Initialize all private members
     */
    void Init();

private:
    size_t m_Length;
    char* m_Buffer;
    size_t m_BufferLen;

    std::ostream* m_Stream;
    std::istream* m_ReadStream;
    bool m_StreamOwned;

    PdfRefCountedBuffer* m_RefCountedBuffer;
    size_t m_Position;
    std::string  m_printBuffer;
};

};

#endif // PDF_OUTPUT_DEVICE_H
