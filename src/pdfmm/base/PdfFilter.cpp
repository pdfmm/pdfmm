/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFilter.h"

#include <pdfmm/private/PdfFiltersPrivate.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfOutputStream.h"

using namespace std;
using namespace mm;

// All known filters
static const char* s_filters[] = {
    "ASCIIHexDecode",
    "ASCII85Decode",
    "LZWDecode",
    "FlateDecode",
    "RunLengthDecode",
    "CCITTFaxDecode",
    "JBIG2Decode",
    "DCTDecode",
    "JPXDecode",
    "Crypt",
};

static const char* s_shortFilters[] = {
    "AHx",
    "A85",
    "LZW",
    "Fl",
    "RL",
    "CCF",
    "", // There is no shortname for JBIG2Decode
    "DCT",
    "", // There is no shortname for JPXDecode
    "", // There is no shortname for Crypt
};

/** Create a filter that is a PdfOutputStream.
 *
 *  All data written to this stream is encoded using a
 *  filter and written to another PdfOutputStream.
 *
 *  The passed output stream is owned by this PdfOutputStream
 *  and deleted along with it.
 */
class PdfFilteredEncodeStream : public PdfOutputStream
{
private:
    void init(PdfOutputStream& outputStream, PdfFilterType filterType)
    {
        m_filter = PdfFilterFactory::Create(filterType);

        if (m_filter == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

        m_filter->BeginEncode(outputStream);
    }

public:
    PdfFilteredEncodeStream(PdfOutputStream& outputStream, PdfFilterType filterType)
    {
        init(outputStream, filterType);
    }

    PdfFilteredEncodeStream(unique_ptr<PdfOutputStream> outputStream, PdfFilterType filterType)
        : m_OutputStream(std::move(outputStream))
    {
        init(*outputStream, filterType);
    }

    /** Write data to the output stream
     *
     *  \param buffer the data is read from this buffer
     *  \param len the size of the buffer
     */
    void WriteImpl(const char* buffer, size_t len) override
    {
        m_filter->EncodeBlock({ buffer, len });
    }

    void Close() override
    {
        m_filter->EndEncode();
    }

private:
    unique_ptr<PdfOutputStream> m_OutputStream;
    unique_ptr<PdfFilter> m_filter;
};

/** Create a filter that is a PdfOutputStream.
 *
 *  All data written to this stream is decoded using a
 *  filter and written to another PdfOutputStream.
 *
 *  The passed output stream is owned by this PdfOutputStream
 *  and deleted along with it (optionally, see constructor).
 */
class PdfFilteredDecodeStream : public PdfOutputStream
{
private:
    void init(PdfOutputStream& outputStream, const PdfFilterType filterType,
        const PdfDictionary* decodeParms)
    {
        m_filter = PdfFilterFactory::Create(filterType);
        if (m_filter == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

        m_filter->BeginDecode(outputStream, decodeParms);
    }

public:
    PdfFilteredDecodeStream(PdfOutputStream& outputStream, const PdfFilterType filterType,
        const PdfDictionary* decodeParms)
        : m_FilterFailed(false)
    {
        init(outputStream, filterType, decodeParms);
    }

    PdfFilteredDecodeStream(unique_ptr<PdfOutputStream> outputStream, const PdfFilterType filterType,
        const PdfDictionary* decodeParms)
        : m_OutputStream(std::move(outputStream)), m_FilterFailed(false)
    {
        if (outputStream == nullptr)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Output stream must be not null");

        init(*outputStream, filterType, decodeParms);
    }

    /** Write data to the output stream
     *
     *  \param buffer the data is read from this buffer
     *  \param len the size of the buffer
     */
    void WriteImpl(const char* buffer, size_t len) override
    {
        try
        {
            m_filter->DecodeBlock({ buffer, len });
        }
        catch (PdfError& e)
        {
            PDFMM_PUSH_FRAME(e);
            m_FilterFailed = true;
            throw;
        }
    }

    void Close() override
    {
        try
        {
            if (!m_FilterFailed)
            {
                m_filter->EndDecode();
            }
        }
        catch (PdfError& e)
        {
            PDFMM_PUSH_FRAME_INFO(e, "PdfFilter::EndDecode() failed in filter of type {}",
                PdfFilterFactory::FilterTypeToName(m_filter->GetType()));
            m_FilterFailed = true;
            throw;
        }
    }

private:
    shared_ptr<PdfOutputStream> m_OutputStream;
    unique_ptr<PdfFilter> m_filter;
    bool m_FilterFailed;
};

//
// Actual PdfFilter code
//

PdfFilter::PdfFilter()
    : m_OutputStream(nullptr)
{
}

PdfFilter::~PdfFilter()
{
    // Whoops! Didn't call EndEncode() before destroying the filter!
    // Note that we can't do this for the user, since EndEncode() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    PDFMM_ASSERT(m_OutputStream == nullptr);
}

void PdfFilter::EncodeTo(charbuff& outBuffer, const bufferview& inBuffer) const
{
    if (!this->CanEncode())
        PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    PdfMemoryOutputStream stream;
    const_cast<PdfFilter*>(this)->BeginEncode(stream);
    const_cast<PdfFilter*>(this)->EncodeBlock(inBuffer);
    const_cast<PdfFilter*>(this)->EndEncode();
    outBuffer = stream.TakeBuffer();
}

void PdfFilter::DecodeTo(charbuff& outBuffer, const bufferview& inBuffer,
    const PdfDictionary* decodeParms) const
{
    if (!this->CanDecode())
        PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);

    PdfMemoryOutputStream stream;
    const_cast<PdfFilter*>(this)->BeginDecode(stream, decodeParms);
    const_cast<PdfFilter*>(this)->DecodeBlock(inBuffer);
    const_cast<PdfFilter*>(this)->EndDecode();
    outBuffer = stream.TakeBuffer();
}

//
// PdfFilterFactory code
//

unique_ptr<PdfFilter> PdfFilterFactory::Create(PdfFilterType filterType)
{
    PdfFilter* filter = nullptr;
    switch (filterType)
    {
        case PdfFilterType::None:
            break;

        case PdfFilterType::ASCIIHexDecode:
            filter = new PdfHexFilter();
            break;

        case PdfFilterType::ASCII85Decode:
            filter = new PdfAscii85Filter();
            break;

        case PdfFilterType::LZWDecode:
            filter = new PdfLZWFilter();
            break;

        case PdfFilterType::FlateDecode:
            filter = new PdfFlateFilter();
            break;

        case PdfFilterType::RunLengthDecode:
            filter = new PdfRLEFilter();
            break;

        case PdfFilterType::DCTDecode:
#ifdef PDFMM_HAVE_JPEG_LIB
            filter = new PdfDCTFilter();
            break;
#else
            break;
#endif // PDFMM_HAVE_JPEG_LIB

        case PdfFilterType::CCITTFaxDecode:
#ifdef PDFMM_HAVE_TIFF_LIB
            filter = new PdfCCITTFilter();
            break;
#else
            break;
#endif // PDFMM_HAVE_TIFF_LIB


        case PdfFilterType::JBIG2Decode:
        case PdfFilterType::JPXDecode:
        case PdfFilterType::Crypt:
        default:
            break;
    }

    return unique_ptr<PdfFilter>(filter);
}

unique_ptr<PdfOutputStream> PdfFilterFactory::CreateEncodeStream(const PdfFilterList& filters, PdfOutputStream& stream)
{
    PdfFilterList::const_iterator it = filters.begin();

    PDFMM_RAISE_LOGIC_IF(!filters.size(), "Cannot create an EncodeStream from an empty list of filters");

    unique_ptr<PdfOutputStream> filter(new PdfFilteredEncodeStream(stream, *it));
    it++;

    while (it != filters.end())
    {
        filter.reset(new PdfFilteredEncodeStream(std::move(filter), *it));
        it++;
    }

    return filter;
}

unique_ptr<PdfOutputStream> PdfFilterFactory::CreateDecodeStream(const PdfFilterList& filters, PdfOutputStream& stream,
    const PdfDictionary* dictionary)
{
    PdfFilterList::const_reverse_iterator it = filters.rbegin();

    PDFMM_RAISE_LOGIC_IF(filters.size() == 0, "Cannot create an DecodeStream from an empty list of filters");

    // TODO: Add also support for DP? (used in inline images)
    const PdfObject* decodeParams;
    if (dictionary != nullptr
        && (decodeParams = dictionary->FindKey("DecodeParms")) != nullptr
        && decodeParams->IsDictionary())
    {
        dictionary = &decodeParams->GetDictionary();
    }

    unique_ptr<PdfOutputStream> filterStream(new PdfFilteredDecodeStream(stream, *it, dictionary));
    it++;

    while (it != filters.rend())
    {
        filterStream.reset(new PdfFilteredDecodeStream(std::move(filterStream), *it, dictionary));
        it++;
    }

    return filterStream;
}

PdfFilterType PdfFilterFactory::FilterNameToType(const PdfName& name, bool supportShortNames)
{
    for (unsigned i = 0; i < std::size(s_filters); i++)
    {
        if (name == s_filters[i])
            return static_cast<PdfFilterType>(i + 1);
    }

    if (supportShortNames)
    {
        for (unsigned i = 0; i < std::size(s_shortFilters); i++)
        {
            if (name == s_shortFilters[i])
                return static_cast<PdfFilterType>(i + 1);
        }
    }

    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFilter, name.GetString());
}

const char* PdfFilterFactory::FilterTypeToName(PdfFilterType filterType)
{
    return s_filters[static_cast<unsigned>(filterType) - 1];
}

PdfFilterList PdfFilterFactory::CreateFilterList(const PdfObject& filtersObj)
{
    PdfFilterList filters;

    const PdfObject* filterKeyObj = nullptr;

    if (filtersObj.IsDictionary() && filtersObj.GetDictionary().HasKey("Filter"))
        filterKeyObj = filtersObj.GetDictionary().GetKey("Filter");
    else if (filtersObj.IsArray())
        filterKeyObj = &filtersObj;
    else if (filtersObj.IsName())
        filterKeyObj = &filtersObj;


    if (filterKeyObj == nullptr)
        // Object had no /Filter key . Return a null filter list.
        return filters;

    if (filterKeyObj->IsName())
    {
        filters.push_back(PdfFilterFactory::FilterNameToType(filterKeyObj->GetName()));
    }
    else if (filterKeyObj->IsArray())
    {
        for (auto& filter : filterKeyObj->GetArray())
        {
            if (filter.IsName())
            {
                filters.push_back(PdfFilterFactory::FilterNameToType(filter.GetName()));
            }
            else if (filter.IsReference())
            {
                auto filterObj = filtersObj.GetDocument()->GetObjects().GetObject(filter.GetReference());
                if (filterObj == nullptr)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Filter array contained unexpected reference");

                filters.push_back(PdfFilterFactory::FilterNameToType(filterObj->GetName()));
            }
            else
            {
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Filter array contained unexpected non-name type");
            }
        }
    }

    return filters;
}

void PdfFilter::BeginEncode(PdfOutputStream& output)
{
    PDFMM_RAISE_LOGIC_IF(m_OutputStream != nullptr, "BeginEncode() on failed filter or without EndEncode()");
    m_OutputStream = &output;

    try
    {
        BeginEncodeImpl();
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EncodeBlock(const bufferview& view)
{
    PDFMM_RAISE_LOGIC_IF(m_OutputStream == nullptr, "EncodeBlock() without BeginEncode() or on failed filter");

    try
    {
        EncodeBlockImpl(view.data(), view.size());
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EndEncode()
{
    PDFMM_RAISE_LOGIC_IF(m_OutputStream == nullptr, "EndEncode() without BeginEncode() or on failed filter");

    try
    {
        EndEncodeImpl();
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }

    m_OutputStream->Close();
    m_OutputStream = nullptr;
}

void PdfFilter::BeginDecode(PdfOutputStream& output, const PdfDictionary* decodeParms)
{
    PDFMM_RAISE_LOGIC_IF(m_OutputStream != nullptr, "BeginDecode() on failed filter or without EndDecode()");
    m_OutputStream = &output;

    try
    {
        BeginDecodeImpl(decodeParms);
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::DecodeBlock(const bufferview& view)
{
    PDFMM_RAISE_LOGIC_IF(m_OutputStream == nullptr, "DecodeBlock() without BeginDecode() or on failed filter")

    try
    {
        DecodeBlockImpl(view.data(), view.size());
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EndDecode()
{
    PDFMM_RAISE_LOGIC_IF(m_OutputStream == nullptr, "EndDecode() without BeginDecode() or on failed filter")

    try
    {
        EndDecodeImpl();
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME(e);
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
    try
    {
        if (m_OutputStream != nullptr)
        {
            m_OutputStream->Close();
            m_OutputStream = nullptr;
        }
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME_INFO(e, "Exception caught closing filter's output stream");
        // Closing stream failed, just get rid of it
        m_OutputStream = nullptr;
        throw;
    }
}

void PdfFilter::FailEncodeDecode()
{
    if (m_OutputStream != nullptr)
        m_OutputStream->Close();

    m_OutputStream = nullptr;
}
