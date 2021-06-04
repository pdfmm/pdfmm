/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
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

#include "PdfTokenizer.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfName.h"
#include "PdfString.h"
#include "PdfReference.h"
#include "PdfVariant.h"
#include "PdfDefinesPrivate.h"

#include <limits>
#include <sstream>
#include <memory>

#include <stdlib.h>
#include <string.h>

#define PDF_BUFFER_SIZE 4096

using namespace std;
using namespace PoDoFo;

namespace PdfTokenizerNameSpace{

static const int g_MapAllocLen = 256;
static char g_DelMap[g_MapAllocLen] = { 0 };
static char g_WsMap[g_MapAllocLen] = { 0 };
static char g_EscMap[g_MapAllocLen] = { 0 };
static char g_hexMap[g_MapAllocLen] = { 0 };

// Generate the delimiter character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genDelMap()
{
    char* map = static_cast<char*>(g_DelMap);
    memset( map, 0, sizeof(char) * g_MapAllocLen );
    for (int i = 0; i < PoDoFo::s_nNumDelimiters; ++i)
    {
        map[static_cast<int>(PoDoFo::s_cDelimiters[i])] = 1;
    }

    return map;
}

// Generate the whitespace character map at runtime
// so that it can be derived from the more easily
// maintainable structures in PdfDefines.h
const char * genWsMap()
{
    char* map = static_cast<char*>(g_WsMap);
    memset( map, 0, sizeof(char) * g_MapAllocLen );
    for (int i = 0; i < PoDoFo::s_nNumWhiteSpaces; ++i)
    {
        map[static_cast<int>(PoDoFo::s_cWhiteSpaces[i])] = 1;
    }
    return map;
}

// Generate the escape character map at runtime
const char* genEscMap()
{
    char* map = static_cast<char*>(g_EscMap);
    memset( map, 0, sizeof(char) * g_MapAllocLen );

    map[static_cast<unsigned char>('n')] = '\n'; // Line feed (LF)
    map[static_cast<unsigned char>('r')] = '\r'; // Carriage return (CR)
    map[static_cast<unsigned char>('t')] = '\t'; // Horizontal tab (HT)
    map[static_cast<unsigned char>('b')] = '\b'; // Backspace (BS)
    map[static_cast<unsigned char>('f')] = '\f'; // Form feed (FF)
    map[static_cast<unsigned char>(')')] = ')';
    map[static_cast<unsigned char>('(')] = '(';
    map[static_cast<unsigned char>('\\')] = '\\';

    return map;
}

// Generate the hex character map at runtime
const char* genHexMap()
{
    char* map = static_cast<char*>(g_hexMap);
    memset( map, PdfTokenizer::HEX_NOT_FOUND, sizeof(char) * g_MapAllocLen );

    map[static_cast<unsigned char>('0')] = 0x0;
    map[static_cast<unsigned char>('1')] = 0x1;
    map[static_cast<unsigned char>('2')] = 0x2;
    map[static_cast<unsigned char>('3')] = 0x3;
    map[static_cast<unsigned char>('4')] = 0x4;
    map[static_cast<unsigned char>('5')] = 0x5;
    map[static_cast<unsigned char>('6')] = 0x6;
    map[static_cast<unsigned char>('7')] = 0x7;
    map[static_cast<unsigned char>('8')] = 0x8;
    map[static_cast<unsigned char>('9')] = 0x9;
    map[static_cast<unsigned char>('a')] = 0xA;
    map[static_cast<unsigned char>('b')] = 0xB;
    map[static_cast<unsigned char>('c')] = 0xC;
    map[static_cast<unsigned char>('d')] = 0xD;
    map[static_cast<unsigned char>('e')] = 0xE;
    map[static_cast<unsigned char>('f')] = 0xF;
    map[static_cast<unsigned char>('A')] = 0xA;
    map[static_cast<unsigned char>('B')] = 0xB;
    map[static_cast<unsigned char>('C')] = 0xC;
    map[static_cast<unsigned char>('D')] = 0xD;
    map[static_cast<unsigned char>('E')] = 0xE;
    map[static_cast<unsigned char>('F')] = 0xF;

    return map;
}

};

const char * const PdfTokenizer::s_delimiterMap  = PdfTokenizerNameSpace::genDelMap();
const char * const PdfTokenizer::s_whitespaceMap = PdfTokenizerNameSpace::genWsMap();
const char * const PdfTokenizer::s_escMap        = PdfTokenizerNameSpace::genEscMap();
const char * const PdfTokenizer::s_hexMap        = PdfTokenizerNameSpace::genHexMap();

const char PdfTokenizer::s_octMap[]        = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

PdfTokenizer::PdfTokenizer()
    : PdfTokenizer(PdfRefCountedBuffer(PDF_BUFFER_SIZE))
{
}

PdfTokenizer::PdfTokenizer(const PdfRefCountedBuffer& rBuffer)
    : m_buffer(rBuffer)
{
    PdfLocaleImbue(m_doubleParser);
}

PdfTokenizer::~PdfTokenizer() { }

bool PdfTokenizer::TryReadNextToken(PdfInputDevice& device, string_view& pszToken , EPdfTokenType* peType)
{
    int  c;
    int64_t  counter  = 0;

    // check first if there are queued tokens and return them first
    if( m_deqQueque.size() )
    {
        TTokenizerPair pair = m_deqQueque.front();
        m_deqQueque.pop_front();

        if( peType )
            *peType = pair.second;

        if ( !m_buffer.GetBuffer() || m_buffer.GetSize() == 0)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
        }

        // make sure buffer is \0 terminated
        strncpy(m_buffer.GetBuffer(), pair.first.c_str(), m_buffer.GetSize());
        m_buffer.GetBuffer()[m_buffer.GetSize() - 1] = 0;
        pszToken = m_buffer.GetBuffer();
        return true;
    }

    if( peType )
        *peType = EPdfTokenType::Literal;

    while ((c = device.Look()) != EOF
           && counter + 1 < static_cast<int64_t>(m_buffer.GetSize()))
    {
        // ignore leading whitespaces
        if( !counter && IsWhitespace( c ) )
        {
            // Consume the whitespace character
            (void)device.GetChar();
            continue;
        }
        // ignore comments
        else if (c == '%')
        {
            // Consume all characters before the next line break
            do
            {
                (void)device.GetChar();
            } while ((c = device.Look()) != EOF && c != '\n' && c != '\r');

            // If we've already read one or more chars of a token, return them, since
            // comments are treated as token-delimiting whitespace. Otherwise keep reading
            // at the start of the next line.
            if (counter)
                break;
        }
        // special handling for << and >> tokens
        else if( !counter && (c == '<' || c == '>' ) )
        {
            if( peType )
                *peType = EPdfTokenType::Delimiter;

            // Really consume character from stream
            (void)device.GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;

            char n = device.Look();
            // Is n another < or > , ie are we opening/closing a dictionary?
            // If so, consume that character too.
            if( n == c )
            {
                (void)device.GetChar();
                m_buffer.GetBuffer()[counter] = n;
                ++counter;
            }
            // `m_buffer' contains one of < , > , << or >> ; we're done .
            break;
        }
        else if( counter && (IsWhitespace( c ) || IsDelimiter( c )) )
        {
            // Next (unconsumed) character is a token-terminating char, so
            // we have a complete token and can return it.
            break;
        }
        else
        {
            // Consume the next character and add it to the token we're building.
            (void)device.GetChar();
            m_buffer.GetBuffer()[counter] = c;
            ++counter;

            if( IsDelimiter( c ) )
            {
                // All delimeters except << and >> (handled above) are
                // one-character tokens, so if we hit one we can just return it
                // immediately.
                if( peType )
                    *peType = EPdfTokenType::Delimiter;
                break;
            }
        }
    }

    m_buffer.GetBuffer()[counter] = '\0';

    if( c == EOF && !counter )
    {
        // No characters were read before EOF, so we're out of data.
        // Ensure the buffer points to nullptr in case someone fails to check the return value.
        pszToken = { };
        return false;
    }

    pszToken = m_buffer.GetBuffer();
    return true;
}

bool PdfTokenizer::IsNextToken(PdfInputDevice& device, const string_view& pszToken)
{
    if (pszToken.length() == 0)
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    string_view pszRead;
    bool gotToken = this->TryReadNextToken(device, pszRead);
    if (!gotToken)
    {
        PODOFO_RAISE_ERROR( EPdfError::UnexpectedEOF );
    }

    return pszToken == pszRead;
}

int64_t PdfTokenizer::ReadNextNumber(PdfInputDevice& device)
{
    EPdfTokenType eType;
    string_view pszRead;
    bool gotToken = this->TryReadNextToken(device, pszRead, &eType);

    if( !gotToken )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::UnexpectedEOF, "Expected number" );
    }

    char* end;
    long long num = strtoll(pszRead.data(), &end, 10);
    if (pszRead == end)
    {
        // Don't consume the token
        this->EnqueueToken(pszRead, eType);
        PODOFO_RAISE_ERROR_INFO( EPdfError::NoNumber, "Could not read number" );
    }

    return static_cast<int64_t>(num);
}

void PdfTokenizer::ReadNextVariant(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
   EPdfTokenType eTokenType;
   string_view pszRead;
   bool gotToken = this->TryReadNextToken(device, pszRead, &eTokenType);

   if (!gotToken)
   {
       PODOFO_RAISE_ERROR_INFO( EPdfError::UnexpectedEOF, "Expected variant." );
   }

   this->ReadNextVariant(device, pszRead, eTokenType, rVariant, pEncrypt);
}

void PdfTokenizer::ReadNextVariant(PdfInputDevice& device, const string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    if (!TryReadNextVariant(device, pszToken, eType, rVariant, pEncrypt))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Could not read a variant");
}

bool PdfTokenizer::TryReadNextVariant(PdfInputDevice& device, const string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    EPdfLiteralDataType eDataType = this->DetermineDataType(device, pszToken, eType, rVariant);
    return tryReadDataType(device, eDataType, rVariant, pEncrypt);
}

PdfTokenizer::EPdfLiteralDataType PdfTokenizer::DetermineDataType(PdfInputDevice& device,
    const string_view& pszToken, EPdfTokenType eTokenType, PdfVariant& rVariant )
{
    switch (eTokenType)
    {
    case EPdfTokenType::Literal:
    {
        // check for the two special datatypes
        // null and boolean.
        // check for numbers
        if (pszToken == "null")
        {
            rVariant = PdfVariant();
            return EPdfLiteralDataType::Null;
        }
        else if (pszToken == "true")
        {
            rVariant = PdfVariant( true );
            return EPdfLiteralDataType::Bool;
        }
        else if (pszToken == "false")
        {
            rVariant = PdfVariant( false );
            return EPdfLiteralDataType::Bool;
        }

        EPdfLiteralDataType eDataType = EPdfLiteralDataType::Number;
        const char* pszStart = pszToken.data();
        while (*pszStart)
        {
            if (*pszStart == '.')
            {
                eDataType = EPdfLiteralDataType::Real;
            }
            else if( !(isdigit( *pszStart ) || *pszStart == '-' || *pszStart == '+' ) )
            {
                eDataType = EPdfLiteralDataType::Unknown;
                break;
            }

            ++pszStart;
        }

        if (eDataType == EPdfLiteralDataType::Real)
        {
            double dVal;

            m_doubleParser.clear(); // clear error state
            m_doubleParser.str((string)pszToken);
            if (!(m_doubleParser >> dVal))
            {
                m_doubleParser.clear(); // clear error state
                PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, pszToken.data());
            }

            rVariant = PdfVariant(dVal);
            return EPdfLiteralDataType::Real;
        }
        else if (eDataType == EPdfLiteralDataType::Number)
        {
            rVariant = PdfVariant(static_cast<int64_t>(strtoll(pszToken.data(), nullptr, 10)));
            // read another two tokens to see if it is a reference
            // we cannot be sure that there is another token
            // on the input device, so if we hit EOF just return
            // EPdfDataType::Number .
            EPdfTokenType eSecondTokenType;
            string_view nextToken;
            bool gotToken = this->TryReadNextToken(device, nextToken, &eSecondTokenType);
            if (!gotToken)
            {
                // No next token, so it can't be a reference
                return EPdfLiteralDataType::Number;
            }
            if (eSecondTokenType != EPdfTokenType::Literal)
            {
                this->EnqueueToken(nextToken, eSecondTokenType);
                return EPdfLiteralDataType::Number;
            }

            char* end;
            long long l = strtoll(nextToken.data(), &end, 10);
            if (nextToken.data() == end)
            {
                this->EnqueueToken(nextToken, eSecondTokenType);
                return EPdfLiteralDataType::Number;
            }

            std::string tmp(nextToken);
            EPdfTokenType eThirdTokenType;
            gotToken = this->TryReadNextToken(device, nextToken, &eThirdTokenType);
            if (!gotToken)
            {
                // No third token, so it can't be a reference
                return EPdfLiteralDataType::Number;
            }
            if (eThirdTokenType == EPdfTokenType::Literal &&
                nextToken.length() == 1 && nextToken[0] == 'R')
            {
                rVariant = PdfReference(static_cast<unsigned int>(rVariant.GetNumber()),
                    static_cast<uint16_t>(l));
                return EPdfLiteralDataType::Reference;
            }
            else
            {
                this->EnqueueToken(tmp, eSecondTokenType);
                this->EnqueueToken(nextToken, eThirdTokenType);
                return EPdfLiteralDataType::Number;
            }
        }
        else
            return EPdfLiteralDataType::Unknown;
    }
    case EPdfTokenType::Delimiter:
    {
        if (pszToken == "<<")
            return EPdfLiteralDataType::Dictionary;
        else if (pszToken[0] == '[')
            return EPdfLiteralDataType::Array;
        else if (pszToken[0] == '(')
            return EPdfLiteralDataType::String;
        else if (pszToken[0] == '<')
            return EPdfLiteralDataType::HexString;
        else if (pszToken[0] == '/')
            return EPdfLiteralDataType::Name;
        else
            PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
        break;
    }
    default:
        PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
    }
}

bool PdfTokenizer::tryReadDataType(PdfInputDevice& device, EPdfLiteralDataType eDataType, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    switch( eDataType )
    {
        case EPdfLiteralDataType::Dictionary:
            this->ReadDictionary(device, rVariant, pEncrypt);
            return true;
        case EPdfLiteralDataType::Array:
            this->ReadArray(device, rVariant, pEncrypt);
            return true;
        case EPdfLiteralDataType::String:
            this->ReadString(device, rVariant, pEncrypt);
            return true;
        case EPdfLiteralDataType::HexString:
            this->ReadHexString(device, rVariant, pEncrypt);
            return true;
        case EPdfLiteralDataType::Name:
            this->ReadName(device, rVariant);
            return true;
        // The following datatypes are not handled by read datatype
        // but are already parsed by DetermineDatatype
        case EPdfLiteralDataType::Null:
        case EPdfLiteralDataType::Bool:
        case EPdfLiteralDataType::Number:
        case EPdfLiteralDataType::Real:
        case EPdfLiteralDataType::Reference:
            return true;
        default:
            return false;
    }
}

void PdfTokenizer::ReadDictionary(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    PdfVariant val;
    PdfName key;
    EPdfTokenType eType;
    string_view pszToken;
    std::unique_ptr<std::vector<char> > contentsHexBuffer;

    rVariant = PdfDictionary();
    PdfDictionary &dict = rVariant.GetDictionary();

    for( ;; )
    {
        bool gotToken = this->TryReadNextToken(device, pszToken, &eType);
        if (!gotToken)
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected dictionary key name or >> delim.");
        }
        if (eType == EPdfTokenType::Delimiter && pszToken == ">>")
            break;

        this->ReadNextVariant(device, pszToken, eType, val, pEncrypt );
        // Convert the read variant to a name; throws InvalidDataType if not a name.
        key = val.GetName();

        // Try to get the next variant
        gotToken = this->TryReadNextToken(device, pszToken, &eType );
        if ( !gotToken )
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::UnexpectedEOF, "Expected variant." );
        }

        EPdfLiteralDataType eDataType = this->DetermineDataType(device, pszToken, eType, val);
        if ( key == "Contents" && eDataType == EPdfLiteralDataType::HexString )
        {
            // 'Contents' key in signature dictionaries is an unencrypted Hex string:
            // save the string buffer for later check if it needed decryption
            contentsHexBuffer = std::unique_ptr<std::vector<char> >( new std::vector<char>() );
            readHexString(device, *contentsHexBuffer);
            continue;
        }

        if (!tryReadDataType(device, eDataType, val, pEncrypt))
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Could not read variant");

        // Add the key without triggering SetDirty
        dict.addKey(key, val, true);
    }

    if ( contentsHexBuffer.get() != nullptr )
    {
        PdfObject *type = dict.GetKey( "Type" );
        // "Contents" is unencrypted in /Type/Sig and /Type/DocTimeStamp dictionaries 
        // https://issues.apache.org/jira/browse/PDFBOX-3173
        bool contentsUnencrypted = type != nullptr && type->GetDataType() == EPdfDataType::Name &&
            (type->GetName() == PdfName( "Sig" ) || type->GetName() == PdfName( "DocTimeStamp" ));

        PdfEncrypt *encrypt = nullptr;
        if ( !contentsUnencrypted )
            encrypt = pEncrypt;

        PdfString string;
        string.SetHexData( contentsHexBuffer->size() ? &(*contentsHexBuffer)[0] : "", contentsHexBuffer->size(), encrypt );

        val = string;
        dict.AddKey( "Contents", val );
    }
}

void PdfTokenizer::ReadArray(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    string_view pszToken;
    EPdfTokenType eType;
    PdfVariant var;
    rVariant = PdfArray();
    PdfArray &array = rVariant.GetArray();

    for( ;; )
    {
        bool gotToken = this->TryReadNextToken(device, pszToken, &eType);
        if (!gotToken)
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected array item or ] delim.");
        }
        if( eType == EPdfTokenType::Delimiter && pszToken[0] == ']' )
            break;

        this->ReadNextVariant(device, pszToken, eType, var, pEncrypt);
        array.push_back( var );
    }
}

void PdfTokenizer::ReadString(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    int               c;

    bool              bEscape       = false;
    bool              bOctEscape    = false;
    int               nOctCount     = 0;
    char              cOctValue     = 0;
    int               nBalanceCount = 0; // Balanced parathesis do not have to be escaped in strings

    m_vecBuffer.clear();

    while (device.TryGetChar(c))
    {
        // end of stream reached
        if( !bEscape )
        {
            // Handle raw characters
            if( !nBalanceCount && c == ')' )
                break;

            if( c == '(' )
                ++nBalanceCount;
            else if( c == ')' )
                --nBalanceCount;

            bEscape = (c == '\\');
            if( !bEscape )
                m_vecBuffer.push_back( static_cast<char>(c) );
        }
        else
        {
            // Handle escape sequences
            if( bOctEscape || s_octMap[c & 0xff] )
                // The last character we have read was a '\\',
                // so we check now for a digit to find stuff like \005
                bOctEscape = true;

            if( bOctEscape )
            {
                // Handle octal escape sequences
                ++nOctCount;

                if( !s_octMap[c & 0xff] )
                {
                    // No octal character anymore,
                    // so the octal sequence must be ended
                    // and the character has to be treated as normal character!
                    m_vecBuffer.push_back ( cOctValue );
                    bEscape    = false;
                    bOctEscape = false;
                    nOctCount  = 0;
                    cOctValue  = 0;
                    continue;
                }

                cOctValue <<= 3;
                cOctValue  |= ((c-'0') & 0x07);

                if( nOctCount > 2 )
                {
                    m_vecBuffer.push_back ( cOctValue );
                    bEscape    = false;
                    bOctEscape = false;
                    nOctCount  = 0;
                    cOctValue  = 0;
                }
            }
            else
            {
                // Handle plain escape sequences
                const char & code = s_escMap[c & 0xff];
                if( code )
                    m_vecBuffer.push_back( code );

                bEscape = false;
            }
        }
    }

    // In case the string ends with a octal escape sequence
    if( bOctEscape )
        m_vecBuffer.push_back ( cOctValue );

    if( m_vecBuffer.size() )
    {
        if( pEncrypt )
        {
            size_t outLen = m_vecBuffer.size() - pEncrypt->CalculateStreamOffset();
            char * outBuffer = new char[outLen + 16 - (outLen % 16)];
            pEncrypt->Decrypt( reinterpret_cast<unsigned char*>(&(m_vecBuffer[0])),
                              static_cast<unsigned int>(m_vecBuffer.size()),
                              reinterpret_cast<unsigned char*>(outBuffer), outLen);

            rVariant = PdfString( outBuffer, outLen );

            delete[] outBuffer;
        }
        else
        {
            rVariant = PdfString( &(m_vecBuffer[0]), m_vecBuffer.size() );
        }
    }
    else
    {
        rVariant = PdfString("");
    }
}

void PdfTokenizer::ReadHexString(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt )
{
    readHexString(device, m_vecBuffer );

    PdfString string;
    string.SetHexData( m_vecBuffer.size() ? &(m_vecBuffer[0]) : "", m_vecBuffer.size(), pEncrypt );

    rVariant = string;
}

void PdfTokenizer::readHexString(PdfInputDevice& device, std::vector<char>& rVecBuffer)
{
    rVecBuffer.clear();
    int c;

    while (device.TryGetChar(c))
    {
        // end of stream reached
        if( c == '>' )
            break;

        // only a hex digits
        if( isdigit( c ) ||
            ( c >= 'A' && c <= 'F') ||
            ( c >= 'a' && c <= 'f'))
            rVecBuffer.push_back( c );
    }

    // pad to an even length if necessary
    if(rVecBuffer.size() % 2 )
        rVecBuffer.push_back( '0' );
}

void PdfTokenizer::ReadName(PdfInputDevice& device, PdfVariant& rVariant)
{
    // Do special checking for empty names
    // as tryReadNextToken will ignore white spaces
    // and we have to take care for stuff like:
    // 10 0 obj / endobj
    // which stupid but legal PDF
    int c = device.Look();
    if( IsWhitespace( c ) ) // Delimeters are handled correctly by tryReadNextToken
    {
        // We are an empty PdfName
        rVariant = PdfName();
        return;
    }

    EPdfTokenType eType;
    string_view pszToken;
    bool gotToken = this->TryReadNextToken(device, pszToken, &eType);
    if( !gotToken || eType != EPdfTokenType::Literal )
    {
        // We got an empty name which is legal according to the PDF specification
        // Some weird PDFs even use them.
        rVariant = PdfName();

        // Enqueue the token again
        if( gotToken )
            EnqueueToken( pszToken, eType );
    }
    else
        rVariant = PdfName::FromEscaped( pszToken );
}

void PdfTokenizer::EnqueueToken(const string_view& pszToken, EPdfTokenType eType )
{
    m_deqQueque.push_back(TTokenizerPair(string(pszToken), eType));
}

bool PdfTokenizer::IsWhitespace(const unsigned char ch)
{
    return PdfTokenizer::s_whitespaceMap[static_cast<size_t>(ch)] != 0;
}

bool PdfTokenizer::IsDelimiter(const unsigned char ch)
{
    return PdfTokenizer::s_delimiterMap[ch] != 0;
}

bool PdfTokenizer::IsRegular(const unsigned char ch)
{
    return !IsWhitespace(ch) && !IsDelimiter(static_cast<size_t>(ch));
}

bool PdfTokenizer::IsPrintable(const unsigned char ch)
{
    return ch > 32U && ch < 125U;
}

int PdfTokenizer::GetHexValue(const unsigned char ch)
{
    return PdfTokenizer::s_hexMap[static_cast<size_t>(ch)];
}
