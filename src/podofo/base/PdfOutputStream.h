/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#ifndef _PDF_OUTPUT_STREAM_H_
#define _PDF_OUTPUT_STREAM_H_

#include "PdfDefines.h"
#include "PdfRefCountedBuffer.h"

#include <string>

namespace PoDoFo {

class PdfOutputDevice;

/** An interface for writing blocks of data to 
 *  a data source.
 */     
class PODOFO_API PdfOutputStream
{
public:
    virtual ~PdfOutputStream() { };

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer
     */
    void Write(const char* buffer, size_t len);

    /** Write data to the output stream
     *  
     *  \param view the data is read from this buffer
     */
    void Write(const std::string_view& view);

    /** Close the PdfOutputStream.
     *  This method may throw exceptions and has to be called 
     *  before the descructor to end writing.
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
 *
 *  TODO: remove in favour of PdfBufferOutputStream.
 */
class PODOFO_API PdfMemoryOutputStream : public PdfOutputStream
{
public:
    static constexpr size_t INITIAL_SIZE = 4096;

    /** 
     *  Construct a new PdfMemoryOutputStream
     *  \param lInitial initial size of the buffer
     */
    PdfMemoryOutputStream(size_t lInitial = INITIAL_SIZE);

    /**
     * Construct a new PdfMemoryOutputStream that writes to an existing buffer
     * \param pBuffer handle to the buffer
     * \param lLen length of the buffer
     */
    PdfMemoryOutputStream( char* pBuffer, size_t lLen );

    virtual ~PdfMemoryOutputStream();

    /** Close the PdfOutputStream.
     *  This method may throw exceptions and has to be called 
     *  before the descructor to end writing.
     *
     *  No more data may be written to the output device
     *  after calling close.
     */
    void Close() override;

    inline const char * GetBuffer() const { return m_pBuffer; }

    /** \returns the length of the written data
     */
    inline size_t GetLength() const { return m_lLen; }

    /**
     *  \returns a handle to the internal buffer.
     *
     *  The internal buffer is now owned by the caller
     *  and will not be deleted by PdfMemoryOutputStream.
     *  Further calls to Write() are not allowed.
     *
     *  The caller has to free() the returned malloc()'ed buffer!
     */
    char* TakeBuffer();

protected:
    void WriteImpl(const char* data, size_t len) override;

 private:
    char* m_pBuffer;
    size_t m_lLen;
    size_t m_lSize;
    bool m_bOwnBuffer;
};

/** An output stream that writes to a PdfOutputDevice
 */
class PODOFO_API PdfDeviceOutputStream : public PdfOutputStream
{
public:
    
    /** 
     *  Write to an already opened input device
     * 
     *  \param pDevice an output device
     */
    PdfDeviceOutputStream( PdfOutputDevice* pDevice );

    /** Close the PdfOutputStream.
     *  This method may throw exceptions and has to be called 
     *  before the descructor to end writing.
     *
     *  No more data may be written to the output device
     *  after calling close.
     */
    void Close() override;

protected:
    void WriteImpl(const char* data, size_t len) override;

 private:
    PdfOutputDevice* m_pDevice;
};

/** An output stream that writes to a PdfRefCountedBuffer.
 * 
 *  The PdfRefCountedBuffer is resized automatically if necessary.
 */
class PODOFO_API PdfBufferOutputStream : public PdfOutputStream
{
public:
    
    /** 
     *  Write to an already opened input device
     * 
     *  \param pBuffer data is written to this buffer
     */
    PdfBufferOutputStream(PdfRefCountedBuffer* pBuffer);

    virtual void Close() override;

    /** 
     * \returns the length of the buffers contents
     */
    inline size_t GetLength() const { return m_lLength; }

protected:
    void WriteImpl(const char* data, size_t len) override;

 private:
    PdfRefCountedBuffer* m_pBuffer;
    size_t m_lLength;
};

};

#endif // _PDF_OUTPUT_STREAM_H_
