/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfString.h"

#include "PdfEncrypt.h"
#include "PdfPredefinedEncoding.h"
#include "PdfEncodingFactory.h"
#include "PdfFilter.h"
#include "PdfTokenizer.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

enum class StringEncoding
{
    utf8,
    utf16be,
    utf16le,
    PdfDocEncoding
};

static char getEscapedCharacter(char ch);
static StringEncoding getEncoding(const string_view& view);

PdfString::PdfString()
    : m_data(new StringData{ StringState::PdfDocEncoding, { } }), m_isHex(false)
{
}

PdfString::PdfString(const char* str)
    : m_isHex(false)
{
    initFromUtf8String({ str, std::strlen(str) });
}

PdfString::PdfString(const string_view& view)
    : m_isHex(false)
{
    initFromUtf8String(view);
}

PdfString::PdfString(const PdfString& rhs)
{
    this->operator=(rhs);
}

PdfString::PdfString(charbuff chars, bool isHex)
    : m_data(new StringData{ StringState::RawBuffer, std::move(chars) }), m_isHex(isHex)
{
}

PdfString PdfString::FromRaw(const bufferview& view, bool isHex)
{
    return PdfString((charbuff)view, isHex);
}

PdfString PdfString::FromHexData(const string_view& hexView, PdfEncrypt* encrypt)
{
    size_t len = hexView.size();
    charbuff buffer;
    buffer.reserve(len % 2 ? (len + 1) >> 1 : len >> 1);

    char val;
    char decodedChar = 0;
    bool low = true;
    for (size_t i = 0; i < len; i++)
    {
        char ch = hexView[i];
        if (PdfTokenizer::IsWhitespace(ch))
            continue;

        val = PdfTokenizer::GetHexValue(ch);
        if (low)
        {
            decodedChar = val & 0x0F;
            low = false;
        }
        else
        {
            decodedChar = (decodedChar << 4) | val;
            low = true;
            buffer.push_back(decodedChar);
        }
    }

    if (!low)
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        buffer.push_back(decodedChar);
    }

    if (encrypt == nullptr)
    {
        buffer.shrink_to_fit();
        return PdfString(std::move(buffer), true);
    }
    else
    {
        charbuff decrypted;
        encrypt->Decrypt(buffer, decrypted);
        return PdfString(std::move(decrypted), true);
    }
}

void PdfString::Write(PdfOutputDevice& device, PdfWriteMode writeMode, const PdfEncrypt* encrypt) const
{
    (void)writeMode;

    // Strings in PDF documents may contain \0 especially if they are encrypted
    // this case has to be handled!

    // We are not encrypting the empty strings (was access violation)!
    charbuff tempBuffer;
    string_view dataview;
    if (m_data->State == StringState::Unicode)
    {
        // Prepend utf-16 BOM
        tempBuffer.push_back(static_cast<char>(0xFE));
        tempBuffer.push_back(static_cast<char>(0xFF));
        tempBuffer.insert(tempBuffer.end(), m_data->Chars.data(), m_data->Chars.data() + m_data->Chars.size());
        dataview = string_view(tempBuffer.data(), tempBuffer.size());
    }
    else
    {
        dataview = string_view(m_data->Chars);
    }

    if (encrypt != nullptr && dataview.size() > 0)
    {
        charbuff encrypted;
        encrypt->Encrypt(dataview, encrypted);
        encrypted.swap(tempBuffer);
        dataview = string_view(tempBuffer.data(), tempBuffer.size());
    }

    device.Put(m_isHex ? '<' : '(');
    if (dataview.size() > 0)
    {
        char ch;
        const char* buffer = dataview.data();
        size_t len = dataview.size();

        if (m_isHex)
        {
            char data[2];
            while (len-- != 0)
            {
                ch = *buffer;
                utls::WriteCharHexTo(data, ch);
                device.Write(string_view(data, 2));
                buffer++;
            }
        }
        else
        {
            char ch;
            while (len-- != 0)
            {
                ch = *buffer;
                char escaped = getEscapedCharacter(ch);
                if (escaped == '\0')
                {
                    device.Put(ch);
                }
                else
                {
                    device.Put('\\');
                    device.Put(escaped);
                }

                buffer++;
            }
        }
    }

    device.Put(m_isHex ? '>' : ')');
}

bool PdfString::IsUnicode() const
{
    return m_data->State == StringState::Unicode;
}

const string& PdfString::GetString() const
{
    evaluateString();
    return m_data->Chars;
}

bool PdfString::IsEmpty() const
{
    return m_data->Chars.empty();
}

const PdfString& PdfString::operator=(const PdfString& rhs)
{
    this->m_data = rhs.m_data;
    this->m_isHex = rhs.m_isHex;
    return *this;
}

bool PdfString::operator==(const PdfString& rhs) const
{
    if (this == &rhs)
        return true;

    if (!canPerformComparison(*this, rhs))
        return false;

    if (this->m_data == rhs.m_data)
        return true;

    return this->m_data->Chars == rhs.m_data->Chars;
}

bool PdfString::operator==(const char* str) const
{
    return operator==(string_view(str, std::strlen(str)));
}

bool PdfString::operator==(const string& str) const
{
    return operator==((string_view)str);
}

bool PdfString::operator==(const string_view& view) const
{
    if (!isValidText())
        return false;

    return m_data->Chars == view;
}

bool PdfString::operator!=(const PdfString& rhs) const
{
    if (this == &rhs)
        return false;

    if (!canPerformComparison(*this, rhs))
        return true;

    if (this->m_data == rhs.m_data)
        return false;

    return this->m_data->Chars != rhs.m_data->Chars;
}

bool PdfString::operator!=(const char* str) const
{
    return operator!=(string_view(str, std::strlen(str)));
}

bool PdfString::operator!=(const string& str) const
{
    return operator!=((string_view)str);
}

bool PdfString::operator!=(const string_view& view) const
{
    if (!isValidText())
        return true;

    return m_data->Chars != view;
}

void PdfString::initFromUtf8String(const string_view& view)
{
    if (view.data() == nullptr)
        throw runtime_error("String is null");

    if (view.length() == 0)
    {
        m_data.reset(new StringData{ StringState::PdfDocEncoding, { } });
        return;
    }

    bool isPdfDocEncodingEqual;
    if (PdfDocEncoding::CheckValidUTF8ToPdfDocEcondingChars(view, isPdfDocEncodingEqual))
        m_data.reset(new StringData{ StringState::PdfDocEncoding, charbuff(view) });
    else
        m_data.reset(new StringData{ StringState::Unicode, charbuff(view) });
}

void PdfString::evaluateString() const
{
    switch (m_data->State)
    {
        case StringState::PdfDocEncoding:
        case StringState::Unicode:
            return;
        case StringState::RawBuffer:
        {
            auto encoding = getEncoding(m_data->Chars);
            switch (encoding)
            {
                case StringEncoding::utf16be:
                {
                    // Remove BOM and decode utf-16 string
                    string utf8;
                    auto view = string_view(m_data->Chars).substr(2);
                    utls::ReadUtf16BEString(view, utf8);
                    utf8.swap(m_data->Chars);
                    m_data->State = StringState::Unicode;
                    break;
                }
                case StringEncoding::utf16le:
                {
                    // Remove BOM and decode utf-16 string
                    string utf8;
                    auto view = string_view(m_data->Chars).substr(2);
                    utls::ReadUtf16LEString(view, utf8);
                    utf8.swap(m_data->Chars);
                    m_data->State = StringState::Unicode;
                    break;
                }
                case StringEncoding::utf8:
                {
                    // Remove BOM
                    m_data->Chars.substr(3).swap(m_data->Chars);
                    m_data->State = StringState::Unicode;
                    break;
                }
                case StringEncoding::PdfDocEncoding:
                {
                    bool isUTF8Equal;
                    auto utf8 = PdfDocEncoding::ConvertPdfDocEncodingToUTF8(m_data->Chars, isUTF8Equal);
                    utf8.swap(m_data->Chars);
                    m_data->State = StringState::PdfDocEncoding;
                    break;
                }
                default:
                    throw runtime_error("Unsupported");
            }

            return;
        }
        default:
            throw runtime_error("Unsupported");
    }
}

// Returns true only if same state or it's valid text string
bool PdfString::canPerformComparison(const PdfString& lhs, const PdfString& rhs)
{
    if (lhs.m_data->State == rhs.m_data->State)
        return true;

    if (lhs.isValidText() || rhs.isValidText())
        return true;

    return false;
}

const string& PdfString::GetRawData() const
{
    if (m_data->State != StringState::RawBuffer)
        throw runtime_error("The string buffer has been evaluated");

    return m_data->Chars;
}

bool PdfString::isValidText() const
{
    return m_data->State == StringState::PdfDocEncoding || m_data->State == StringState::Unicode;
}

StringEncoding getEncoding(const string_view& view)
{
    const char utf16beMarker[2] = { static_cast<char>(0xFE), static_cast<char>(0xFF) };
    if (view.size() >= sizeof(utf16beMarker) && memcmp(view.data(), utf16beMarker, sizeof(utf16beMarker)) == 0)
        return StringEncoding::utf16be;

    // NOTE: Little endian should not be officially supported
    const char utf16leMarker[2] = { static_cast<char>(0xFF), static_cast<char>(0xFE) };
    if (view.size() >= sizeof(utf16leMarker) && memcmp(view.data(), utf16leMarker, sizeof(utf16leMarker)) == 0)
        return StringEncoding::utf16le;

    const char utf8Marker[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
    if (view.size() >= sizeof(utf8Marker) && memcmp(view.data(), utf8Marker, sizeof(utf8Marker)) == 0)
        return StringEncoding::utf8;

    return StringEncoding::PdfDocEncoding;
}

char getEscapedCharacter(char ch)
{
    switch (ch)
    {
        case '\n':           // Line feed (LF)
            return 'n';
        case '\r':           // Carriage return (CR)
            return 'r';
        case '\t':           // Horizontal tab (HT)
            return 't';
        case '\b':           // Backspace (BS)
            return 'b';
        case '\f':           // Form feed (FF)
            return 'f';
        case '(':
            return '(';
        case ')':
            return ')';
        case '\\':
            return '\\';
        default:
            return '\0';
    }
}
