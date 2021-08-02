/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfRefCountedBuffer.h"

using namespace PoDoFo;

PdfRefCountedBuffer::PdfRefCountedBuffer( char* pBuffer, size_t lSize )
    : m_pBuffer( nullptr )
{
    if( pBuffer && lSize ) 
    {
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount     = 1;
        m_pBuffer->m_pHeapBuffer   = pBuffer;
        m_pBuffer->m_bOnHeap       = true;
        m_pBuffer->m_lBufferSize   = lSize;
        m_pBuffer->m_lVisibleSize  = lSize;
        m_pBuffer->m_bPossesion    = true;
    }
}

PdfRefCountedBuffer::PdfRefCountedBuffer(const std::string_view& view)
{
    this->Resize(view.size());
    std::memcpy(m_pBuffer->GetRealBuffer(), view.data(), view.size());
}

PdfRefCountedBuffer::PdfRefCountedBuffer()
    : m_pBuffer(nullptr)
{
}

PdfRefCountedBuffer::PdfRefCountedBuffer(size_t lSize)
    : m_pBuffer(nullptr)
{
    this->Resize(lSize);
}

// We define the copy ctor separately to the assignment
// operator since it's a *LOT* faster this way.
PdfRefCountedBuffer::PdfRefCountedBuffer(const PdfRefCountedBuffer & rhs)
    : m_pBuffer(rhs.m_pBuffer)
{
    if (m_pBuffer)
        ++(m_pBuffer->m_lRefCount);
}

PdfRefCountedBuffer::~PdfRefCountedBuffer()
{
    DerefBuffer();
}

char * PdfRefCountedBuffer::GetBuffer()
{
    if (m_pBuffer == nullptr)
        return nullptr;

    return m_pBuffer->GetRealBuffer();
}

const char* PdfRefCountedBuffer::GetBuffer() const
{
    return const_cast<PdfRefCountedBuffer &>(*this).GetBuffer();
}

size_t PdfRefCountedBuffer::GetSize() const
{
    return m_pBuffer ? m_pBuffer->m_lVisibleSize : 0;
}

void PdfRefCountedBuffer::SetTakePossesion(bool bTakePossession)
{
    if (m_pBuffer)
        m_pBuffer->m_bPossesion = bTakePossession;
}

bool PdfRefCountedBuffer::TakePossesion() const
{
    return m_pBuffer ? m_pBuffer->m_bPossesion : false;
}

void PdfRefCountedBuffer::Detach(size_t lExtraLen)
{
    if (m_pBuffer && m_pBuffer->m_lRefCount > 1)
        ReallyDetach(lExtraLen);
}

void PdfRefCountedBuffer::Resize(size_t lSize)
{
    if (m_pBuffer && m_pBuffer->m_lRefCount == 1 && static_cast<size_t>(m_pBuffer->m_lBufferSize) >= lSize)
    {
        // We have a solely owned buffer the right size already; no need to
        // waste any time detaching or resizing it. Just let the client see
        // more of it (or less if they're shrinking their view).
        m_pBuffer->m_lVisibleSize = lSize;
    }
    else
    {
        ReallyResize(lSize);
    }
}

void PdfRefCountedBuffer::DerefBuffer()
{
    if (m_pBuffer && !(--m_pBuffer->m_lRefCount))
        FreeBuffer();
    // Whether or not it still exists, we no longer have anything to do with
    // the buffer we just released our claim on.
    m_pBuffer = nullptr;
}

void PdfRefCountedBuffer::FreeBuffer()
{
    PODOFO_RAISE_LOGIC_IF( !m_pBuffer || m_pBuffer->m_lRefCount, "Tried to free in-use buffer" );

    // last owner of the file!
    if( m_pBuffer->m_bOnHeap && m_pBuffer->m_bPossesion )
        podofo_free( m_pBuffer->m_pHeapBuffer );
    delete m_pBuffer;
}

void PdfRefCountedBuffer::ReallyDetach( size_t lExtraLen )
{
    PODOFO_RAISE_LOGIC_IF( m_pBuffer && m_pBuffer->m_lRefCount == 1, "Use Detach() rather than calling ReallyDetach() directly." )

    if( !m_pBuffer )
    {
        // throw error rather than de-referencing nullptr
        PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
    }
    
    size_t lSize                 = m_pBuffer->m_lBufferSize + lExtraLen;
    TRefCountedBuffer* pBuffer = new TRefCountedBuffer();
    pBuffer->m_lRefCount       = 1;

    pBuffer->m_bOnHeap = (lSize > TRefCountedBuffer::INTERNAL_BUFSIZE);
    if ( pBuffer->m_bOnHeap )
        pBuffer->m_pHeapBuffer  = static_cast<char*>(podofo_calloc( lSize, sizeof(char) ));
    else
        pBuffer->m_pHeapBuffer = 0;
    pBuffer->m_lBufferSize     = std::max( lSize, static_cast<size_t>(+TRefCountedBuffer::INTERNAL_BUFSIZE) );
    pBuffer->m_bPossesion      = true;

    if( pBuffer->m_bOnHeap && !pBuffer->m_pHeapBuffer ) 
    {
        delete pBuffer;
        pBuffer = nullptr;

        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }

    memcpy( pBuffer->GetRealBuffer(), this->GetBuffer(), this->GetSize() );
    // Detaching the buffer should have NO visible effect to clients, so the
    // visible size must not change.
    pBuffer->m_lVisibleSize    = m_pBuffer->m_lVisibleSize;

    // Now that we've copied the data, release our claim on the old buffer,
    // deleting it if needed, and link up the new one.
    DerefBuffer();
    m_pBuffer = pBuffer;
}

void PdfRefCountedBuffer::ReallyResize( const size_t lSize ) 
{
    if( m_pBuffer )
    {
        // Resizing the buffer counts as altering it, so detach as per copy on write behaviour. If the detach
        // actually has to do anything it'll reallocate the buffer at the new desired size.
        this->Detach( static_cast<size_t>(m_pBuffer->m_lBufferSize) < lSize ? lSize - static_cast<size_t>(m_pBuffer->m_lBufferSize) : 0 );
        // We might have pre-allocated enough to service the request already
        if( static_cast<size_t>(m_pBuffer->m_lBufferSize) < lSize )
        {
            // Allocate more space, since we need it. We over-allocate so that clients can efficiently
            // request lots of small resizes if they want, but these over allocations are not visible
            // to clients.
            //
            const size_t lAllocSize = std::max(lSize, m_pBuffer->m_lBufferSize) << 1;
            if ( m_pBuffer->m_bPossesion && m_pBuffer->m_bOnHeap )
            {
                // We have an existing on-heap buffer that we own. Realloc()
                // it, potentially saving us a memcpy and free().
                void* temp = podofo_realloc( m_pBuffer->m_pHeapBuffer, lAllocSize );
                if (!temp)
                {
                    PODOFO_RAISE_ERROR_INFO( EPdfError::OutOfMemory, "PdfRefCountedBuffer::Resize failed!" );
                }
                m_pBuffer->m_pHeapBuffer = static_cast<char*>(temp);
                m_pBuffer->m_lBufferSize = lAllocSize;
            }
            else
            {
                // Either we don't own the buffer or it's a local static buffer that's no longer big enough.
                // Either way, it's time to move to a heap-allocated buffer we own.
                char* pBuffer = static_cast<char*>(podofo_calloc( lAllocSize, sizeof(char) ));
                if( !pBuffer ) 
                {
                    PODOFO_RAISE_ERROR_INFO( EPdfError::OutOfMemory, "PdfRefCountedBuffer::Resize failed!" );
                }
                // Only bother copying the visible portion of the buffer. It's completely incorrect
                // to rely on anything more than that, and not copying it will help catch those errors.
                memcpy( pBuffer, m_pBuffer->GetRealBuffer(), m_pBuffer->m_lVisibleSize );
                // Record the newly allocated buffer's details. The visible size gets updated later.
                m_pBuffer->m_lBufferSize = lAllocSize;
                m_pBuffer->m_pHeapBuffer = pBuffer;
                m_pBuffer->m_bOnHeap = true;
            }
        }
        else
        {
            // Allocated buffer is large enough, do nothing
        }
    }
    else
    {
        // No buffer was allocated at all, so we need to make one.
        m_pBuffer = new TRefCountedBuffer();
        m_pBuffer->m_lRefCount       = 1;
        m_pBuffer->m_bOnHeap   = (lSize > TRefCountedBuffer::INTERNAL_BUFSIZE);
        if ( m_pBuffer->m_bOnHeap )
        {
            m_pBuffer->m_pHeapBuffer = static_cast<char*>(podofo_calloc( lSize, sizeof(char) ));
        }
        else
        {
            m_pBuffer->m_pHeapBuffer = 0;
        }

        m_pBuffer->m_lBufferSize     = std::max( lSize, static_cast<size_t>(+TRefCountedBuffer::INTERNAL_BUFSIZE) );
        m_pBuffer->m_bPossesion      = true;

        if( m_pBuffer->m_bOnHeap && !m_pBuffer->m_pHeapBuffer ) 
        {
            delete m_pBuffer;
            m_pBuffer = nullptr;

            PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
        }
    }
    m_pBuffer->m_lVisibleSize = lSize;

    PODOFO_RAISE_LOGIC_IF ( m_pBuffer->m_lVisibleSize > m_pBuffer->m_lBufferSize, "Buffer improperly allocated/resized");
}

const PdfRefCountedBuffer & PdfRefCountedBuffer::operator=( const PdfRefCountedBuffer & rhs )
{
    // Self assignment is a no-op
    if (this == &rhs)
        return rhs;

    DerefBuffer();

    m_pBuffer = rhs.m_pBuffer;
    if( m_pBuffer )
        m_pBuffer->m_lRefCount++;

    return *this;
}

bool PdfRefCountedBuffer::operator==( const PdfRefCountedBuffer & rhs ) const
{
    if( m_pBuffer != rhs.m_pBuffer )
    {
        if( m_pBuffer && rhs.m_pBuffer ) 
        {
            if ( m_pBuffer->m_lVisibleSize != rhs.m_pBuffer->m_lVisibleSize )
                // Unequal buffer sizes cannot be equal buffers
                return false;
            // Test for byte-for-byte equality since lengths match
            return (memcmp( m_pBuffer->GetRealBuffer(), rhs.m_pBuffer->GetRealBuffer(), m_pBuffer->m_lVisibleSize ) == 0 );
        }
        else
            // Cannot be equal if only one object has a real data buffer
            return false;
    }

    return true;
}

bool PdfRefCountedBuffer::operator<( const PdfRefCountedBuffer & rhs ) const
{
    // equal buffers are neither smaller nor greater
    if( m_pBuffer == rhs.m_pBuffer )
        return false;

    if( !m_pBuffer && rhs.m_pBuffer ) 
        return true;
    else if( m_pBuffer && !rhs.m_pBuffer ) 
        return false;
    else
    {
        int cmp = memcmp( m_pBuffer->GetRealBuffer(), rhs.m_pBuffer->GetRealBuffer(), std::min( m_pBuffer->m_lVisibleSize, rhs.m_pBuffer->m_lVisibleSize ) );
        if (cmp == 0)
            // If one is a prefix of the other, ie they compare equal for the length of the shortest but one is longer,
            // the longer buffer is the greater one.
            return m_pBuffer->m_lVisibleSize < rhs.m_pBuffer->m_lVisibleSize;
        else
            return cmp < 0;
    }
}

bool PdfRefCountedBuffer::operator>( const PdfRefCountedBuffer & rhs ) const
{
    // equal buffers are neither smaller nor greater
    if( m_pBuffer == rhs.m_pBuffer )
        return false;

    if( !m_pBuffer && rhs.m_pBuffer ) 
        return false;
    else if( m_pBuffer && !rhs.m_pBuffer ) 
        return true;
    else
    {
        int cmp = memcmp( m_pBuffer->GetRealBuffer(), rhs.m_pBuffer->GetRealBuffer(), std::min( m_pBuffer->m_lVisibleSize, rhs.m_pBuffer->m_lVisibleSize ) );
        if (cmp == 0)
            // If one is a prefix of the other, ie they compare equal for the length of the shortest but one is longer,
            // the longer buffer is the greater one.
            return m_pBuffer->m_lVisibleSize > rhs.m_pBuffer->m_lVisibleSize;
        else
            return cmp > 0;
    }
}

char * PdfRefCountedBuffer::TRefCountedBuffer::GetRealBuffer()
{
     return m_bOnHeap ? m_pHeapBuffer : &(m_sInternalBuffer[0]);
}
