/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_OUTPUT_DEVICE_H_
#define _PDF_OUTPUT_DEVICE_H_

#include <string_view>
#include <ostream>
#include <iostream>

#include "PdfDefines.h"
#include "PdfLocale.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

// TODO1: Evaluate if this class should implement PdfInputStream/PdfOutputStream
// TODO2: Remove use of C style variadic functions
/** This class provides an output device which operates 
 *  either on a file or on a buffer in memory.
 *  Additionally it can count the bytes written to the device.
 *
 *  This class is suitable for inheritance to provide output 
 *  devices of your own for PoDoFo.
 *  Just overide the required virtual methods.
 */
class PODOFO_API PdfOutputDevice
{
public:
    /** Construct a new PdfOutputDevice that does not write any data. Only the length
     *  of the data is counted.
     *
     */
    PdfOutputDevice();

    /** Construct a new PdfOutputDevice that writes all data to a file.
     *
     *  \param pszFilename path to a file that will be opened and all data
     *                     is written to this file.
     *  \param bTruncate whether to truncate the file after open. This is useful
     *                   for incremental updates, to not truncate the file when
     *                   writing to the same file as the loaded. Default is true.
     *
     *  When the bTruncate is false, the device is automatically positioned
     *  to the end of the file.
     */
    PdfOutputDevice( const std::string_view &filename, bool bTruncate = true );

    /** Construct a new PdfOutputDevice that writes all data to a memory buffer.
     *  The buffer will not be owned by this object and has to be allocated before.
     *
     *  \param pBuffer a buffer in memory
     *  \param lLen the length of the buffer in memory
     */
    PdfOutputDevice( char* pBuffer, size_t lLen );

    /** Construct a new PdfOutputDevice that writes all data to a std::ostream.
     *
     *  WARNING: PoDoFo will change the stream's locale.  It will be restored when
     *  the PdfOutputStream controlling the stream is destroyed.
     *
     *  \param pOutStream write to this std::ostream
     */
    PdfOutputDevice( const std::ostream* pOutStream );

    /** Construct a new PdfOutputDevice that writes all data to a PdfRefCountedBuffer.
     *  This output device has the advantage that the PdfRefCountedBuffer will resize itself
     *  if more memory is needed to hold all data.
     *
     *  \param pOutBuffer write to this PdfRefCountedBuffer
     *
     *  \see PdfRefCountedBuffer
     */
    PdfOutputDevice( PdfRefCountedBuffer* pOutBuffer );

    /** Construct a new PdfOutputDevice that writes all data to a std::iostream
     *  and reads from it as well.
     *
     *  WARNING: PoDoFo will change the stream's locale.  It will be restored when
     *  the PdfOutputStream controlling the stream is destroyed.
     *
     *  \param pStream read/write from/to this std::iostream
     */
    PdfOutputDevice( std::iostream* pStream );

    /** Destruct the PdfOutputDevice object and close any open files.
     */
    virtual ~PdfOutputDevice();

    /** The number of bytes written to this object.
     *  \returns the number of bytes written to this object.
     *  
     *  \see Init
     */
    inline size_t GetLength() const { return m_ulLength; }

    /** Write to the PdfOutputDevice. Usage is as the usage of printf.
     * 
     *  WARNING: Do not use this for doubles or floating point values
     *           as the output might depend on the current locale.
     *
     *  \param pszFormat a format string as you would use it with printf
     *
     *  \see Write
     */
    void Print( const char* pszFormat, ... );

    void Print(const char* pszFormat, va_list args);

    /** Write to the PdfOutputDevice. Usage is as the usage of printf.
     * 
     *  WARNING: Do not use this for doubles or floating point values
     *           as the output might depend on the current locale.
     *
     *  \param pszFormat a format string as you would use it with printf
     *  \param lBytes length of the format string in bytes when written
     *  \param argptr variable argument list
     *
     *  \see Write
     */
    void PrintV( const char* pszFormat, size_t lBytes, va_list argptr );

    /** Write data to the buffer. Use this call instead of Print if you 
     *  want to write binary data to the PdfOutputDevice.
     *
     *  \param pBuffer a pointer to the data buffer
     *  \param lLen write lLen bytes of pBuffer to the PdfOutputDevice
     * 
     *  \see Print
     */
    void Write( const char* pBuffer, size_t lLen );

    /** Read data from the device
     *  \param pBuffer a pointer to the data buffer
     *  \param lLen length of the output buffer
     *  \returns Number of read bytes. Return 0 if EOF
     */
    size_t Read( char* pBuffer, size_t lLen );

    /** Seek the device to the position offset from the begining
     *  \param offset from the beginning of the file
     */
    void Seek( size_t offset );

    /** Get the current offset from the beginning of the file.
     *  \return the offset form the beginning of the file.
     */
    inline size_t Tell() const { return m_ulPosition; }

    /** Flush the output files buffer to disk if this devices
     *  operates on a disk.
     */
    void Flush();

private: 
    /** Initialize all private members
     */
    void Init();

private:
    size_t               m_ulLength;
    char*                m_pBuffer;
    size_t               m_lBufferLen;

    std::ostream*        m_pStream;
    std::istream*        m_pReadStream;
    bool                 m_pStreamOwned;

    PdfRefCountedBuffer* m_pRefCountedBuffer;
    size_t               m_ulPosition;
    std::string  m_printBuffer;
};

};

#endif // _PDF_OUTPUT_DEVICE_H_

