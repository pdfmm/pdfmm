/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfName.h"

#include "PdfOutputDevice.h"
#include "PdfTokenizer.h"
#include "PdfPredefinedEncoding.h"

using namespace std;
using namespace mm;

template<typename T>
void hexchr(const unsigned char ch, T& it);

static string EscapeName(const string_view& view);
static string UnescapeName(const string_view& view);

const PdfName PdfName::KeyNull = PdfName();
const PdfName PdfName::KeyContents = PdfName("Contents");
const PdfName PdfName::KeyFlags = PdfName("Flags");
const PdfName PdfName::KeyLength = PdfName("Length");
const PdfName PdfName::KeyRect = PdfName("Rect");
const PdfName PdfName::KeySize = PdfName("Size");
const PdfName PdfName::KeySubtype = PdfName("Subtype");
const PdfName PdfName::KeyType = PdfName("Type");
const PdfName PdfName::KeyFilter = PdfName("Filter");

PdfName::PdfName()
    : m_data(std::make_shared<string>()), m_isUtf8Expanded(true)
{
}

PdfName::PdfName(const char* str)
{
    initFromUtf8String({ str, std::strlen(str) });
}

PdfName::PdfName(const string& str)
{
    initFromUtf8String(str);
}

PdfName::PdfName(const string_view& view)
{
    initFromUtf8String(view);
}

PdfName::PdfName(const PdfName& rhs)
    : m_data(rhs.m_data), m_isUtf8Expanded(rhs.m_isUtf8Expanded), m_utf8String(rhs.m_utf8String)
{
}

PdfName::PdfName(const shared_ptr<string>& rawdata)
    : m_data(rawdata), m_isUtf8Expanded(false)
{
}

void PdfName::initFromUtf8String(const string_view& view)
{
    if (view.data() == nullptr)
        throw runtime_error("Name is null");

    if (view.length() == 0)
    {
        m_data = std::make_shared<string>();
        m_isUtf8Expanded = true;
        return;
    }

    bool isPdfDocEncodingEqual;
    if (!PdfDocEncoding::CheckValidUTF8ToPdfDocEcondingChars(view, isPdfDocEncodingEqual))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Characters in string must be PdfDocEncoding character set");

    if (isPdfDocEncodingEqual)
    {
        m_data = std::make_shared<string>(view);
    }
    else
    {
        m_data = std::make_shared<string>(PdfDocEncoding::ConvertUTF8ToPdfDocEncoding(view));
        m_utf8String = std::make_shared<string>(view);
    }

    m_isUtf8Expanded = true;
}

PdfName PdfName::FromEscaped(const std::string_view& view)
{
    return FromRaw(UnescapeName(view));
}

PdfName PdfName::FromRaw(const string_view& rawcontent)
{
    return PdfName(std::make_shared<string>(rawcontent));
}

void PdfName::Write(PdfOutputDevice& device, PdfWriteMode, const PdfEncrypt*) const
{
    // Allow empty names, which are legal according to the PDF specification
    device.Print("/");
    if (m_data->length() != 0)
    {
        string escaped = EscapeName(*m_data);
        device.Write(escaped.c_str(), escaped.length());
    }
}

string PdfName::GetEscapedName() const
{
    if (m_data->length() == 0)
        return string();

    return EscapeName(*m_data);
}

void PdfName::expandUtf8String() const
{
    if (!m_isUtf8Expanded)
    {
        auto& name = const_cast<PdfName&>(*this);
        bool isUtf8Equal;
        auto utf8str = std::make_shared<string>();
        PdfDocEncoding::ConvertPdfDocEncodingToUTF8(*m_data, *utf8str, isUtf8Equal);
        if (!isUtf8Equal)
            name.m_utf8String = utf8str;

        name.m_isUtf8Expanded = true;
    }
}

/** Escape the input string according to the PDF name
 *  escaping rules and return the result.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Escaped string
 */
string EscapeName(const string_view& view)
{
    // Scan the input string once to find out how much memory we need
    // to reserve for the encoded result string. We could do this in one
    // pass using a ostringstream instead, but it's a LOT slower.
    size_t outchars = 0;
    for (size_t i = 0; i < view.length(); i++)
    {
        char ch = view[i];

        // Null chars are illegal in names, even escaped
        if (ch == '\0')
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidName, "Null byte in PDF name is illegal");
        }
        else
        {
            // Leave room for either just the char, or a #xx escape of it.
            outchars += (PdfTokenizer::IsRegular(ch) &&
                PdfTokenizer::IsPrintable(ch) && (ch != '#')) ? 1 : 3;
        }
    }
    // Reserve it. We can't use reserve() because the GNU STL doesn't seem to
    // do it correctly; the memory never seems to get allocated.
    string buf;
    buf.resize(outchars);
    // and generate the encoded string
    string::iterator bufIt(buf.begin());
    for (size_t i = 0; i < view.length(); i++)
    {
        char ch = view[i];

        if (PdfTokenizer::IsRegular(ch)
            && PdfTokenizer::IsPrintable(ch)
            && ch != '#')
        {
            *(bufIt++) = ch;
        }
        else
        {
            *(bufIt++) = '#';
            hexchr(static_cast<unsigned char>(ch), bufIt);
        }
    }
    return buf;
}

/** Interpret the passed string as an escaped PDF name
 *  and return the unescaped form.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Unescaped string
 */
string UnescapeName(const string_view& view)
{
    // We know the decoded string can be AT MOST
    // the same length as the encoded one, so:
    string buf;
    buf.reserve(view.length());
    size_t incount = 0;
    const char* curr = view.data();
    while (incount++ < view.length())
    {
        if (*curr == '#' && incount + 1 < view.length())
        {
            unsigned char hi = static_cast<unsigned char>(*(++curr));
            incount++;
            unsigned char low = static_cast<unsigned char>(*(++curr));
            incount++;
            hi -= (hi < 'A' ? '0' : 'A' - 10);
            low -= (low < 'A' ? '0' : 'A' - 10);
            unsigned char codepoint = (hi << 4) | (low & 0x0F);
            buf.push_back((char)codepoint);
        }
        else
            buf.push_back(*curr);

        curr++;
    }

    return buf;
}

const string& PdfName::GetString() const
{
    expandUtf8String();
    if (m_utf8String == nullptr)
        return *m_data;
    else
        return *m_utf8String;
}

size_t PdfName::GetLength() const
{
    expandUtf8String();
    if (m_utf8String == nullptr)
        return m_data->length();
    else
        return m_utf8String->length();
}

const string& PdfName::GetRawData() const
{
    return *m_data;
}

const PdfName& PdfName::operator=(const PdfName& rhs)
{
    m_data = rhs.m_data;
    m_isUtf8Expanded = rhs.m_isUtf8Expanded;
    m_utf8String = rhs.m_utf8String;
    return *this;
}

bool PdfName::operator==(const PdfName& rhs) const
{
    return *this->m_data == *rhs.m_data;
}

bool PdfName::operator==(const char* str) const
{
    return operator==(string_view(str, std::strlen(str)));
}

bool PdfName::operator==(const string& str) const
{
    return operator==((string_view)str);
}

bool PdfName::operator==(const string_view& view) const
{
    auto& str = GetString();
    return str == view;
}

bool PdfName::operator!=(const PdfName& rhs) const
{
    return *this->m_data != *rhs.m_data;
}

bool PdfName::operator!=(const char* str) const
{
    return operator!=(string_view(str, std::strlen(str)));
}

bool PdfName::operator!=(const string& str) const
{
    return operator!=((string_view)str);
}

bool PdfName::operator!=(const string_view& view) const
{
    auto& str = GetString();
    return str != view;
}

bool PdfName::operator<(const PdfName& rhs) const
{
    return *this->m_data < *rhs.m_data;
}

/**
 * This function writes a hex encoded representation of the character
 * `ch' to `buf', advancing the iterator by two steps.
 *
 * \warning no buffer length checking is performed, so MAKE SURE
 *          you have enough room for the two characters that
 *          will be written to the buffer.
 *
 * \param ch The character to write a hex representation of
 * \param buf An iterator (eg a char* or std::string::iterator) to write the
 *            characters to.  Must support the postfix ++, operator=(char) and
 *            dereference operators.
 */
template<typename T>
void hexchr(const unsigned char ch, T& it)
{
    *(it++) = "0123456789ABCDEF"[ch / 16];
    *(it++) = "0123456789ABCDEF"[ch % 16];
}
