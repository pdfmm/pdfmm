/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfName.h"

#include <cassert>

#include "PdfOutputDevice.h"
#include "PdfTokenizer.h"
#include "PdfDefinesPrivate.h"
#include "PdfEncoding.h"

using namespace std;
using namespace PoDoFo;

template<typename T>
void hexchr(const unsigned char ch, T & it);

static string EscapeName(const string_view & view);
static string UnescapeName(const string_view& view);

const PdfName PdfName::KeyNull = PdfName();
const PdfName PdfName::KeyContents  = PdfName( "Contents" );
const PdfName PdfName::KeyFlags     = PdfName( "Flags" );
const PdfName PdfName::KeyLength    = PdfName( "Length" );
const PdfName PdfName::KeyRect      = PdfName( "Rect" );
const PdfName PdfName::KeySize      = PdfName( "Size" );
const PdfName PdfName::KeySubtype   = PdfName( "Subtype" );
const PdfName PdfName::KeyType      = PdfName( "Type" );
const PdfName PdfName::KeyFilter    = PdfName( "Filter" );

PdfName::PdfName()
    : m_rawdata(std::make_shared<string>()), m_isUtf8Expanded(true)
{
}

PdfName::PdfName(const char* str)
{
    initFromUtf8String({ str, std::strlen(str) });
}

PdfName::PdfName(const char* str, size_t len)
{
    initFromUtf8String({ str, len });
}

PdfName::PdfName(const string_view& view)
{
    initFromUtf8String(view);
}

PdfName::PdfName(const std::string& str)
{
    initFromUtf8String(str);
}

PdfName::PdfName(const PdfName& rhs)
    : m_rawdata(rhs.m_rawdata), m_isUtf8Expanded(rhs.m_isUtf8Expanded), m_utf8String(rhs.m_utf8String)
{
}

PdfName::PdfName(const shared_ptr<string>& rawdata)
    : m_rawdata(rawdata), m_isUtf8Expanded(false)
{
}

void PdfName::initFromUtf8String(const string_view& view)
{
    if (view.length() == 0)
    {
        m_rawdata = std::make_shared<string>();
        m_isUtf8Expanded = true;
        return;
    }

    bool isPdfDocEncodingEqual;
    if (!PdfDocEncoding::CheckValidUTF8ToPdfDocEcondingChars(view, isPdfDocEncodingEqual))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidName, "Characters in string must be PdfDocEncoding character set");

    if (isPdfDocEncodingEqual)
    {
        m_rawdata = std::make_shared<string>(view);
    }
    else
    {
        m_rawdata = std::make_shared<string>(PdfDocEncoding::ConvertUTF8ToPdfDocEncoding(view));
        m_utf8String = std::make_shared<string>(view);
    }

    m_isUtf8Expanded = true;
}

PdfName PdfName::FromEscaped(const std::string_view& view)
{
    return FromRaw(UnescapeName(view));
}

PdfName PdfName::FromEscaped(const char * pszName, size_t ilen)
{
    if( !pszName )
        return PdfName();

    if( !ilen )
        ilen = strlen( pszName );

    return FromRaw(UnescapeName({ pszName, ilen }));
}

PdfName PdfName::FromRaw(const string_view & rawcontent)
{
    return PdfName(std::make_shared<string>(rawcontent));
}

void PdfName::Write( PdfOutputDevice* pDevice, EPdfWriteMode, const PdfEncrypt* ) const
{
    // Allow empty names, which are legal according to the PDF specification
    pDevice->Print( "/" );
    if (m_rawdata->length() != 0)
    {
        string escaped = EscapeName(*m_rawdata);
        pDevice->Write( escaped.c_str(), escaped.length() );
    }
}

string PdfName::GetEscapedName() const
{
    if (m_rawdata->length() == 0)
        return string();

    return EscapeName(*m_rawdata);
}

void PdfName::expandUtf8String() const
{
    if (!m_isUtf8Expanded)
    {
        auto& name = const_cast<PdfName&>(*this);
        bool isUtf8Equal;
        auto utf8str = std::make_shared<string>();
        PdfDocEncoding::ConvertPdfDocEncodingToUTF8(*m_rawdata, *utf8str, isUtf8Equal);
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
    // pass using a std::ostringstream instead, but it's a LOT slower.
    size_t outchars = 0;
    for (size_t i = 0; i < view.length(); i++)
    {
        char ch = view[i];

        // Null chars are illegal in names, even escaped
        if (ch == '\0')
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidName, "Null byte in PDF name is illegal");
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
    for (size_t i = 0; i < view.length(); ++i)
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
            hi  -= ( hi  < 'A' ? '0' : 'A'-10 );
            low -= ( low < 'A' ? '0' : 'A'-10 );
            unsigned char codepoint = (hi << 4) | (low & 0x0F);
            buf.push_back((char)codepoint);
        }
        else
            buf.push_back(*curr);

        ++curr;
    }

    return buf;
}

const string & PdfName::GetString() const
{
    expandUtf8String();
    if (m_utf8String == nullptr)
        return *m_rawdata;
    else
        return *m_utf8String;
}

size_t PdfName::GetLength() const
{
    expandUtf8String();
    if (m_utf8String == nullptr)
        return m_rawdata->length();
    else
        return m_utf8String->length();
}

const PdfName& PdfName::operator=(const PdfName& rhs)
{
    m_rawdata = rhs.m_rawdata;
    m_isUtf8Expanded = rhs.m_isUtf8Expanded;
    m_utf8String = rhs.m_utf8String;
    return *this;
}

bool PdfName::operator==(const PdfName& rhs) const
{
    return *this->m_rawdata == *rhs.m_rawdata;
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
    return *this->m_rawdata != *rhs.m_rawdata;
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
    return *this->m_rawdata < *rhs.m_rawdata;
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
