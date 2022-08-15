/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfObjectStream.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfFilter.h"
#include "PdfInputDevice.h"
#include "PdfDictionary.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace mm;

constexpr PdfFilterType DefaultFilter = PdfFilterType::FlateDecode;

PdfObjectStream::PdfObjectStream(PdfObject& parent)
    : m_Parent(&parent), m_locked(false)
{
}

PdfObjectStream::~PdfObjectStream() { }

PdfObjectOutputStream PdfObjectStream::GetOutputStreamRaw(bool append)
{
    EnsureClosed();
    return PdfObjectOutputStream(*this, { }, append);
}

PdfObjectOutputStream PdfObjectStream::GetOutputStream(bool append)
{
    EnsureClosed();
    return PdfObjectOutputStream(*this, { DefaultFilter }, append);
}

PdfObjectOutputStream PdfObjectStream::GetOutputStream(const PdfFilterList& filters, bool append)
{
    EnsureClosed();
    return PdfObjectOutputStream(*this, PdfFilterList(filters), append);
}

PdfObjectInputStream PdfObjectStream::GetInputStream(bool unpack)
{
    EnsureClosed();
    return PdfObjectInputStream(*this, unpack);
}

void PdfObjectStream::UnwrapTo(charbuff& buffer) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    UnwrapTo(stream);
}

void PdfObjectStream::UnwrapToSafe(charbuff& buffer) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    UnwrapToSafe(stream);
}

void PdfObjectStream::UnwrapTo(OutputStream& stream) const
{
    PdfFilterList mediaFilters;
    auto inputStream = const_cast<PdfObjectStream&>(*this).getInputStream(true, mediaFilters);
    if (mediaFilters.size() != 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported expansion with media filters. Use GetInputStream(true) instead");
        
    inputStream->CopyTo(stream);
    stream.Flush();
}

void PdfObjectStream::UnwrapToSafe(OutputStream& stream) const
{
    PdfFilterList mediaFilters;
    auto inputStream = const_cast<PdfObjectStream&>(*this).getInputStream(true, mediaFilters);
    inputStream->CopyTo(stream);
    stream.Flush();
}

charbuff PdfObjectStream::GetUnwrappedCopy() const
{
    charbuff ret;
    StringStreamDevice stream(ret);
    UnwrapTo(stream);
    return ret;
}

charbuff PdfObjectStream::GetUnwrappedCopySafe() const
{
    charbuff ret;
    StringStreamDevice stream(ret);
    UnwrapToSafe(stream);
    return ret;
}

void PdfObjectStream::MoveTo(PdfObject& obj)
{
    PDFMM_RAISE_LOGIC_IF(!obj.IsDictionary(), "Target object should be a dictionary");
    EnsureClosed();
    obj.MoveStreamFrom(*m_Parent);
}

PdfObjectStream& PdfObjectStream::operator=(const PdfObjectStream& rhs)
{
    CopyFrom(rhs);
    return (*this);
}

void PdfObjectStream::CopyFrom(const PdfObjectStream& rhs)
{
    auto stream = const_cast<PdfObjectStream&>(rhs).getInputStream();
    this->SetData(*stream, true);
}

void PdfObjectStream::SetData(const bufferview& buffer, bool raw)
{
    EnsureClosed();
    SpanStreamDevice stream(buffer);
    if (raw)
        setData(stream, { }, -1, true);
    else
        setData(stream, { DefaultFilter }, -1, true);
}

void PdfObjectStream::SetData(const bufferview& buffer, const PdfFilterList& filters)
{
    EnsureClosed();
    SpanStreamDevice stream(buffer);
    setData(stream, filters, -1, true);
}

void PdfObjectStream::SetData(InputStream& stream, bool raw)
{
    EnsureClosed();
    if (raw)
        setData(stream, { DefaultFilter }, -1, true);
    else
        setData(stream, { }, -1, true);
}

void PdfObjectStream::SetData(InputStream& stream, const PdfFilterList& filters)
{
    EnsureClosed();
    setData(stream, filters, -1, true);
}

unique_ptr<InputStream> PdfObjectStream::getInputStream(bool unpack, PdfFilterList& mediaFilters)
{
    if (unpack)
    {
        auto filters = PdfFilterFactory::CreateFilterList(*m_Parent, mediaFilters);
        if (filters.size() == 0)
        {
            return getInputStream();
        }
        else
        {
            return PdfFilterFactory::CreateDecodeStream(getInputStream(),
                filters, m_Parent->GetDictionary());
        }
    }
    else
    {
        return getInputStream();
    }
}

void PdfObjectStream::setData(InputStream& stream, PdfFilterList filters, ssize_t size, bool markObjectDirty)
{
    if (markObjectDirty)
    {
        // We must make sure the parent will be set dirty. All methods
        // writing to the stream will call this method first
        m_Parent->SetDirty();
    }

    PdfObjectOutputStream output(*this, std::move(filters), false);
    if (size < 0)
        stream.CopyTo(output);
    else
        stream.CopyTo(output, (size_t)size);
}

void PdfObjectStream::InitData(InputStream& stream, size_t size)
{
    PdfObjectOutputStream output(*this);
    stream.CopyTo(output, size);
}

void PdfObjectStream::EnsureClosed()
{
    PDFMM_RAISE_LOGIC_IF(m_locked, "The stream should have no read/write operations in progress");
}

PdfObjectInputStream::PdfObjectInputStream()
    : m_stream(nullptr) { }

PdfObjectInputStream::~PdfObjectInputStream()
{
    if (m_stream != nullptr)
        m_stream->m_locked = false;
}

PdfObjectInputStream::PdfObjectInputStream(PdfObjectInputStream&& rhs) noexcept
{
    utls::move(rhs.m_stream, m_stream);
}

PdfObjectInputStream::PdfObjectInputStream(PdfObjectStream& stream, bool unpack)
    : m_stream(&stream)
{
    m_stream->m_locked = true;
    m_input = stream.getInputStream(unpack, m_MediaFilters);
}

size_t PdfObjectInputStream::readBuffer(char* buffer, size_t size, bool& eof)
{
    return ReadBuffer(*m_input, buffer, size, eof);
}

bool PdfObjectInputStream::readChar(char& ch)
{
    return ReadChar(*m_input, ch);
}

PdfObjectInputStream& PdfObjectInputStream::operator=(PdfObjectInputStream&& rhs) noexcept
{
    utls::move(rhs.m_stream, m_stream);
    return *this;
}

PdfObjectOutputStream::PdfObjectOutputStream()
    : m_stream(nullptr) { }

PdfObjectOutputStream::~PdfObjectOutputStream()
{
    if (m_stream != nullptr)
        m_stream->m_locked = false;
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectOutputStream&& rhs) noexcept
    : m_filters(std::move(rhs.m_filters))
{
    utls::move(rhs.m_stream, m_stream);
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream,
        PdfFilterList&& filters, bool append)
    : PdfObjectOutputStream(stream, std::move(filters), append, false)
{
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream)
    : PdfObjectOutputStream(stream, { }, false, true)
{
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream, PdfFilterList&& filters,
        bool append, bool preserveFilters)
    : m_stream(&stream), m_filters(std::move(filters))
{
    auto& parent = stream.GetParent();
    auto document = parent.GetDocument();
    if (document != nullptr)
        document->GetObjects().BeginAppendStream(stream);

    charbuff buffer;
    if (append)
        stream.UnwrapTo(buffer);

    if (m_filters.size() == 0)
    {
        if (!preserveFilters)
            parent.GetDictionary().RemoveKey(PdfName::KeyFilter);
    }
    else if (m_filters.size() == 1)
    {
        parent.GetDictionary().AddKey(PdfName::KeyFilter,
            PdfName(PdfFilterFactory::FilterTypeToName(m_filters.front())));
    }
    else // filters.size() > 1
    {
        PdfArray arrFilters;
        for (auto filterType : m_filters)
            arrFilters.Add(PdfName(PdfFilterFactory::FilterTypeToName(filterType)));

        parent.GetDictionary().AddKey(PdfName::KeyFilter, arrFilters);
    }

    m_stream->m_locked = true;
    if (m_filters.size() == 0)
    {
        m_output = stream.getOutputStream();
    }
    else
    {
        m_output = PdfFilterFactory::CreateEncodeStream(stream.getOutputStream(),
            m_filters);
    }

    if (buffer.size() != 0)
        WriteBuffer(*m_output, buffer.data(), buffer.size());
}

void PdfObjectOutputStream::writeBuffer(const char* buffer, size_t size)
{
    WriteBuffer(*m_output, buffer, size);
}

void PdfObjectOutputStream::flush()
{
    Flush(*m_output);
}

PdfObjectOutputStream& PdfObjectOutputStream::operator=(PdfObjectOutputStream&& rhs) noexcept
{
    utls::move(rhs.m_stream, m_stream);
    m_output = std::move(rhs.m_output);
    m_filters = std::move(rhs.m_filters);
    return *this;
}
