/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfSharedBuffer.h"

using namespace mm;

PdfSharedBuffer::PdfSharedBuffer(char* buffer, size_t size)
    : m_Buffer(nullptr)
{
    if (buffer != nullptr && size != 0)
    {
        m_Buffer = new RefCountedBuffer();
        m_Buffer->m_RefCount = 1;
        m_Buffer->m_HeapBuffer = buffer;
        m_Buffer->m_OnHeap = true;
        m_Buffer->m_BufferSize = size;
        m_Buffer->m_VisibleSize = size;
        m_Buffer->m_Possesion = true;
    }
}

PdfSharedBuffer::PdfSharedBuffer(const std::string_view& view)
{
    this->Resize(view.size());
    std::memcpy(m_Buffer->GetRealBuffer(), view.data(), view.size());
}

PdfSharedBuffer::PdfSharedBuffer()
    : m_Buffer(nullptr)
{
}

PdfSharedBuffer::PdfSharedBuffer(size_t size)
    : m_Buffer(nullptr)
{
    this->Resize(size);
}

// We define the copy ctor separately to the assignment
// operator since it's a *LOT* faster this way.
PdfSharedBuffer::PdfSharedBuffer(const PdfSharedBuffer& rhs)
    : m_Buffer(rhs.m_Buffer)
{
    if (m_Buffer != nullptr)
        m_Buffer->m_RefCount++;
}

PdfSharedBuffer::~PdfSharedBuffer()
{
    DerefBuffer();
}

char* PdfSharedBuffer::GetBuffer()
{
    if (m_Buffer == nullptr)
        return nullptr;

    return m_Buffer->GetRealBuffer();
}

const char* PdfSharedBuffer::GetBuffer() const
{
    return const_cast<PdfSharedBuffer&>(*this).GetBuffer();
}

size_t PdfSharedBuffer::GetSize() const
{
    return m_Buffer != nullptr ? m_Buffer->m_VisibleSize : 0;
}

void PdfSharedBuffer::SetTakePossesion(bool bTakePossession)
{
    if (m_Buffer != nullptr)
        m_Buffer->m_Possesion = bTakePossession;
}

bool PdfSharedBuffer::TakePossesion() const
{
    return m_Buffer ? m_Buffer->m_Possesion : false;
}

void PdfSharedBuffer::Detach(size_t lExtraLen)
{
    if (m_Buffer != nullptr && m_Buffer->m_RefCount > 1)
        ReallyDetach(lExtraLen);
}

void PdfSharedBuffer::Resize(size_t size)
{
    if (m_Buffer != nullptr && m_Buffer->m_RefCount == 1 && static_cast<size_t>(m_Buffer->m_BufferSize) >= size)
    {
        // We have a solely owned buffer the right size already; no need to
        // waste any time detaching or resizing it. Just let the client see
        // more of it (or less if they're shrinking their view).
        m_Buffer->m_VisibleSize = size;
    }
    else
    {
        ReallyResize(size);
    }
}

void PdfSharedBuffer::DerefBuffer()
{
    if (m_Buffer != nullptr && --m_Buffer->m_RefCount == 0)
        FreeBuffer();
    // Whether or not it still exists, we no longer have anything to do with
    // the buffer we just released our claim on.
    m_Buffer = nullptr;
}

void PdfSharedBuffer::FreeBuffer()
{
    PDFMM_RAISE_LOGIC_IF(m_Buffer == nullptr || m_Buffer->m_RefCount != 0, "Tried to free in-use buffer");

    // last owner of the file!
    if (m_Buffer->m_OnHeap && m_Buffer->m_Possesion)
        pdfmm_free(m_Buffer->m_HeapBuffer);
    delete m_Buffer;
}

void PdfSharedBuffer::ReallyDetach(size_t extraLen)
{
    PDFMM_RAISE_LOGIC_IF(m_Buffer != nullptr && m_Buffer->m_RefCount == 1, "Use Detach() rather than calling ReallyDetach() directly.")

    if (m_Buffer == nullptr)
    {
        // throw error rather than de-referencing nullptr
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

    size_t size = m_Buffer->m_BufferSize + extraLen;
    auto buffer = new RefCountedBuffer();
    buffer->m_RefCount = 1;

    buffer->m_OnHeap = (size > RefCountedBuffer::INTERNAL_BUFSIZE);
    if (buffer->m_OnHeap)
        buffer->m_HeapBuffer = static_cast<char*>(pdfmm_calloc(size, sizeof(char)));
    else
        buffer->m_HeapBuffer = 0;
    buffer->m_BufferSize = std::max(size, static_cast<size_t>(RefCountedBuffer::INTERNAL_BUFSIZE));
    buffer->m_Possesion = true;

    if (buffer->m_OnHeap && buffer->m_HeapBuffer == nullptr)
    {
        delete buffer;
        buffer = nullptr;

        PDFMM_RAISE_ERROR(PdfErrorCode::OutOfMemory);
    }

    memcpy(buffer->GetRealBuffer(), this->GetBuffer(), this->GetSize());
    // Detaching the buffer should have NO visible effect to clients, so the
    // visible size must not change.
    buffer->m_VisibleSize = m_Buffer->m_VisibleSize;

    // Now that we've copied the data, release our claim on the old buffer,
    // deleting it if needed, and link up the new one.
    DerefBuffer();
    m_Buffer = buffer;
}

void PdfSharedBuffer::ReallyResize(const size_t size)
{
    if (m_Buffer != nullptr)
    {
        // Resizing the buffer counts as altering it, so detach as per copy on write behaviour. If the detach
        // actually has to do anything it'll reallocate the buffer at the new desired size.
        this->Detach(static_cast<size_t>(m_Buffer->m_BufferSize) < size ? size - static_cast<size_t>(m_Buffer->m_BufferSize) : 0);
        // We might have pre-allocated enough to service the request already
        if (static_cast<size_t>(m_Buffer->m_BufferSize) < size)
        {
            // Allocate more space, since we need it. We over-allocate so that clients can efficiently
            // request lots of small resizes if they want, but these over allocations are not visible
            // to clients.
            //
            size_t allocSize = std::max(size, m_Buffer->m_BufferSize) << 1;
            if (m_Buffer->m_Possesion && m_Buffer->m_OnHeap)
            {
                // We have an existing on-heap buffer that we own. Realloc()
                // it, potentially saving us a memcpy and free().
                auto temp = pdfmm_realloc(m_Buffer->m_HeapBuffer, allocSize);
                if (temp == nullptr)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "PdfSharedBuffer::Resize failed!");

                m_Buffer->m_HeapBuffer = static_cast<char*>(temp);
                m_Buffer->m_BufferSize = allocSize;
            }
            else
            {
                // Either we don't own the buffer or it's a local static buffer that's no longer big enough.
                // Either way, it's time to move to a heap-allocated buffer we own.
                char* buffer = static_cast<char*>(pdfmm_calloc(allocSize, sizeof(char)));
                if (buffer == nullptr)
                {
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::OutOfMemory, "PdfSharedBuffer::Resize failed!");
                }
                // Only bother copying the visible portion of the buffer. It's completely incorrect
                // to rely on anything more than that, and not copying it will help catch those errors.
                memcpy(buffer, m_Buffer->GetRealBuffer(), m_Buffer->m_VisibleSize);
                // Record the newly allocated buffer's details. The visible size gets updated later.
                m_Buffer->m_BufferSize = allocSize;
                m_Buffer->m_HeapBuffer = buffer;
                m_Buffer->m_OnHeap = true;
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
        m_Buffer = new RefCountedBuffer();
        m_Buffer->m_RefCount = 1;
        m_Buffer->m_OnHeap = (size > RefCountedBuffer::INTERNAL_BUFSIZE);
        if (m_Buffer->m_OnHeap)
        {
            m_Buffer->m_HeapBuffer = static_cast<char*>(pdfmm_calloc(size, sizeof(char)));
        }
        else
        {
            m_Buffer->m_HeapBuffer = 0;
        }

        m_Buffer->m_BufferSize = std::max(size, static_cast<size_t>(RefCountedBuffer::INTERNAL_BUFSIZE));
        m_Buffer->m_Possesion = true;

        if (m_Buffer->m_OnHeap && m_Buffer->m_HeapBuffer == nullptr)
        {
            delete m_Buffer;
            m_Buffer = nullptr;

            PDFMM_RAISE_ERROR(PdfErrorCode::OutOfMemory);
        }
    }
    m_Buffer->m_VisibleSize = size;

    PDFMM_RAISE_LOGIC_IF(m_Buffer->m_VisibleSize > m_Buffer->m_BufferSize, "Buffer improperly allocated/resized");
}

const PdfSharedBuffer& PdfSharedBuffer::operator=(const PdfSharedBuffer& rhs)
{
    // Self assignment is a no-op
    if (this == &rhs)
        return rhs;

    DerefBuffer();

    m_Buffer = rhs.m_Buffer;
    if (m_Buffer != nullptr)
        m_Buffer->m_RefCount++;

    return *this;
}

char* PdfSharedBuffer::RefCountedBuffer::GetRealBuffer()
{
    return m_OnHeap ? m_HeapBuffer : &(m_InternalBuffer[0]);
}
