/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfFiltersPrivate.h"

#include <podofo/base/PdfDictionary.h>
#include <podofo/base/PdfOutputDevice.h>
#include <podofo/base/PdfOutputStream.h>
#include <podofo/base/PdfTokenizer.h>

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#include <jerror.h>
}
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#ifdef WIN32		// For O_RDONLY
    // TODO: DS
#else
#include <sys/stat.h>
#include <fcntl.h>
#endif
}
#endif // PODOFO_HAVE_TIFF_LIB

using namespace std;
using namespace PoDoFo;

namespace PoDoFo {

// Private data for PdfAscii85Filter. This will be optimised
// by the compiler through compile-time constant expression
// evaluation.
const unsigned s_Powers85[] = { 85 * 85 * 85 * 85, 85 * 85 * 85, 85 * 85, 85, 1 };

/**
 * This structur contains all necessary values
 * for a FlateDecode and LZWDecode Predictor.
 * These values are normally stored in the /DecodeParams
 * key of a PDF dictionary.
 */
class PdfPredictorDecoder
{
public:
    PdfPredictorDecoder(const PdfDictionary* decodeParms)
    {
        m_Predictor = static_cast<int>(decodeParms->FindKeyAs<int64_t>("Predictor", 1));
        m_Colors = static_cast<int>(decodeParms->FindKeyAs<int64_t>("Colors", 1));
        m_BytesPerComponent = static_cast<int>(decodeParms->FindKeyAs<int64_t>("BitsPerComponent", 8));
        m_ColumnCount = static_cast<int>(decodeParms->FindKeyAs<int64_t>("Columns", 1));
        m_EarlyChange = static_cast<int>(decodeParms->FindKeyAs<int64_t>("EarlyChange", 1));

        if (m_Predictor >= 10)
        {
            m_NextByteIsPredictor = true;
            m_CurrPredictor = -1;
        }
        else
        {
            m_NextByteIsPredictor = false;
            m_CurrPredictor = m_Predictor;
        }

        m_CurrRowIndex = 0;
        m_BytesPerPixel = (m_BytesPerComponent * m_Colors) >> 3;
        m_Rows = (m_ColumnCount * m_Colors * m_BytesPerComponent) >> 3;

        m_Prev.resize(m_Rows);
        memset(m_Prev.data(), 0, sizeof(char) * m_Rows);

        m_UpperLeftPixelComponents.resize(m_BytesPerPixel);
        memset(m_UpperLeftPixelComponents.data(), 0, sizeof(char) * m_BytesPerPixel);
    }

    void Decode(const char* buffer, size_t len, PdfOutputStream* stream)
    {
        if (m_Predictor == 1)
        {
            stream->Write(buffer, len);
            return;
        }

        while (len--)
        {
            if (m_NextByteIsPredictor)
            {
                m_CurrPredictor = *buffer + 10;
                m_NextByteIsPredictor = false;
            }
            else
            {
                switch (m_CurrPredictor)
                {
                    case 2: // Tiff Predictor
                    {
                        if (m_BytesPerComponent == 8)
                        {   // Same as png sub
                            int prev = (m_CurrRowIndex - m_BytesPerPixel < 0
                                ? 0 : m_Prev[m_CurrRowIndex - m_BytesPerPixel]);
                            m_Prev[m_CurrRowIndex] = *buffer + prev;
                            break;
                        }

                        // TODO: implement tiff predictor for other than 8 BPC
                        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidPredictor, "tiff predictors other than 8 BPC are not implemented");
                        break;
                    }
                    case 10: // png none
                    {
                        m_Prev[m_CurrRowIndex] = *buffer;
                        break;
                    }
                    case 11: // png sub
                    {
                        int prev = (m_CurrRowIndex - m_BytesPerPixel < 0
                            ? 0 : m_Prev[m_CurrRowIndex - m_BytesPerPixel]);
                        m_Prev[m_CurrRowIndex] = *buffer + prev;
                        break;
                    }
                    case 12: // png up
                    {
                        m_Prev[m_CurrRowIndex] += *buffer;
                        break;
                    }
                    case 13: // png average
                    {
                        int prev = (m_CurrRowIndex - m_BytesPerPixel < 0
                            ? 0 : m_Prev[m_CurrRowIndex - m_BytesPerPixel]);
                        m_Prev[m_CurrRowIndex] = ((prev + m_Prev[m_CurrRowIndex]) >> 1) + *buffer;
                        break;
                    }
                    case 14: // png paeth
                    {
                        int nLeftByteIndex = m_CurrRowIndex - m_BytesPerPixel;

                        int a = nLeftByteIndex < 0 ? 0 : static_cast<unsigned char>(m_Prev[nLeftByteIndex]);
                        int b = static_cast<unsigned char>(m_Prev[m_CurrRowIndex]);

                        int nCurrComponentIndex = m_CurrRowIndex % m_BytesPerPixel;
                        int c = nLeftByteIndex < 0 ? 0 : static_cast<unsigned char>(m_UpperLeftPixelComponents[nCurrComponentIndex]);

                        int p = a + b - c;

                        int pa = p - a;
                        if (pa < 0)
                            pa = -pa;

                        int pb = p - b;
                        if (pb < 0)
                            pb = -pb;

                        int pc = p - c;
                        if (pc < 0)
                            pc = -pc;

                        char closestByte;
                        if (pa <= pb && pa <= pc)
                            closestByte = a;
                        else if (pb <= pc)
                            closestByte = b;
                        else
                            closestByte = c;

                        // Save the byte we're about to clobber for the next pixel's prediction
                        m_UpperLeftPixelComponents[nCurrComponentIndex] = m_Prev[m_CurrRowIndex];

                        m_Prev[m_CurrRowIndex] = *buffer + closestByte;
                        break;
                    }
                    case 15: // png optimum
                        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidPredictor, "png optimum predictor is not implemented");
                        break;

                    default:
                    {
                        //PODOFO_RAISE_ERROR( EPdfError::InvalidPredictor );
                        break;
                    }
                }

                m_CurrRowIndex++;
            }

            buffer++;

            if (m_CurrRowIndex >= m_Rows)
            {   // One line finished
                m_CurrRowIndex = 0;
                m_NextByteIsPredictor = (m_CurrPredictor >= 10);
                stream->Write(m_Prev.data(), m_Rows);
            }
        }
    }

private:
    int m_Predictor;
    int m_Colors;
    int m_BytesPerComponent;
    int m_ColumnCount;
    int m_EarlyChange;
    int m_BytesPerPixel;     // Bytes per pixel

    int m_CurrPredictor;
    int m_CurrRowIndex;
    int m_Rows;

    bool m_NextByteIsPredictor;

    buffer_t m_Prev;

    // The PNG Paeth predictor uses the values of the pixel above and to the left
    // of the current pixel. But we overwrite the row above as we go, so we'll
    // have to store the bytes of the upper-left pixel separately.
    buffer_t m_UpperLeftPixelComponents;
};

} // end anonymous namespace

#pragma region PdfHexFilter

PdfHexFilter::PdfHexFilter()
    : m_DecodedByte(0), m_Low(true)
{
}

void PdfHexFilter::EncodeBlockImpl(const char* buffer, size_t len)
{
    char data[2];
    while (len--)
    {
        data[0] = (*buffer & 0xF0) >> 4;
        data[0] += (data[0] > 9 ? 'A' - 10 : '0');

        data[1] = (*buffer & 0x0F);
        data[1] += (data[1] > 9 ? 'A' - 10 : '0');

        GetStream()->Write(data, 2);

        buffer++;
    }
}

void PdfHexFilter::BeginDecodeImpl(const PdfDictionary*)
{
    m_DecodedByte = 0;
    m_Low = true;
}

void PdfHexFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    char val;

    while (len--)
    {
        if (PdfTokenizer::IsWhitespace(*buffer))
        {
            buffer++;
            continue;
        }

        val = PdfTokenizer::GetHexValue(*buffer);
        if (m_Low)
        {
            m_DecodedByte = (val & 0x0F);
            m_Low = false;
        }
        else
        {
            m_DecodedByte = ((m_DecodedByte << 4) | val);
            m_Low = true;

            GetStream()->Write(&m_DecodedByte, 1);
        }

        buffer++;
    }
}

void PdfHexFilter::EndDecodeImpl()
{
    if (!m_Low)
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        GetStream()->Write(&m_DecodedByte, 1);
    }
}

#pragma endregion // PdfHexFilter

#pragma region PdfAscii85Filter

// Ascii 85
// 
// based on public domain software from:
// Paul Haahr - http://www.webcom.com/~haahr/

PdfAscii85Filter::PdfAscii85Filter()
    : m_count(0), m_tuple(0)
{
}

void PdfAscii85Filter::EncodeTuple(unsigned tuple, int count)
{
    int i = 5;
    int z = 0;
    char buf[5];
    char out[5];
    char* start = buf;;

    do
    {
        *start++ = static_cast<char>(tuple % 85);
        tuple /= 85;
    } while (--i > 0);

    i = count;
    do
    {
        out[z++] = static_cast<unsigned char>(*--start) + '!';
    } while (i-- > 0);

    GetStream()->Write(out, z);
}

void PdfAscii85Filter::BeginEncodeImpl()
{
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::EncodeBlockImpl(const char* buffer, size_t len)
{
    unsigned c;
    const char* z = "z";

    while (len)
    {
        c = *buffer & 0xFF;
        switch (m_count++) {
            case 0: m_tuple |= (c << 24); break;
            case 1: m_tuple |= (c << 16); break;
            case 2: m_tuple |= (c << 8); break;
            case 3:
                m_tuple |= c;
                if (m_tuple == 0)
                {
                    GetStream()->Write(z, 1);
                }
                else
                {
                    this->EncodeTuple(m_tuple, m_count);
                }

                m_tuple = 0;
                m_count = 0;
                break;
        }
        len--;
        buffer++;
    }
}

void PdfAscii85Filter::EndEncodeImpl()
{
    if (m_count > 0)
        this->EncodeTuple(m_tuple, m_count);
    //GetStream()->Write( "~>", 2 );
}

void PdfAscii85Filter::BeginDecodeImpl(const PdfDictionary*)
{
    m_count = 0;
    m_tuple = 0;
}

void PdfAscii85Filter::DecodeBlockImpl(const char* buffer, size_t len)
{
    bool foundEndMarker = false;

    while (len != 0 && !foundEndMarker)
    {
        switch (*buffer)
        {
            default:
                if (*buffer < '!' || *buffer > 'u')
                {
                    PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
                }

                m_tuple += (*buffer - '!') * s_Powers85[m_count++];
                if (m_count == 5)
                {
                    WidePut(m_tuple, 4);
                    m_count = 0;
                    m_tuple = 0;
                }
                break;
            case 'z':
                if (m_count != 0)
                {
                    PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
                }

                this->WidePut(0, 4);
                break;
            case '~':
                buffer++;
                len--;
                if (len != 0 && *buffer != '>')
                    PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);

                foundEndMarker = true;
                break;
            case '\n': case '\r': case '\t': case ' ':
            case '\0': case '\f': case '\b': case 0177:
                break;
        }

        len--;
        buffer++;
    }
}

void PdfAscii85Filter::EndDecodeImpl()
{
    if (m_count > 0)
    {
        m_count--;
        m_tuple += s_Powers85[m_count];
        WidePut(m_tuple, m_count);
    }
}

void PdfAscii85Filter::WidePut(unsigned tuple, int bytes) const
{
    char data[4];

    switch (bytes)
    {
        case 4:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >> 8);
            data[3] = static_cast<char>(tuple);
            break;
        case 3:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            data[2] = static_cast<char>(tuple >> 8);
            break;
        case 2:
            data[0] = static_cast<char>(tuple >> 24);
            data[1] = static_cast<char>(tuple >> 16);
            break;
        case 1:
            data[0] = static_cast<char>(tuple >> 24);
            break;
    }

    GetStream()->Write(data, bytes);
}

#pragma endregion // PdfAscii85Filter

#pragma endregion PdfFlateFilter

PdfFlateFilter::PdfFlateFilter()
    : m_Predictor(0)
{
    memset(m_buffer, 0, sizeof(m_buffer));
    memset(&m_stream, 0, sizeof(m_stream));
}

PdfFlateFilter::~PdfFlateFilter()
{
    delete m_Predictor;
}

void PdfFlateFilter::BeginEncodeImpl()
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;

    if (deflateInit(&m_stream, Z_DEFAULT_COMPRESSION))
        PODOFO_RAISE_ERROR(EPdfError::Flate);
}

void PdfFlateFilter::EncodeBlockImpl(const char* buffer, size_t len)
{
    this->EncodeBlockInternal(buffer, len, Z_NO_FLUSH);
}

void PdfFlateFilter::EncodeBlockInternal(const char* buffer, size_t len, int nMode)
{
    int nWrittenData = 0;

    m_stream.avail_in = static_cast<unsigned>(len);
    m_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(buffer));

    do
    {
        m_stream.avail_out = PODOFO_FILTER_INTERNAL_BUFFER_SIZE;
        m_stream.next_out = m_buffer;

        if (deflate(&m_stream, nMode) == Z_STREAM_ERROR)
        {
            FailEncodeDecode();
            PODOFO_RAISE_ERROR(EPdfError::Flate);
        }

        nWrittenData = PODOFO_FILTER_INTERNAL_BUFFER_SIZE - m_stream.avail_out;
        try
        {
            if (nWrittenData > 0)
            {
                GetStream()->Write(reinterpret_cast<char*>(m_buffer), nWrittenData);
            }
        }
        catch (PdfError& e)
        {
            // clean up after any output stream errors
            FailEncodeDecode();
            e.AddToCallstack(__FILE__, __LINE__);
            throw e;
        }
    } while (m_stream.avail_out == 0);
}

void PdfFlateFilter::EndEncodeImpl()
{
    this->EncodeBlockInternal(nullptr, 0, Z_FINISH);
    deflateEnd(&m_stream);
}

void PdfFlateFilter::BeginDecodeImpl(const PdfDictionary* pDecodeParms)
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;

    m_Predictor = pDecodeParms ? new PdfPredictorDecoder(pDecodeParms) : nullptr;

    if (inflateInit(&m_stream) != Z_OK)
        PODOFO_RAISE_ERROR(EPdfError::Flate);
}

void PdfFlateFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    int flateErr;
    unsigned writtenDataSize;

    m_stream.avail_in = static_cast<unsigned>(len);
    m_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(buffer));

    do
    {
        m_stream.avail_out = PODOFO_FILTER_INTERNAL_BUFFER_SIZE;
        m_stream.next_out = m_buffer;

        switch ((flateErr = inflate(&m_stream, Z_NO_FLUSH)))
        {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                PdfError::LogMessage(LogSeverity::Error, "Flate Decoding Error from ZLib: %i", flateErr);
                (void)inflateEnd(&m_stream);

                FailEncodeDecode();
                PODOFO_RAISE_ERROR(EPdfError::Flate);
            }
            default:
                break;
        }

        writtenDataSize = PODOFO_FILTER_INTERNAL_BUFFER_SIZE - m_stream.avail_out;
        try
        {
            if (m_Predictor)
                m_Predictor->Decode(reinterpret_cast<char*>(m_buffer), writtenDataSize, GetStream());
            else
                GetStream()->Write(reinterpret_cast<char*>(m_buffer), writtenDataSize);
        }
        catch (PdfError& e)
        {
            // clean up after any output stream errors
            FailEncodeDecode();
            e.AddToCallstack(__FILE__, __LINE__);
            throw e;
        }
    } while (m_stream.avail_out == 0);
}

void PdfFlateFilter::EndDecodeImpl()
{
    delete m_Predictor;
    m_Predictor = nullptr;

    (void)inflateEnd(&m_stream);
}

#pragma endregion // PdfFlateFilter

#pragma region PdfRLEFilter

PdfRLEFilter::PdfRLEFilter()
    : m_CodeLen(0)
{
}

void PdfRLEFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfRLEFilter::EncodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfRLEFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfRLEFilter::BeginDecodeImpl(const PdfDictionary*)
{
    m_CodeLen = 0;
}

void PdfRLEFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    while (len--)
    {
        if (!m_CodeLen)
        {
            m_CodeLen = static_cast<int>(*buffer);
        }
        else if (m_CodeLen == 128)
        {
            break;
        }
        else if (m_CodeLen <= 127)
        {
            GetStream()->Write(buffer, 1);
            m_CodeLen--;
        }
        else if (m_CodeLen >= 129)
        {
            m_CodeLen = 257 - m_CodeLen;

            while (m_CodeLen--)
                GetStream()->Write(buffer, 1);
        }

        buffer++;
    }
}

#pragma endregion // PdfRLEFilter

#pragma region PdfLZWFilter

const unsigned short PdfLZWFilter::s_masks[] = { 0x01FF, 0x03FF, 0x07FF, 0x0FFF };
const unsigned short PdfLZWFilter::s_clear = 0x0100;      // clear table
const unsigned short PdfLZWFilter::s_eod = 0x0101;      // end of data

PdfLZWFilter::PdfLZWFilter()
    : m_mask(0),
    m_code_len(0),
    m_character(0),
    m_First(false),
    m_Predictor(0)
{
}

PdfLZWFilter::~PdfLZWFilter()
{
    delete m_Predictor;
}

void PdfLZWFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfLZWFilter::EncodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfLZWFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfLZWFilter::BeginDecodeImpl(const PdfDictionary* pDecodeParms)
{
    m_mask = 0;
    m_code_len = 9;
    m_character = 0;

    m_First = true;

    m_Predictor = pDecodeParms ? new PdfPredictorDecoder(pDecodeParms) : nullptr;

    InitTable();
}

void PdfLZWFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    unsigned buffer_size = 0;
    const unsigned buffer_max = 24;

    uint32_t old = 0;
    uint32_t code = 0;
    uint32_t codeBuff = 0;

    TLzwItem item;

    vector<unsigned char> data;
    if (m_First)
    {
        m_character = *buffer;
        m_First = false;
    }

    while (len)
    {
        // Fill the buffer
        while (buffer_size <= (buffer_max - 8) && len)
        {
            codeBuff <<= 8;
            codeBuff |= static_cast<uint32_t>(static_cast<unsigned char>(*buffer));
            buffer_size += 8;

            buffer++;
            len--;
        }

        // read from the buffer
        while (buffer_size >= m_code_len)
        {
            code = (codeBuff >> (buffer_size - m_code_len)) & PdfLZWFilter::s_masks[m_mask];
            buffer_size -= m_code_len;

            if (code == PdfLZWFilter::s_clear)
            {
                m_mask = 0;
                m_code_len = 9;

                InitTable();
            }
            else if (code == PdfLZWFilter::s_eod)
            {
                len = 0;
                break;
            }
            else
            {
                if (code >= m_table.size())
                {
                    if (old >= m_table.size())
                    {
                        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
                    }
                    data = m_table[old].value;
                    data.push_back(m_character);
                }
                else
                    data = m_table[code].value;

                // Write data to the output device
                if (m_Predictor)
                    m_Predictor->Decode(reinterpret_cast<char*>(&(data[0])), data.size(), GetStream());
                else
                    GetStream()->Write(reinterpret_cast<char*>(&(data[0])), data.size());

                m_character = data[0];
                if (old < m_table.size()) // fix the first loop
                    data = m_table[old].value;
                data.push_back(m_character);

                item.value = data;
                m_table.push_back(item);

                old = code;

                switch (m_table.size())
                {
                    case 511:
                    case 1023:
                    case 2047:
                        m_code_len++;
                        m_mask++;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void PdfLZWFilter::EndDecodeImpl()
{
    delete m_Predictor;
    m_Predictor = nullptr;
}

void PdfLZWFilter::InitTable()
{
    constexpr unsigned LZW_TABLE_SIZE = 4096;
    TLzwItem item;

    m_table.clear();
    m_table.reserve(LZW_TABLE_SIZE);

    for (int i = 0; i <= 255; i++)
    {
        item.value.clear();
        item.value.push_back(static_cast<unsigned char>(i));
        m_table.push_back(item);
    }

    // Add dummy entry, which is never used by decoder
    item.value.clear();
    m_table.push_back(item);
}

#pragma endregion // PdfLZWFilter


#pragma region PdfDCTFilter

#ifdef PODOFO_HAVE_JPEG_LIB

/*
 * Handlers for errors inside the JPeg library
 */
extern "C"
{
    void JPegErrorExit(j_common_ptr cinfo)
    {
        auto& error = *reinterpret_cast<string*>(cinfo->client_data);
        error.resize(JMSG_LENGTH_MAX);
        /* Create the message */
        (*cinfo->err->format_message) (cinfo, error.data());

        jpeg_destroy(cinfo);
    }

    void JPegErrorOutput(j_common_ptr, int)
    {
    }
};

/*
 * The actual filter implementation
 */
PdfDCTFilter::PdfDCTFilter()
    : m_Device(nullptr)
{
    memset(&m_cinfo, 0, sizeof(struct jpeg_decompress_struct));
    memset(&m_jerr, 0, sizeof(struct jpeg_error_mgr));
}

void PdfDCTFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfDCTFilter::EncodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfDCTFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfDCTFilter::BeginDecodeImpl(const PdfDictionary*)
{
    // Setup variables for JPEGLib
    string error;
    m_cinfo.client_data = &error;
    m_cinfo.err = jpeg_std_error(&m_jerr);
    m_jerr.error_exit = &JPegErrorExit;
    m_jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&m_cinfo);

    if (error.length() != 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedImageFormat, error);

    m_Device = new PdfOutputDevice(m_buffer);
}

void PdfDCTFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    m_Device->Write(buffer, len);
}

void PdfDCTFilter::EndDecodeImpl()
{
    delete m_Device;
    m_Device = nullptr;

    PoDoFo::jpeg_memory_src(&m_cinfo, reinterpret_cast<JOCTET*>(m_buffer.GetBuffer()), m_buffer.GetSize());

    if (jpeg_read_header(&m_cinfo, TRUE) <= 0)
    {
        jpeg_destroy_decompress(&m_cinfo);

        PODOFO_RAISE_ERROR(EPdfError::UnexpectedEOF);
    }

    jpeg_start_decompress(&m_cinfo);

    unsigned rowBytes = (unsigned)(m_cinfo.output_width * m_cinfo.output_components);
    const int iComponents = m_cinfo.output_components;

    // buffer will be deleted by jpeg_destroy_decompress
    JSAMPARRAY scanLines = (*m_cinfo.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&m_cinfo), JPOOL_IMAGE, rowBytes, 1);
    buffer_t buffer(rowBytes);
    while (m_cinfo.output_scanline < m_cinfo.output_height)
    {
        jpeg_read_scanlines(&m_cinfo, scanLines, 1);
        if (iComponents == 4)
        {
            for (unsigned i = 0, c = 0; i < m_cinfo.output_width; i++, c += 4)
            {
                buffer[c] = scanLines[0][i * 4];
                buffer[c + 1] = scanLines[0][i * 4 + 1];
                buffer[c + 2] = scanLines[0][i * 4 + 2];
                buffer[c + 3] = scanLines[0][i * 4 + 3];
            }
        }
        else if (iComponents == 3)
        {
            for (unsigned i = 0, c = 0; i < m_cinfo.output_width; i++, c += 3)
            {
                buffer[c] = scanLines[0][i * 3];
                buffer[c + 1] = scanLines[0][i * 3 + 1];
                buffer[c + 2] = scanLines[0][i * 3 + 2];
            }
        }
        else if (iComponents == 1)
        {
            memcpy(buffer.data(), scanLines[0], m_cinfo.output_width);
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "DCTDecode unknown components");
        }

        GetStream()->Write(reinterpret_cast<char*>(buffer.data()), rowBytes);
    }

    jpeg_destroy_decompress(&m_cinfo);
}

/*
 * memsrc.c
 *
 * Copyright (C) 1994-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression data source routines for the case of
 * reading JPEG data from a memory buffer that is preloaded with the entire
 * JPEG file. This would not seem especially useful at first sight, but
 * a number of people have asked for it.
 * This is really just a stripped-down version of jdatasrc.c. Comparison
 * of this code with jdatasrc.c may be helpful in seeing how to make
 * custom source managers for other purposes.
*/

/* Expanded data source object for memory input */
typedef struct
{
    struct jpeg_source_mgr pub; /* public fields */
    JOCTET eoi_buffer[2]; /* a place to put a dummy EOI */
} my_source_mgr;

typedef my_source_mgr* my_src_ptr;

/*
 * Initialize source, called by jpeg_read_header
 * before any data is actually read.
 */

METHODDEF(void) init_source(j_decompress_ptr)
{
    /* No work, since jpeg_memory_src set up the buffer pointer and count.
     * Indeed, if we want to read multiple JPEG images from one buffer,
     * this *must* not do anything to the pointer.
     */
}

/*
 * Fill the input buffer, called whenever buffer is emptied.
 *
 * In this application, this routine should never be called; if it is called,
 * the decompressor has overrun the end of the input buffer, implying we
 * supplied an incomplete or corrupt JPEG datastream. A simple error exit
 * might be the most appropriate response.
 *
 * But what we choose to do in this code is to supply dummy EOI markers
 * in order to force the decompressor to finish processing and supply
 * some sort of output image, no matter how corrupted.
 */

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
{
    my_src_ptr src = reinterpret_cast<my_src_ptr>(cinfo->src);
    WARNMS(cinfo, JWRN_JPEG_EOF);

    /* Create a fake EOI marker */
    src->eoi_buffer[0] = static_cast<JOCTET>(0xFF);
    src->eoi_buffer[1] = static_cast<JOCTET>(JPEG_EOI);
    src->pub.next_input_byte = src->eoi_buffer;
    src->pub.bytes_in_buffer = 2;

    return TRUE;
}

/*
 * Skip data, used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * If we overrun the end of the buffer, we let fill_input_buffer deal with
 * it. An extremely large skip could cause some time-wasting here, but
 * it really isn't supposed to happen ... and the decompressor will never
 * skip more than 64K anyway.
 */
METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    my_src_ptr src = reinterpret_cast<my_src_ptr>(cinfo->src);

    if (num_bytes > 0)
    {
        while (num_bytes > static_cast<long>(src->pub.bytes_in_buffer))
        {
            num_bytes -= static_cast<long>(src->pub.bytes_in_buffer);
            fill_input_buffer(cinfo);
            /* note we assume that fill_input_buffer will never return FALSE,
             * so suspension need not be handled.
             */
        }

        src->pub.next_input_byte += static_cast<size_t>(num_bytes);
        src->pub.bytes_in_buffer -= static_cast<size_t>(num_bytes);
    }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library. That method assumes that no backtracking
 * is possible.
 */

/*
 * Terminate source, called by jpeg_finish_decompress
 * after all data has been read. Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
METHODDEF(void) term_source(j_decompress_ptr)
{
    /* no work necessary here */
}

/*
 * Prepare for input from a memory buffer.
 */
void PoDoFo::jpeg_memory_src(j_decompress_ptr cinfo, const JOCTET* buffer, size_t bufsize)
{
    my_src_ptr src;

    /* The source object is made permanent so that a series of JPEG images
     * can be read from a single buffer by calling jpeg_memory_src
     * only before the first one.
     * This makes it unsafe to use this manager and a different source
     * manager serially with the same JPEG object. Caveat programmer.
     */

    if (cinfo->src == nullptr)
    {
        // first time for this JPEG object?
        cinfo->src = static_cast<struct jpeg_source_mgr*>(
            (*cinfo->mem->alloc_small) (reinterpret_cast<j_common_ptr>(cinfo), JPOOL_PERMANENT,
                sizeof(my_source_mgr)));
    }

    src = reinterpret_cast<my_src_ptr>(cinfo->src);
    src->pub.init_source = init_source;
    src->pub.fill_input_buffer = fill_input_buffer;
    src->pub.skip_input_data = skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->pub.term_source = term_source;

    src->pub.next_input_byte = buffer;
    src->pub.bytes_in_buffer = bufsize;
}

#endif // PODOFO_HAVE_JPEG_LIB

#pragma endregion // PdfDCTFilter

#pragma region PdfCCITTFilter

#ifdef PODOFO_HAVE_TIFF_LIB

#ifdef DS_CCITT_DEVELOPMENT_CODE

static tsize_t dummy_read(thandle_t, tdata_t, tsize_t)
{
    return 0;
}

static tsize_t dummy_write(thandle_t, tdata_t, tsize_t size)
{
    return size;
}

static toff_t dummy_seek(thandle_t, toff_t, int)
{

}

static int dummy_close(thandle_t)
{

}

static toff_t dummy_size(thandle_t)
{

}

#endif

PdfCCITTFilter::PdfCCITTFilter()
    : m_tiff(nullptr)
{
}

void PdfCCITTFilter::BeginEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfCCITTFilter::EncodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfCCITTFilter::EndEncodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfCCITTFilter::BeginDecodeImpl(const PdfDictionary* pDict)
{
#ifdef DS_CCITT_DEVELOPMENT_CODE

    if (pDict == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "PdfCCITTFilter required a DecodeParms dictionary");

    m_tiff = TIFFClientOpen("podofo", "w", reinterpret_cast<thandle_t>(-1),
        dummy_read, dummy_write,
        dummy_seek, dummy_close, dummy_size, nullptr, nullptr);

    if (m_tiff == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "TIFFClientOpen failed");

    m_tiff->tif_mode = O_RDONLY;

    TIFFSetField(m_tiff, TIFFTAG_IMAGEWIDTH, pDict->GetAs<int64_t>(PdfName("Columns"), 1728));
    TIFFSetField(m_tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE, 1);
    TIFFSetField(m_tiff, TIFFTAG_FILLORDER, FILLORDER_LSB2MSB);
    TIFFSetField(m_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
    TIFFSetField(m_tiff, TIFFTAG_YRESOLUTION, 196.);
    TIFFSetField(m_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);

    /*
    m_tiff->tif_scanlinesize = TIFFSetField(m_tiff);

    if (pDict != nullptr)
    {
        int64_t lEncoding = pDict->GetAs<int64_t>(PdfName("K"), 0);
        if (lEncoding == 0) // pure 1D encoding, Group3 1D
        {
            TIFFSetField(faxTIFF, TIFFTAG_GROUP3OPTIONS, GROUP3OPT_1DENCODING);
        }
        else if (lEncoding < 0) // pure 2D encoding, Group4
        {
            TIFFSetField(faxTIFF, TIFFTAG_GROUP4OPTIONS, GROUP4OPT_2DENCODING);
        }
        else //if (lEncoding > 0)  // mixed, Group3 2D
        {
            TIFFSetField(faxTIFF, TIFFTAG_GROUP3OPTIONS, GROUP3OPT_2DENCODING);
        }
    }
    */

#else // DS_CCITT_DEVELOPMENT_CODE
    (void)pDict;
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
#endif // DS_CCITT_DEVELOPMENT_CODE
}

void PdfCCITTFilter::DecodeBlockImpl(const char*, size_t)
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

void PdfCCITTFilter::EndDecodeImpl()
{
    PODOFO_RAISE_ERROR(EPdfError::UnsupportedFilter);
}

#endif // PODOFO_HAVE_TIFF_LIB

#pragma endregion // PdfCCITTFilter
