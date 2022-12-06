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

static bool isMediaFilter(PdfFilterType filterType);
static PdfFilterList stripMediaFilters(const PdfFilterList& filters, PdfFilterList& mediaFilters);

PdfObjectStream::PdfObjectStream(PdfObject& parent)
    : m_Parent(&parent), m_locked(false)
{
}

PdfObjectStream::~PdfObjectStream() { }

PdfObjectOutputStream PdfObjectStream::GetOutputStreamRaw(bool append)
{
    EnsureClosed();
    return PdfObjectOutputStream(*this, PdfFilterList(), append);
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

PdfObjectInputStream PdfObjectStream::GetInputStream(bool raw) const
{
    EnsureClosed();
    return PdfObjectInputStream(const_cast<PdfObjectStream&>(*this), raw);
}

void PdfObjectStream::CopyTo(charbuff& buffer, bool raw) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    CopyTo(stream, raw);
}

void PdfObjectStream::CopyToSafe(charbuff& buffer) const
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    CopyToSafe(stream);
}

void PdfObjectStream::CopyTo(OutputStream& stream, bool raw) const
{
    PdfFilterList mediaFilters;
    auto inputStream = const_cast<PdfObjectStream&>(*this).getInputStream(raw, mediaFilters);
    if (mediaFilters.size() != 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Unsupported expansion with media filters. Use GetInputStream(true) instead");
        
    inputStream->CopyTo(stream);
    stream.Flush();
}

void PdfObjectStream::CopyToSafe(OutputStream& stream) const
{
    PdfFilterList mediaFilters;
    auto inputStream = const_cast<PdfObjectStream&>(*this).getInputStream(false, mediaFilters);
    inputStream->CopyTo(stream);
    stream.Flush();
}

charbuff PdfObjectStream::GetCopy(bool raw) const
{
    charbuff ret;
    StringStreamDevice stream(ret);
    CopyTo(stream, raw);
    return ret;
}

charbuff PdfObjectStream::GetCopySafe() const
{
    charbuff ret;
    StringStreamDevice stream(ret);
    CopyToSafe(stream);
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
    CopyDataFrom(rhs);
    return (*this);
}

void PdfObjectStream::CopyDataFrom(const PdfObjectStream& rhs)
{
    auto stream = const_cast<PdfObjectStream&>(rhs).getInputStream();
    this->SetData(*stream, true);
    CopyFrom(rhs);
}

void PdfObjectStream::CopyFrom(const PdfObjectStream& rhs)
{
    // Just set the filters
    m_Filters = rhs.m_Filters;
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
        setData(stream, { }, -1, true);
    else
        setData(stream, { DefaultFilter }, -1, true);
}

void PdfObjectStream::SetData(InputStream& stream, const PdfFilterList& filters)
{
    EnsureClosed();
    setData(stream, filters, -1, true);
}

unique_ptr<InputStream> PdfObjectStream::getInputStream(bool raw, PdfFilterList& mediaFilters)
{
    if (raw)
    {
        return getInputStream();
    }
    else
    {
        auto filters = stripMediaFilters(m_Filters, mediaFilters);
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

void PdfObjectStream::InitData(InputStream& stream, size_t size, PdfFilterList&& filterList)
{
    PdfObjectOutputStream output(*this);
    stream.CopyTo(output, size);
    m_Filters = std::move(filterList);
}

void PdfObjectStream::EnsureClosed() const
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

PdfObjectInputStream::PdfObjectInputStream(PdfObjectStream& stream, bool raw)
    : m_stream(&stream)
{
    m_stream->m_locked = true;
    m_input = stream.getInputStream(raw, m_MediaFilters);
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
    {
        // Set filters on the stream and on the parent object
        // NOTE: if filters are not defined assume we will
        // preserve them on the parent
        if (m_filters.has_value())
        {
            auto& filters = *m_filters;
            if (filters.size() == 0)
            {
                m_stream->GetParent().GetDictionary().RemoveKey(PdfName::KeyFilter);
            }
            else if (filters.size() == 1)
            {
                m_stream->GetParent().GetDictionary().AddKey(PdfName::KeyFilter,
                    PdfName(mm::FilterToName(filters.front())));
            }
            else // filters.size() > 1
            {
                PdfArray arrFilters;
                for (auto filterType : filters)
                    arrFilters.Add(PdfName(mm::FilterToName(filterType)));

                m_stream->GetParent().GetDictionary().AddKey(PdfName::KeyFilter, arrFilters);
            }

            m_stream->m_Filters = std::move(filters);
        }

        // Unlock the stream
        m_stream->m_locked = false;
    }
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectOutputStream&& rhs) noexcept
    : m_filters(std::move(rhs.m_filters))
{
    utls::move(rhs.m_stream, m_stream);
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream,
        PdfFilterList&& filters, bool append)
    : PdfObjectOutputStream(stream, nullable<PdfFilterList>(std::move(filters)), append)
{
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream)
    : PdfObjectOutputStream(stream, nullptr, false)
{
}

PdfObjectOutputStream::PdfObjectOutputStream(PdfObjectStream& stream,
        nullable<PdfFilterList> filters, bool append)
    : m_stream(&stream), m_filters(std::move(filters))
{
    auto document = stream.GetParent().GetDocument();
    if (document != nullptr)
        document->GetObjects().BeginAppendStream(stream);

    charbuff buffer;
    if (append)
        stream.CopyTo(buffer);

    if (m_filters.has_value())
    {
        auto& filters = *m_filters;
        if (filters.size() == 0)
        {
            m_output = stream.getOutputStream();
        }
        else
        {
            m_output = PdfFilterFactory::CreateEncodeStream(
                stream.getOutputStream(), filters);
        }
    }
    else
    {
        m_output = stream.getOutputStream();
    }

    m_stream->m_locked = true;

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

// Strip media filters from regular ones
PdfFilterList stripMediaFilters(const PdfFilterList& filters, PdfFilterList& mediaFilters)
{
    PdfFilterList ret;
    for (unsigned i = 0; i < filters.size(); i++)
    {
        PdfFilterType type = filters[i];
        if (isMediaFilter(type))
        {
            mediaFilters.push_back(type);
        }
        else
        {
            if (mediaFilters.size() != 0)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, "Inconsistent filter with regular filters after media ones");

            ret.push_back(type);
        }
    }

    return ret;
}

bool isMediaFilter(PdfFilterType filterType)
{
    switch (filterType)
    {
        case PdfFilterType::ASCIIHexDecode:
        case PdfFilterType::ASCII85Decode:
        case PdfFilterType::LZWDecode:
        case PdfFilterType::FlateDecode:
        case PdfFilterType::RunLengthDecode:
        case PdfFilterType::Crypt:
            return false;
        case PdfFilterType::CCITTFaxDecode:
        case PdfFilterType::JBIG2Decode:
        case PdfFilterType::DCTDecode:
        case PdfFilterType::JPXDecode:
            return true;
        case PdfFilterType::None:
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}
