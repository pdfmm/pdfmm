/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_STREAM_H
#define PDF_STREAM_H

#include "PdfDefines.h"

#include "PdfFilter.h"
#include "PdfSharedBuffer.h"
#include "PdfEncrypt.h"

namespace mm {

class PdfInputStream;
class PdfName;
class PdfObject;
class PdfOutputStream;

/** A PDF stream can be appended to any PdfObject
 *  and can contain arbitrary data.
 *
 *  Most of the time it will contain either drawing commands
 *  to draw onto a page or binary data like a font or an image.
 *
 *  You have to use a concrete implementation of a stream,
 *  which can be retrieved from a StreamFactory.
 *  \see PdfIndirectObjectList
 *  \see PdfMemoryStream
 *  \see PdfFileStream
 */
class PDFMM_API PdfStream
{
    friend class PdfParserObject;
public:
    /** The default filter to use when changing the stream content.
     *  It's a static member and applies to all newly created/changed streams.
     *  The default value is EPdfFilter::FlateDecode.
     */
    static enum PdfFilterType DefaultFilter;

protected:
    /** Create a new PdfStream object which has a parent PdfObject.
     *  The stream will be deleted along with the parent.
     *  This constructor will be called by PdfObject::Stream() for you.
     *  \param parent parent object
     */
    PdfStream(PdfObject& parent);

public:
    virtual ~PdfStream();

    /** Write the stream to an output device
     *  \param device write to this outputdevice.
     *  \param encrypt encrypt stream data using this object
     */
    virtual void Write(PdfOutputDevice& device, const PdfEncrypt* encrypt) = 0;

    /** Set a binary buffer as stream data.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param view buffer containing the stream data
     *  \param filters a list of filters to use when appending data
     */
    void Set(const std::string_view& view, const PdfFilterList& filters);

    /** Set a binary buffer as stream data.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param buffer buffer containing the stream data
     *  \param len length of the buffer
     *  \param filters a list of filters to use when appending data
     */
    void Set(const char* buffer, size_t len, const PdfFilterList& filters);

    /** Set a binary buffer as stream data.
     *  All data will be Flate-encoded.
     *
     *  \param view buffer containing the stream data
     */
    void Set(const std::string_view& view);

    /** Set a binary buffer as stream data.
     *  All data will be Flate-encoded.
     *
     *  \param buffer buffer containing the stream data
     *  \param len length of the buffer
     */
    void Set(const char* buffer, size_t len);

    /** Set a binary buffer whose contents are read from a PdfInputStream
     *  All data will be Flate-encoded.
     *
     *  \param stream read stream contents from this PdfInputStream
     */
    void Set(PdfInputStream& stream);

    /** Set a binary buffer whose contents are read from a PdfInputStream
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param stream read stream contents from this PdfInputStream
     *  \param filters a list of filters to use when appending data
     */
    void Set(PdfInputStream& stream, const PdfFilterList& filters);

    /** Sets raw data for this stream which is read from an input stream.
     *  This method does neither encode nor decode the read data.
     *  The filters of the object are not modified and the data is expected
     *  encoded as stated by the /Filters key in the stream's object.
     *
     *  \param stream read data from this input stream
     *  \param len    read exactly len bytes from the input stream,
     *                 if len = -1 read until the end of the input stream
     *                 was reached.
     */
    void SetRawData(PdfInputStream& stream, ssize_t len = -1);

    /** Start appending data to this stream.
     *
     *  This method has to be called before any of the append methods.
     *  All appended data will be Flate-encoded.
     *
     *  \param bClearExisting if true any existing stream contents will
     *         be cleared.
     *
     *  \see Append
     *  \see EndAppend
     *  \see eDefaultFilter
     */
    void BeginAppend(bool clearExisting = true);

    /** Start appending data to this stream.
     *  This method has to be called before any of the append methods.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param filters a list of filters to use when appending data
     *  \param bClearExisting if true any existing stream contents will
               be cleared.
     *  \param bDeleteFilters if true existing filter keys are deleted if an
     *         empty list of filters is passed (required for SetRawData())
     *
     *  \see Append
     *  \see EndAppend
     */
    void BeginAppend(const PdfFilterList& filters, bool clearExisting = true, bool deleteFilters = true);

    /** Append a binary buffer to the current stream contents.
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param view a buffer
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    PdfStream& Append(const std::string_view& view);

    /** Append a binary buffer to the current stream contents.
     *
     *  Make sure BeginAppend() has been called before.
     *
     *  \param buffer a buffer
     *  \param len size of the buffer
     *
     *  \see BeginAppend
     *  \see EndAppend
     */
    PdfStream& Append(const char* buffer, size_t len);

    /** Finish appending data to this stream.
     *  BeginAppend() has to be called before this method.
     *
     *  \see BeginAppend
     *  \see Append
     */
    void EndAppend();

    /**
     * \returns true if code is between BeginAppend()
     *          and EndAppend() at the moment, i.e. it
     *          is safe to call EndAppend() now.
     *
     *  \see BeginAppend
     *  \see Append
     */
    inline bool IsAppending() const { return m_Append; }

    /** Get the stream's length with all filters applied (e.g. if the stream is
     * Flate-compressed, the length of the compressed data stream).
     *
     *  \returns the length of the internal buffer
     */
    virtual size_t GetLength() const = 0;

    /** Get a malloc()'d buffer of the current stream.
     *  No filters will be applied to the buffer, so
     *  if the stream is Flate-compressed the compressed copy
     *  will be returned.
     *
     *  The caller has to the buffer.
     *
     *  \param buffer pointer to the buffer
     *  \param len    pointer to the buffer length
     */
     // TODO: Move to std::unique_ptr<char>
    virtual void GetCopy(char** buffer, size_t* len) const = 0;

    /** Get a copy of a the stream and write it to a PdfOutputStream
     *
     *  \param stream data is written to this stream.
     */
    virtual void GetCopy(PdfOutputStream& stream) const = 0;

    /** Get a malloc()'d buffer of the current stream which has been
     *  filtered by all filters as specified in the dictionary's
     *  /Filter key. For example, if the stream is Flate-compressed,
     *  the buffer returned from this method will have been decompressed.
     *
     *  The caller has to the buffer.
     *
     *  \param buffer pointer to the buffer
     *  \param len    pointer to the buffer length
     */
    void GetFilteredCopy(std::unique_ptr<char>& buffer, size_t& len) const;

    /** Get a filtered copy of a the stream and write it to a PdfOutputStream
     *
     *  \param stream filtered data is written to this stream.
     */
    void GetFilteredCopy(PdfOutputStream& stream) const;

    /** Create a copy of a PdfStream object
     *  \param rhs the object to clone
     *  \returns a reference to this object
     */
    const PdfStream& operator=(const PdfStream& rhs);

protected:
    /** Required for the GetFilteredCopy() implementation
     *  \returns a handle to the internal buffer
     */
    virtual const char* GetInternalBuffer() const = 0;

    /** Required for the GetFilteredCopy() implementation
     *  \returns the size of the internal buffer
     */
    virtual size_t GetInternalBufferSize() const = 0;

    /** Begin appending data to this stream.
     *  Clears the current stream contents.
     *
     * Use PdfFilterFactory::CreateFilterList() if you want to use the contents
     * of the stream dictionary's existing filter key.
     *
     *  \param filters use these filters to encode any data written
     *         to the stream.
     */
    virtual void BeginAppendImpl(const PdfFilterList& filters) = 0;

    /** Append a binary buffer to the current stream contents.
     *
     *  \param data a buffer
     *  \param len length of the buffer
     *
     *  \see BeginAppend
     *  \see Append
     *  \see EndAppend
     */
    virtual void AppendImpl(const char* data, size_t len) = 0;

    /** Finish appending data to the stream
     */
    virtual void EndAppendImpl() = 0;

    virtual void CopyFrom(const PdfStream& rhs);

    void EnsureAppendClosed();

private:
    PdfStream(const PdfStream& rhs) = delete;

    void append(const char* data, size_t len);

    void endAppend();

    void SetRawData(PdfInputStream& stream, ssize_t len, bool markObjectDirty);

    void BeginAppend(const PdfFilterList& filters, bool clearExisting, bool deleteFilters, bool markObjectDirty);

protected:
    PdfObject* m_Parent;

    bool m_Append;
};

};

#endif // PDF_STREAM_H
