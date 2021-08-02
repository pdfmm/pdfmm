/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfTokenizer.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfName.h"
#include "PdfString.h"
#include "PdfReference.h"
#include "PdfVariant.h"

#include <sstream>

using namespace std;
using namespace PoDoFo;

static char getEscapedCharacter(char ch);
static void readHexString(PdfInputDevice& device, buffer_t& rVecBuffer);
static bool isOctalChar(char ch);

PdfTokenizer::PdfTokenizer(bool readReferences)
    : PdfTokenizer(PdfRefCountedBuffer(BufferSize), readReferences)
{
}

PdfTokenizer::PdfTokenizer(const PdfRefCountedBuffer& rBuffer, bool readReferences)
    : m_buffer(rBuffer), m_readReferences(readReferences)
{
    PdfLocaleImbue(m_doubleParser);
}

bool PdfTokenizer::TryReadNextToken(PdfInputDevice& device, string_view& token)
{
    EPdfTokenType tokenType;
    return TryReadNextToken(device, token, tokenType);
}

bool PdfTokenizer::TryReadNextToken(PdfInputDevice& device, string_view& pszToken, EPdfTokenType& tokenType)
{
    int c;
    int64_t counter = 0;

    // check first if there are queued tokens and return them first
    if (m_deqQueque.size())
    {
        TTokenizerPair pair = m_deqQueque.front();
        m_deqQueque.pop_front();

        tokenType = pair.second;

        if (!m_buffer.GetBuffer() || m_buffer.GetSize() == 0)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
        }

        // make sure buffer is \0 terminated
        strncpy(m_buffer.GetBuffer(), pair.first.c_str(), m_buffer.GetSize());
        m_buffer.GetBuffer()[m_buffer.GetSize() - 1] = 0;
        pszToken = m_buffer.GetBuffer();
        return true;
    }

    tokenType = EPdfTokenType::Literal;

    while ((c = device.Look()) != EOF
        && counter + 1 < static_cast<int64_t>(m_buffer.GetSize()))
    {
        // ignore leading whitespaces
        if (counter == 0 && IsWhitespace(c))
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
            if (counter != 0)
                break;
        }
        // special handling for << and >> tokens
        else if (counter == 0 && (c == '<' || c == '>'))
        {
            // Really consume character from stream
            (void)device.GetChar();
            m_buffer.GetBuffer()[counter] = c;
            counter++;

            char n = device.Look();
            // Is n another < or > , ie are we opening/closing a dictionary?
            // If so, consume that character too.
            if (n == c)
            {
                (void)device.GetChar();
                m_buffer.GetBuffer()[counter] = n;
                if (c == '<')
                    tokenType = EPdfTokenType::DoubleAngleBracketsLeft;
                else
                    tokenType = EPdfTokenType::DoubleAngleBracketsRight;
                counter++;

            }
            else
            {
                if (c == '<')
                    tokenType = EPdfTokenType::AngleBracketLeft;
                else
                    tokenType = EPdfTokenType::AngleBracketRight;
            }
            break;
        }
        else if (counter != 0 && (IsWhitespace(c) || IsDelimiter(c)))
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
            counter++;

            EPdfTokenType tokenDelimiterType;
            if (IsTokenDelimiter(c, tokenDelimiterType))
            {
                // All delimeters except << and >> (handled above) are
                // one-character tokens, so if we hit one we can just return it
                // immediately.
                tokenType = tokenDelimiterType;
                break;
            }
        }
    }

    m_buffer.GetBuffer()[counter] = '\0';

    if (c == EOF && counter == 0)
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
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    string_view pszRead;
    bool gotToken = this->TryReadNextToken(device, pszRead);
    if (!gotToken)
    {
        PODOFO_RAISE_ERROR(EPdfError::UnexpectedEOF);
    }

    return pszToken == pszRead;
}

int64_t PdfTokenizer::ReadNextNumber(PdfInputDevice& device)
{
    EPdfTokenType eType;
    string_view pszRead;
    bool gotToken = this->TryReadNextToken(device, pszRead, eType);

    if (!gotToken)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected number");
    }

    char* end;
    long long num = strtoll(pszRead.data(), &end, 10);
    if (pszRead == end)
    {
        // Don't consume the token
        this->EnqueueToken(pszRead, eType);
        PODOFO_RAISE_ERROR_INFO(EPdfError::NoNumber, "Could not read number");
    }

    return static_cast<int64_t>(num);
}

void PdfTokenizer::ReadNextVariant(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    EPdfTokenType eTokenType;
    string_view pszRead;
    bool gotToken = this->TryReadNextToken(device, pszRead, eTokenType);

    if (!gotToken)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected variant.");
    }

    this->ReadNextVariant(device, pszRead, eTokenType, rVariant, pEncrypt);
}

void PdfTokenizer::ReadNextVariant(PdfInputDevice& device, const string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    if (!TryReadNextVariant(device, pszToken, eType, rVariant, pEncrypt))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Could not read a variant");
}

bool PdfTokenizer::TryReadNextVariant(PdfInputDevice& device, const string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    EPdfLiteralDataType eDataType = DetermineDataType(device, pszToken, eType, rVariant);
    return tryReadDataType(device, eDataType, rVariant, pEncrypt);
}

PdfTokenizer::EPdfLiteralDataType PdfTokenizer::DetermineDataType(PdfInputDevice& device,
    const string_view& pszToken, EPdfTokenType eTokenType, PdfVariant& rVariant)
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
                rVariant = PdfVariant(true);
                return EPdfLiteralDataType::Bool;
            }
            else if (pszToken == "false")
            {
                rVariant = PdfVariant(false);
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
                else if (!(isdigit(*pszStart) || *pszStart == '-' || *pszStart == '+'))
                {
                    eDataType = EPdfLiteralDataType::Unknown;
                    break;
                }

                pszStart++;
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
                if (!m_readReferences)
                    return EPdfLiteralDataType::Number;

                // read another two tokens to see if it is a reference
                // we cannot be sure that there is another token
                // on the input device, so if we hit EOF just return
                // EPdfDataType::Number .
                EPdfTokenType eSecondTokenType;
                string_view nextToken;
                bool gotToken = this->TryReadNextToken(device, nextToken, eSecondTokenType);
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
                gotToken = this->TryReadNextToken(device, nextToken, eThirdTokenType);
                if (!gotToken)
                {
                    // No third token, so it can't be a reference
                    return EPdfLiteralDataType::Number;
                }
                if (eThirdTokenType == EPdfTokenType::Literal &&
                    nextToken.length() == 1 && nextToken[0] == 'R')
                {
                    rVariant = PdfReference(static_cast<unsigned>(rVariant.GetNumber()),
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
        case EPdfTokenType::DoubleAngleBracketsLeft:
            return EPdfLiteralDataType::Dictionary;
        case EPdfTokenType::SquareBracketLeft:
            return EPdfLiteralDataType::Array;
        case EPdfTokenType::ParenthesisLeft:
            return EPdfLiteralDataType::String;
        case EPdfTokenType::AngleBracketLeft:
            return EPdfLiteralDataType::HexString;
        case EPdfTokenType::Slash:
            return EPdfLiteralDataType::Name;
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEnumValue, "Unsupported token at this context");
    }
}

bool PdfTokenizer::tryReadDataType(PdfInputDevice& device, EPdfLiteralDataType eDataType, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    switch (eDataType)
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
    unique_ptr<buffer_t> contentsHexBuffer;

    rVariant = PdfDictionary();
    PdfDictionary& dict = rVariant.GetDictionary();

    while (true)
    {
        bool gotToken = this->TryReadNextToken(device, pszToken, eType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected dictionary key name or >> delim.");

        if (eType == EPdfTokenType::DoubleAngleBracketsRight)
            break;

        this->ReadNextVariant(device, pszToken, eType, val, pEncrypt);
        // Convert the read variant to a name; throws InvalidDataType if not a name.
        key = val.GetName();

        // Try to get the next variant
        gotToken = this->TryReadNextToken(device, pszToken, eType);
        if (!gotToken)
            PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected variant.");

        EPdfLiteralDataType eDataType = DetermineDataType(device, pszToken, eType, val);
        if (key == "Contents" && eDataType == EPdfLiteralDataType::HexString)
        {
            // 'Contents' key in signature dictionaries is an unencrypted Hex string:
            // save the string buffer for later check if it needed decryption
            contentsHexBuffer = std::unique_ptr<buffer_t>(new buffer_t());
            readHexString(device, *contentsHexBuffer);
            continue;
        }

        if (!tryReadDataType(device, eDataType, val, pEncrypt))
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Could not read variant");

        // Add the key without triggering SetDirty
        dict.addKey(key, val, true);
    }

    if (contentsHexBuffer.get() != nullptr)
    {
        PdfObject* type = dict.GetKey("Type");
        // "Contents" is unencrypted in /Type/Sig and /Type/DocTimeStamp dictionaries 
        // https://issues.apache.org/jira/browse/PDFBOX-3173
        bool contentsUnencrypted = type != nullptr && type->GetDataType() == EPdfDataType::Name &&
            (type->GetName() == PdfName("Sig") || type->GetName() == PdfName("DocTimeStamp"));

        PdfEncrypt* encrypt = nullptr;
        if (!contentsUnencrypted)
            encrypt = pEncrypt;

        val = PdfString::FromHexData({ contentsHexBuffer->size() ? contentsHexBuffer->data() : "", contentsHexBuffer->size() }, encrypt);
        dict.AddKey("Contents", val);
    }
}

void PdfTokenizer::ReadArray(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    string_view pszToken;
    EPdfTokenType eType;
    PdfVariant var;
    rVariant = PdfArray();
    PdfArray& array = rVariant.GetArray();

    for (;; )
    {
        bool gotToken = this->TryReadNextToken(device, pszToken, eType);
        if (!gotToken)
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected array item or ] delim.");
        }
        if (eType == EPdfTokenType::SquareBracketRight)
            break;

        this->ReadNextVariant(device, pszToken, eType, var, pEncrypt);
        array.push_back(var);
    }
}

void PdfTokenizer::ReadString(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt)
{
    char ch;
    bool escape = false;
    bool octEscape = false;
    int octCharCount = 0;
    char octValue = 0;
    int nBalanceCount = 0; // Balanced parathesis do not have to be escaped in strings

    m_charBuffer.clear();
    while (device.TryGetChar(ch))
    {
        if (escape)
        {
            // Handle escape sequences
            if (octEscape)
            {
                // Handle octal escape sequences
                octCharCount++;

                if (!isOctalChar(ch))
                {
                    if (ch == ')')
                    {
                        // Handle end of string while reading octal code
                        // NOTE: The octal value is added outside of the loop
                        break;
                    }

                    // No octal character anymore,
                    // so the octal sequence must be ended
                    // and the character has to be treated as normal character!
                    m_charBuffer.push_back(octValue);

                    if (ch != '\\')
                    {
                        m_charBuffer.push_back(ch);
                        escape = false;
                    }

                    octEscape = false;
                    octCharCount = 0;
                    octValue = 0;
                    continue;
                }

                octValue <<= 3;
                octValue |= ((ch - '0') & 0x07);

                if (octCharCount == 3)
                {
                    m_charBuffer.push_back(octValue);
                    escape = false;
                    octEscape = false;
                    octCharCount = 0;
                    octValue = 0;
                }
            }
            else if (isOctalChar(ch))
            {
                // The last character we have read was a '\\',
                // so we check now for a digit to find stuff like \005
                octValue = (ch - '0') & 0x07;
                octEscape = true;
            }
            else
            {
                // Ignore end of line characters when reading escaped sequences
                if (ch != '\n' && ch != '\r')
                {
                    // Handle plain escape sequences
                    char escapedCh = getEscapedCharacter(ch);
                    if (escapedCh != '\0')
                        m_charBuffer.push_back(escapedCh);
                }

                escape = false;
            }
        }
        else
        {
            // Handle raw characters
            if (!nBalanceCount && ch == ')')
                break;

            if (ch == '(')
                nBalanceCount++;
            else if (ch == ')')
                nBalanceCount--;

            escape = ch == '\\';
            if (!escape)
                m_charBuffer.push_back(static_cast<char>(ch));
        }
    }

    // In case the string ends with a octal escape sequence
    if (octEscape)
        m_charBuffer.push_back(octValue);

    if (m_charBuffer.size())
    {
        if (encrypt)
        {
            auto decrypted = std::make_shared<string>();
            encrypt->Decrypt({ m_charBuffer.data(), m_charBuffer.size() }, *decrypted);
            variant = PdfString(decrypted, false);
        }
        else
        {
            variant = PdfString::FromRaw({ m_charBuffer.data(), m_charBuffer.size() }, false);
        }
    }
    else
    {
        // NOTE: The string is empty but ensure it will be
        // initialized as a raw buffer first
        variant = PdfString::FromRaw({ }, false);
    }
}

void PdfTokenizer::ReadHexString(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt)
{
    readHexString(device, m_charBuffer);
    rVariant = PdfString::FromHexData({ m_charBuffer.size() ? m_charBuffer.data() : "", m_charBuffer.size() }, pEncrypt);
}

void PdfTokenizer::ReadName(PdfInputDevice& device, PdfVariant& rVariant)
{
    // Do special checking for empty names
    // as tryReadNextToken will ignore white spaces
    // and we have to take care for stuff like:
    // 10 0 obj / endobj
    // which stupid but legal PDF
    int c = device.Look();
    if (IsWhitespace(c)) // Delimeters are handled correctly by tryReadNextToken
    {
        // We are an empty PdfName
        rVariant = PdfName();
        return;
    }

    EPdfTokenType eType;
    string_view pszToken;
    bool gotToken = this->TryReadNextToken(device, pszToken, eType);
    if (!gotToken || eType != EPdfTokenType::Literal)
    {
        // We got an empty name which is legal according to the PDF specification
        // Some weird PDFs even use them.
        rVariant = PdfName();

        // Enqueue the token again
        if (gotToken)
            EnqueueToken(pszToken, eType);
    }
    else
        rVariant = PdfName::FromEscaped(pszToken);
}

void PdfTokenizer::EnqueueToken(const string_view& pszToken, EPdfTokenType eType)
{
    m_deqQueque.push_back(TTokenizerPair(string(pszToken), eType));
}

bool PdfTokenizer::IsWhitespace(unsigned char ch)
{
    switch (ch)
    {
        case '\0': // NULL
            return true;
        case '\t': // TAB
            return true;
        case '\n': // Line Feed
            return true;
        case '\f': // Form Feed
            return true;
        case '\r': // Carriage Return
            return true;
        case ' ': // White space
            return true;
        default:
            return false;
    }
}

bool PdfTokenizer::IsDelimiter(unsigned char ch)
{
    switch (ch)
    {
        case '(':
            return true;
        case ')':
            return true;
        case '<':
            return true;
        case '>':
            return true;
        case '[':
            return true;
        case ']':
            return true;
        case '{':
            return true;
        case '}':
            return true;
        case '/':
            return true;
        case '%':
            return true;
        default:
            return false;
    }
}

bool PdfTokenizer::IsTokenDelimiter(unsigned char ch, EPdfTokenType& tokenType)
{
    switch (ch)
    {
        case '(':
            tokenType = EPdfTokenType::ParenthesisLeft;
            return true;
        case ')':
            tokenType = EPdfTokenType::ParenthesisRight;
            return true;
        case '[':
            tokenType = EPdfTokenType::SquareBracketLeft;
            return true;
        case ']':
            tokenType = EPdfTokenType::SquareBracketRight;
            return true;
        case '{':
            tokenType = EPdfTokenType::BraceLeft;
            return true;
        case '}':
            tokenType = EPdfTokenType::BraceRight;
            return true;
        case '/':
            tokenType = EPdfTokenType::Slash;
            return true;
        default:
            tokenType = EPdfTokenType::Unknown;
            return false;
    }
}

bool PdfTokenizer::IsRegular(unsigned char ch)
{
    return !IsWhitespace(ch) && !IsDelimiter(ch);
}

bool PdfTokenizer::IsPrintable(unsigned char ch)
{
    return ch > 32U && ch < 125U;
}

int PdfTokenizer::GetHexValue(unsigned char ch)
{
    switch (ch)
    {
        case '0':
            return 0x0;
        case '1':
            return 0x1;
        case '2':
            return 0x2;
        case '3':
            return 0x3;
        case '4':
            return 0x4;
        case '5':
            return 0x5;
        case '6':
            return 0x6;
        case '7':
            return 0x7;
        case '8':
            return 0x8;
        case '9':
            return 0x9;
        case 'a':
        case 'A':
            return 0xA;
        case 'b':
        case 'B':
            return 0xB;
        case 'c':
        case 'C':
            return 0xC;
        case 'd':
        case 'D':
            return 0xD;
        case 'e':
        case 'E':
            return 0xE;
        case 'f':
        case 'F':
            return 0xF;
        default:
            return HEX_NOT_FOUND;
    }
}

char getEscapedCharacter(char ch)
{
    switch (ch)
    {
        case 'n':           // Line feed (LF)
            return '\n';
        case 'r':           // Carriage return (CR)
            return '\r';
        case 't':           // Horizontal tab (HT)
            return '\t';
        case 'b':           // Backspace (BS)
            return '\b';
        case 'f':           // Form feed (FF)
            return '\f';
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

void readHexString(PdfInputDevice& device, buffer_t& rVecBuffer)
{
    rVecBuffer.clear();
    char ch;
    while (device.TryGetChar(ch))
    {
        // end of stream reached
        if (ch == '>')
            break;

        // only a hex digits
        if (isdigit(ch) ||
            (ch >= 'A' && ch <= 'F') ||
            (ch >= 'a' && ch <= 'f'))
            rVecBuffer.push_back(ch);
    }

    // pad to an even length if necessary
    if (rVecBuffer.size() % 2)
        rVecBuffer.push_back('0');
}

bool isOctalChar(char ch)
{
    switch (ch)
    {
        case '0':
            return true;
        case '1':
            return true;
        case '2':
            return true;
        case '3':
            return true;
        case '4':
            return true;
        case '5':
            return true;
        case '6':
            return true;
        case '7':
            return true;
        default:
            return false;
    }
}
