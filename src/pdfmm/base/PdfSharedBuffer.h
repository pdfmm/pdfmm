/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_SHARED_BUFFER_H_
#define PDF_SHARED_BUFFER_H_

#include "PdfDefines.h"

namespace mm
{

/**
 * A reference counted buffer object
 * which is deleted as soon as the last
 * object having access to it is deleted.
 *
 * The attached memory object can be resized.
 */
class PDFMM_API PdfSharedBuffer
{
public:
    /** Created an empty reference counted buffer
     *  The buffer will be initialized to nullptr
     */
    PdfSharedBuffer();

    /** Created an reference counted buffer and use an exiting buffer
     *  The buffer will be owned by this object.
     *
     *  \param buffer a pointer to an allocated buffer
     *  \param size   size of the allocated buffer
     *
     *  \see SetTakePossesion
     */
    PdfSharedBuffer(char* buffer, size_t size);

    PdfSharedBuffer(const std::string_view& view);

    /** Create a new PdfSharedBuffer.
     *  \param size buffer size
     */
    PdfSharedBuffer(size_t size);

    /** Copy an existing PdfSharedBuffer and increase
     *  the reference count
     *  \param rhs the PdfSharedBuffer to copy
     */
    PdfSharedBuffer(const PdfSharedBuffer& rhs);

    /** Decrease the reference count and delete the buffer
     *  if this is the last owner
     */
    ~PdfSharedBuffer();

    /** Get access to the buffer
     *  \returns the buffer
     */
    const char* GetBuffer() const;

    /** Get access to the buffer
     *  Note it doesn't detach the buffer. Manually detach with Detach()
     *  \returns the buffer
     */
    char* GetBuffer();

    /** Return the buffer size.
     *
     *  \returns the buffer size
     */
    size_t GetSize() const;

    /** Resize the buffer to hold at least
     *  size bytes.
     *
     *  \param size the size of bytes the buffer can at least hold
     *
     *  If the buffer is larger no operation is performed.
     */
    void Resize(size_t size);

    /** Detach from a shared buffer or do nothing if we are the only
     *  one referencing the buffer.
     *
     *  Call this function before any operation modifying the buffer!
     *
     *  \param len an additional parameter specifying extra bytes
     *              to be allocated to optimize allocations of a new buffer.
     */
    void Detach(size_t extraLen = 0);

    /** Copy an existing PdfSharedBuffer and increase
     *  the reference count
     *  \param rhs the PdfSharedBuffer to copy
     *  \returns the copied object
     */
    const PdfSharedBuffer& operator=(const PdfSharedBuffer& rhs);

    /** If the PdfSharedBuffer has no possession on its buffer,
     *  it won't delete the buffer. By default the buffer is owned
     *  and deleted by the PdfSharedBuffer object.
     *
     *  \param bTakePossession if false the buffer will not be deleted.
     */
    void SetTakePossesion(bool bTakePossession);

    /**
     * \returns true if the buffer is owned by the PdfSharedBuffer
     *               and is deleted along with it.
     */
    bool TakePossesion() const;

    /** Compare to buffers.
     *  \param rhs compare to this buffer
     *  \returns true if both buffers contain the same contents
     */
    bool operator==(const PdfSharedBuffer& rhs) const;

    /** Compare to buffers.
     *  \param rhs compare to this buffer
     *  \returns true if this buffer is lexically smaller than rhs
     */
    bool operator<(const PdfSharedBuffer& rhs) const;

    /** Compare to buffers.
     *  \param rhs compare to this buffer
     *  \returns true if this buffer is lexically greater than rhs
     */
    bool operator>(const PdfSharedBuffer& rhs) const;

private:
    /**
     * Indicate that the buffer is no longer being used, freeing it if there
     * are no further users. The buffer becomes inaccessible to this
     * PdfSharedBuffer in either case.
     */
    void DerefBuffer();

    /**
     * Free a buffer if the refcount is zero. Internal method used by DerefBuffer.
     */
    void FreeBuffer();

    /**
     * Called by Detach() to do the work if action is actually required.
     * \see Detach
     */
    void ReallyDetach(size_t lExtraLen);

    /**
     * Do the hard work of resizing the buffer if it turns out not to already be big enough.
     * \see Resize
     */
    void ReallyResize(size_t size);

private:
    struct RefCountedBuffer
    {
        static constexpr unsigned INTERNAL_BUFSIZE = 32;

        // Convenience for buffer switching
        char* GetRealBuffer();
        // size in bytes of the buffer. If and only if this is strictly >INTERNAL_BUFSIZE,
        // this buffer is on the heap in memory pointed to by m_HeapBuffer . If it is <=INTERNAL_BUFSIZE,
        // the buffer is in the in-object buffer m_sInternalBuffer.
        size_t  m_BufferSize;
        // Size in bytes of m_Buffer that should be reported to clients. We
        // over-allocate on the heap for efficiency and have a minimum 32 byte
        // size, but this extra should NEVER be visible to a client.
        size_t  m_VisibleSize;
        unsigned m_RefCount;
        char* m_HeapBuffer;
        char  m_InternalBuffer[INTERNAL_BUFSIZE];
        bool  m_Possesion;
        // Are we using the heap-allocated buffer in place of our small internal one?
        bool  m_OnHeap;
    };

    RefCountedBuffer* m_Buffer;
};

};

#endif // PDF_SHARED_BUFFER_H_
