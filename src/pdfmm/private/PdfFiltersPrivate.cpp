/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDeclarationsPrivate.h"
#include "PdfFiltersPrivate.h"

#include <pdfmm/base/PdfDictionary.h>
#include <pdfmm/base/PdfTokenizer.h>
#include <pdfmm/base/PdfStreamDevice.h>

using namespace std;
using namespace mm;

namespace mm {

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
    PdfPredictorDecoder(const PdfDictionary& decodeParms)
    {
        m_Predictor = static_cast<int>(decodeParms.FindKeyAs<int64_t>("Predictor", 1));
        m_Colors = static_cast<int>(decodeParms.FindKeyAs<int64_t>("Colors", 1));
        m_BytesPerComponent = static_cast<int>(decodeParms.FindKeyAs<int64_t>("BitsPerComponent", 8));
        m_ColumnCount = static_cast<int>(decodeParms.FindKeyAs<int64_t>("Columns", 1));
        m_EarlyChange = static_cast<int>(decodeParms.FindKeyAs<int64_t>("EarlyChange", 1));

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

    void Decode(const char* buffer, size_t len, OutputStream* stream)
    {
        if (m_Predictor == 1)
        {
            stream->Write(buffer, len);
            return;
        }

        while (len-- != 0)
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
                        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidPredictor, "tiff predictors other than 8 BPC are not implemented");
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
                        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidPredictor, "png optimum predictor is not implemented");
                        break;

                    default:
                    {
                        //PDFMM_RAISE_ERROR( EPdfError::InvalidPredictor );
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

    charbuff m_Prev;

    // The PNG Paeth predictor uses the values of the pixel above and to the left
    // of the current pixel. But we overwrite the row above as we go, so we'll
    // have to store the bytes of the upper-left pixel separately.
    charbuff m_UpperLeftPixelComponents;
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
    unsigned char val;
    while (len--)
    {
        if (PdfTokenizer::IsWhitespace(*buffer))
        {
            buffer++;
            continue;
        }

        (void)utls::TryGetHexValue(*buffer, val);
        if (m_Low)
        {
            m_DecodedByte = (char)(val & 0x0F);
            m_Low = false;
        }
        else
        {
            m_DecodedByte = (char)((m_DecodedByte << 4) | val);
            m_Low = true;

            GetStream()->Write(m_DecodedByte);
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
        GetStream()->Write(m_DecodedByte);
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
                    PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
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
                    PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                }

                this->WidePut(0, 4);
                break;
            case '~':
                buffer++;
                len--;
                if (len != 0 && *buffer != '>')
                    PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);

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
{
    memset(m_buffer, 0, sizeof(m_buffer));
    memset(&m_stream, 0, sizeof(m_stream));
}

void PdfFlateFilter::BeginEncodeImpl()
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;

    if (deflateInit(&m_stream, Z_DEFAULT_COMPRESSION))
        PDFMM_RAISE_ERROR(PdfErrorCode::Flate);
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
        m_stream.avail_out = BUFFER_SIZE;
        m_stream.next_out = m_buffer;

        if (deflate(&m_stream, nMode) == Z_STREAM_ERROR)
        {
            FailEncodeDecode();
            PDFMM_RAISE_ERROR(PdfErrorCode::Flate);
        }

        nWrittenData = BUFFER_SIZE - m_stream.avail_out;
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
            PDFMM_PUSH_FRAME(e);
            throw e;
        }
    } while (m_stream.avail_out == 0);
}

void PdfFlateFilter::EndEncodeImpl()
{
    this->EncodeBlockInternal(nullptr, 0, Z_FINISH);
    deflateEnd(&m_stream);
}

void PdfFlateFilter::BeginDecodeImpl(const PdfDictionary* decodeParms)
{
    m_stream.zalloc = Z_NULL;
    m_stream.zfree = Z_NULL;
    m_stream.opaque = Z_NULL;

    if (decodeParms != nullptr)
        m_Predictor.reset(new PdfPredictorDecoder(*decodeParms));

    if (inflateInit(&m_stream) != Z_OK)
        PDFMM_RAISE_ERROR(PdfErrorCode::Flate);
}

void PdfFlateFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    int flateErr;
    unsigned writtenDataSize;

    m_stream.avail_in = static_cast<unsigned>(len);
    m_stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(buffer));

    do
    {
        m_stream.avail_out = BUFFER_SIZE;
        m_stream.next_out = m_buffer;

        switch ((flateErr = inflate(&m_stream, Z_NO_FLUSH)))
        {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
            {
                mm::LogMessage(PdfLogSeverity::Error, "Flate Decoding Error from ZLib: {}", flateErr);
                (void)inflateEnd(&m_stream);

                FailEncodeDecode();
                PDFMM_RAISE_ERROR(PdfErrorCode::Flate);
            }
            default:
                break;
        }

        writtenDataSize = BUFFER_SIZE - m_stream.avail_out;
        try
        {
            if (m_Predictor != nullptr)
                m_Predictor->Decode(reinterpret_cast<char*>(m_buffer), writtenDataSize, GetStream());
            else
                GetStream()->Write(reinterpret_cast<char*>(m_buffer), writtenDataSize);
        }
        catch (PdfError& e)
        {
            // clean up after any output stream errors
            FailEncodeDecode();
            PDFMM_PUSH_FRAME(e);
            throw e;
        }
    } while (m_stream.avail_out == 0);
}

void PdfFlateFilter::EndDecodeImpl()
{
    (void)inflateEnd(&m_stream);
    m_Predictor.reset();
}

#pragma endregion // PdfFlateFilter

#pragma region PdfRLEFilter

PdfRLEFilter::PdfRLEFilter()
    : m_CodeLen(0)
{
}

void PdfRLEFilter::BeginEncodeImpl()
{
    PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
}

void PdfRLEFilter::EncodeBlockImpl(const char*, size_t)
{
    PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
}

void PdfRLEFilter::EndEncodeImpl()
{
    PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
}

void PdfRLEFilter::BeginDecodeImpl(const PdfDictionary*)
{
    m_CodeLen = 0;
}

void PdfRLEFilter::DecodeBlockImpl(const char* buffer, size_t len)
{
    while (len-- != 0)
    {
        if (m_CodeLen == 0)
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

PdfLZWFilter::PdfLZWFilter() :
    m_mask(0),
    m_code_len(0),
    m_character(0),
    m_First(false)
{
}

void PdfLZWFilter::BeginEncodeImpl()
{
    PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
}

void PdfLZWFilter::EncodeBlockImpl(const char*, size_t)
{
    PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
}

void PdfLZWFilter::EndEncodeImpl()
{
    PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
}

void PdfLZWFilter::BeginDecodeImpl(const PdfDictionary* decodeParms)
{
    m_mask = 0;
    m_code_len = 9;
    m_character = 0;

    m_First = true;

    if (decodeParms != nullptr)
        m_Predictor.reset(new PdfPredictorDecoder(*decodeParms));

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
                        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                    }
                    data = m_table[old].value;
                    data.push_back(m_character);
                }
                else
                    data = m_table[code].value;

                // Write data to the output device
                if (m_Predictor != nullptr)
                    m_Predictor->Decode(reinterpret_cast<char*>(data.data()), data.size(), GetStream());
                else
                    GetStream()->Write(reinterpret_cast<char*>(data.data()), data.size());

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
    m_Predictor.reset();
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
