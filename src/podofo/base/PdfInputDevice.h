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

#ifndef _PDF_INPUT_DEVICE_H_
#define _PDF_INPUT_DEVICE_H_

#include <istream>
#include <fstream>

#include "PdfDefines.h"
#include "PdfLocale.h"

namespace PoDoFo {

// TODO: Refactor, PdfInputDevice should be an interface
/** This class provides an Input device which operates 
 *  either on a file, a buffer in memory or any arbitrary std::istream
 *
 *  This class is suitable for inheritance to provide input 
 *  devices of your own for PoDoFo.
 *  Just overide the required virtual methods.
 */
class PODOFO_API PdfInputDevice
{
protected:
    PdfInputDevice();

public:
    /** Construct a new PdfInputDevice that reads all data from a file.
     *
     *  \param pszFilename path to a file that will be opened and all data
     *                     is read from this file.
     */
    explicit PdfInputDevice(const std::string_view& filename);

    /** Construct a new PdfInputDevice that reads all data from a memory buffer.
     *  The buffer will not be owned by this object - it is COPIED.
     *
     *  \param pBuffer a buffer in memory
     *  \param lLen the length of the buffer in memory
     */
    PdfInputDevice( const char* pBuffer, size_t lLen );

    /** Construct a new PdfInputDevice that reads all data from a std::istream.
     *
     *  \param pInStream read from this std::istream
     */
    explicit PdfInputDevice( const std::istream* pInStream );

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
    virtual bool TryGetChar(int &ch);

    /** Peek at next char in stream.
     *  /returns the next char in the stream
     */
    virtual int Look();

    /** Seek the device to the position offset from the begining
     *  \param off from the beginning of the file
     *  \param dir where to start (start, cur, end)
     *
     *  A non-seekable input device will throw an InvalidDeviceOperation.
     */
    void Seek(std::streamoff off, std::ios_base::seekdir dir = std::ios_base::beg);

    /** Read a certain number of bytes from the input device.
     *  
     *  \param pBuffer store bytes in this buffer.
     *                 The buffer has to be large enough.
     *  \param lLen    number of bytes to read.
     *  \returns the number of bytes that have been read.
     *           If reading was successfull the number of read bytes
     *           is equal to lLen.
     */
    virtual size_t Read( char* pBuffer, size_t lLen );

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
    std::istream* m_pStream;
    bool m_StreamOwned;
};

};

#endif // _PDF_INPUT_DEVICE_H_
