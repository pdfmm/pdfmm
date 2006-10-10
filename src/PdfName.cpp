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
 ***************************************************************************/

#include "PdfName.h"

#include "PdfOutputDevice.h"
#include "PdfParserBase.h"

using namespace std;

namespace {

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
inline void hexchr(const char ch, T & it)
{
    *(it++) = "0123456789ABCDEF"[ch / 16];
    *(it++) = "0123456789ABCDEF"[ch % 16];
}

/** Escape the input string according to the PDF name
 *  escaping rules and return the result.
 *
 *  \param it Iterator referring to the start of the input string
 *            ( eg a `const char *' or a `std::string::iterator' )
 *  \param length Length of input string
 *  \returns Escaped string
 */
template<typename T>
static std::string EscapeName(T it, size_t length)
{
    // Scan the input string once to find out how much memory we need
    // to reserve for the encoded result string. We could do this in one
    // pass using a std::ostringstream instead, but it's a LOT slower.
    T it2(it);
    unsigned int outchars = 0;
    for (size_t i = 0; i < length; ++i)
    {
        // Null chars are illegal in names, even escaped
        if (*it2 == '\0')
            throw std::exception();
        else 
            // Leave room for either just the char, or a #xx escape of it.
            outchars += (::PoDoFo::PdfParserBase::IsRegular(*it2) && ::PoDoFo::PdfParserBase::IsPrintable(*it2) && (*it2 != '#')) ? 1 : 3;
        ++it2;
    }
    // Reserve it. We can't use reserve() because the GNU STL doesn't seem to
    // do it correctly; the memory never seems to get allocated.
    std::string buf;
    buf.resize(outchars);
    // and generate the encoded string
    std::string::iterator bufIt(buf.begin());
    for (size_t i = 0; i < length; ++i)
    {
        if (::PoDoFo::PdfParserBase::IsRegular(*it) && ::PoDoFo::PdfParserBase::IsPrintable(*it) && (*it != '#') )
            *(bufIt++) = *it;
        else
        {
            *(bufIt++) = '#';
            hexchr(*it, bufIt);
        }
        ++it;
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
template<typename T>
static std::string UnescapeName(T it, size_t length)
{
    // We know the decoded string can be AT MOST
    // the same length as the encoded one, so:
    std::string buf;
    buf.resize(length);
    unsigned int incount = 0, outcount = 0;
    while (incount++ < length)
    {
        if (*it == '#')
        {
            char hi = *(++it); ++incount;
            char low = *(++it); ++incount;
            hi  -= ( hi  < 'A' ? '0' : 'A'-10 );
            low -= ( low < 'A' ? '0' : 'A'-10 );
            buf[outcount++] = (hi << 4) | (low & 0x0F);
        }
        else
            buf[outcount++] = *it;
        ++it;
    }
    // Chop buffer off at number of decoded bytes
    buf.resize(outcount);
    return buf;
}

}; // End anonymous namespace

namespace PoDoFo {

const PdfName PdfName::KeyContents  = PdfName( "Contents" );
const PdfName PdfName::KeyFlags     = PdfName( "Flags" );
const PdfName PdfName::KeyLength    = PdfName( "Length" );
const PdfName PdfName::KeyNull      = PdfName();
const PdfName PdfName::KeyRect      = PdfName( "Rect" );
const PdfName PdfName::KeySize      = PdfName( "Size" );
const PdfName PdfName::KeySubtype   = PdfName( "Subtype" );
const PdfName PdfName::KeyType      = PdfName( "Type" );
const PdfName PdfName::KeyFilter    = PdfName( "Filter" );

PdfName::PdfName() : m_Data( "" ) {}

PdfName::PdfName( const std::string& sName )
{
    m_Data = sName;
}

PdfName::PdfName( const char* pszName )
{
    if( pszName )
    {
        m_Data.assign( pszName );
    }
}

PdfName::PdfName( const char* pszName, long lLen )
{
    if( pszName )
    {
        m_Data.assign( pszName, lLen );
    }
}

PdfName PdfName::FromEscaped( const string & sName )
{
    return PdfName(UnescapeName(sName.begin(), sName.length()));
}

PdfName PdfName::FromEscaped( const char * pszName, int ilen )
{
    return PdfName(UnescapeName(pszName, ilen));
}

PdfName::PdfName( const PdfName & rhs )
{
    this->operator=( rhs );
}

PdfName::~PdfName()
{
}

void PdfName::Write( PdfOutputDevice* pDevice ) const
{
    pDevice->Print( "/" );
    string escaped( EscapeName(m_Data.begin(), m_Data.length()) );
    pDevice->Write( escaped.c_str(), escaped.length() );
}

string PdfName::GetEscapedName() const
{
    return EscapeName(m_Data.begin(), m_Data.length());
}

const PdfName& PdfName::operator=( const PdfName & rhs )
{
    m_Data = rhs.m_Data;
    return *this;
}

bool PdfName::operator==( const PdfName & rhs ) const
{
    return ( m_Data == rhs.m_Data );
}

bool PdfName::operator==( const std::string & rhs ) const
{
    return ( m_Data == rhs );
}

bool PdfName::operator==( const char* rhs ) const
{
    /*
      If the string is empty and you pass NULL - that's equivalent
      If the string is NOT empty and you pass NULL - that's not equal
      Otherwise, compare them
    */
    if( m_Data.empty() && !rhs )
        return true;
    else if( !m_Data.empty() && !rhs )
        return false;
    else
        return ( m_Data == std::string( rhs ) );
}

bool PdfName::operator<( const PdfName & rhs ) const
{
    return m_Data < rhs.m_Data;
}

};

