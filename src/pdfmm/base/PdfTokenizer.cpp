/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
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
using namespace mm;

static char getEscapedCharacter(char ch);
static void readHexString(PdfInputDevice& device, buffer_t& rVecBuffer);
static bool isOctalChar(char ch);

PdfTokenizer::PdfTokenizer(bool readReferences)
    : PdfTokenizer(PdfRefCountedBuffer(BufferSize), readReferences)
{
}

PdfTokenizer::PdfTokenizer(const PdfRefCountedBuffer& buffer, bool readReferences)
    : m_buffer(buffer), m_readReferences(readReferences)
{
    PdfLocaleImbue(m_doubleParser);
}

bool PdfTokenizer::TryReadNextToken(PdfInputDevice& device, string_view& token)
{
    PdfTokenType tokenType;
    return TryReadNextToken(device, token, tokenType);
}

bool PdfTokenizer::TryReadNextToken(PdfInputDevice& device, string_view& token, PdfTokenType& tokenType)
{
    int c;
    int64_t counter = 0;

    // check first if there are queued tokens and return them first
    if (m_tokenQueque.size())
    {
        TokenizerPair pair = m_tokenQueque.front();
        m_tokenQueque.pop_front();

        tokenType = pair.second;

        if (!m_buffer.GetBuffer() || m_buffer.GetSize() == 0)
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
        }

        // make sure buffer is \0 terminated
        strncpy(m_buffer.GetBuffer(), pair.first.c_str(), m_buffer.GetSize());
        m_buffer.GetBuffer()[m_buffer.GetSize() - 1] = 0;
        token = m_buffer.GetBuffer();
        return true;
    }

    tokenType = PdfTokenType::Literal;

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
                    tokenType = PdfTokenType::DoubleAngleBracketsLeft;
                else
                    tokenType = PdfTokenType::DoubleAngleBracketsRight;
                counter++;

            }
            else
            {
                if (c == '<')
                    tokenType = PdfTokenType::AngleBracketLeft;
                else
                    tokenType = PdfTokenType::AngleBracketRight;
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

            PdfTokenType tokenDelimiterType;
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
        token = { };
        return false;
    }

    token = m_buffer.GetBuffer();
    return true;
}

bool PdfTokenizer::IsNextToken(PdfInputDevice& device, const string_view& token)
{
    if (token.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    string_view readToken;
    bool gotToken = this->TryReadNextToken(device, readToken);
    if (!gotToken)
        PDFMM_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);

    return token == readToken;
}

int64_t PdfTokenizer::ReadNextNumber(PdfInputDevice& device)
{
    PdfTokenType tokenType;
    string_view token;
    bool gotToken = this->TryReadNextToken(device, token, tokenType);
    if (!gotToken)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected number");

    char* end;
    long long num = strtoll(token.data(), &end, 10);
    if (token == end)
    {
        // Don't consume the token
        this->EnqueueToken(token, tokenType);
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoNumber, "Could not read number");
    }

    return static_cast<int64_t>(num);
}

void PdfTokenizer::ReadNextVariant(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt)
{
    PdfTokenType tokenType;
    string_view token;
    bool gotToken = this->TryReadNextToken(device, token, tokenType);
    if (!gotToken)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant.");

    this->ReadNextVariant(device, token, tokenType, variant, encrypt);
}

void PdfTokenizer::ReadNextVariant(PdfInputDevice& device, const string_view& token, PdfTokenType tokenType, PdfVariant& variant, PdfEncrypt* encrypt)
{
    if (!TryReadNextVariant(device, token, tokenType, variant, encrypt))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Could not read a variant");
}

bool PdfTokenizer::TryReadNextVariant(PdfInputDevice& device, const string_view& token, PdfTokenType tokenType, PdfVariant& variant, PdfEncrypt* encrypt)
{
    PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, variant);
    return tryReadDataType(device, dataType, variant, encrypt);
}

PdfTokenizer::PdfLiteralDataType PdfTokenizer::DetermineDataType(PdfInputDevice& device,
    const string_view& token, PdfTokenType tokenType, PdfVariant& variant)
{
    switch (tokenType)
    {
        case PdfTokenType::Literal:
        {
            // check for the two special datatypes
            // null and boolean.
            // check for numbers
            if (token == "null")
            {
                variant = PdfVariant();
                return PdfLiteralDataType::Null;
            }
            else if (token == "true")
            {
                variant = PdfVariant(true);
                return PdfLiteralDataType::Bool;
            }
            else if (token == "false")
            {
                variant = PdfVariant(false);
                return PdfLiteralDataType::Bool;
            }

            PdfLiteralDataType dataType = PdfLiteralDataType::Number;
            const char* start = token.data();
            while (*start)
            {
                if (*start == '.')
                {
                    dataType = PdfLiteralDataType::Real;
                }
                else if (!(isdigit(*start) || *start == '-' || *start == '+'))
                {
                    dataType = PdfLiteralDataType::Unknown;
                    break;
                }

                start++;
            }

            if (dataType == PdfLiteralDataType::Real)
            {
                double val;

                m_doubleParser.clear(); // clear error state
                m_doubleParser.str((string)token);
                if (!(m_doubleParser >> val))
                {
                    m_doubleParser.clear(); // clear error state
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, token.data());
                }

                variant = PdfVariant(val);
                return PdfLiteralDataType::Real;
            }
            else if (dataType == PdfLiteralDataType::Number)
            {
                variant = PdfVariant(static_cast<int64_t>(strtoll(token.data(), nullptr, 10)));
                if (!m_readReferences)
                    return PdfLiteralDataType::Number;

                // read another two tokens to see if it is a reference
                // we cannot be sure that there is another token
                // on the input device, so if we hit EOF just return
                // EPdfDataType::Number .
                PdfTokenType secondTokenType;
                string_view nextToken;
                bool gotToken = this->TryReadNextToken(device, nextToken, secondTokenType);
                if (!gotToken)
                {
                    // No next token, so it can't be a reference
                    return PdfLiteralDataType::Number;
                }
                if (secondTokenType != PdfTokenType::Literal)
                {
                    this->EnqueueToken(nextToken, secondTokenType);
                    return PdfLiteralDataType::Number;
                }

                char* end;
                long long l = strtoll(nextToken.data(), &end, 10);
                if (nextToken.data() == end)
                {
                    this->EnqueueToken(nextToken, secondTokenType);
                    return PdfLiteralDataType::Number;
                }

                std::string tmp(nextToken);
                PdfTokenType thirdTokenType;
                gotToken = this->TryReadNextToken(device, nextToken, thirdTokenType);
                if (!gotToken)
                {
                    // No third token, so it can't be a reference
                    return PdfLiteralDataType::Number;
                }
                if (thirdTokenType == PdfTokenType::Literal &&
                    nextToken.length() == 1 && nextToken[0] == 'R')
                {
                    variant = PdfReference(static_cast<unsigned>(variant.GetNumber()),
                        static_cast<uint16_t>(l));
                    return PdfLiteralDataType::Reference;
                }
                else
                {
                    this->EnqueueToken(tmp, secondTokenType);
                    this->EnqueueToken(nextToken, thirdTokenType);
                    return PdfLiteralDataType::Number;
                }
            }
            else
                return PdfLiteralDataType::Unknown;
        }
        case PdfTokenType::DoubleAngleBracketsLeft:
            return PdfLiteralDataType::Dictionary;
        case PdfTokenType::SquareBracketLeft:
            return PdfLiteralDataType::Array;
        case PdfTokenType::ParenthesisLeft:
            return PdfLiteralDataType::String;
        case PdfTokenType::AngleBracketLeft:
            return PdfLiteralDataType::HexString;
        case PdfTokenType::Slash:
            return PdfLiteralDataType::Name;
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported token at this context");
    }
}

bool PdfTokenizer::tryReadDataType(PdfInputDevice& device, PdfLiteralDataType dataType, PdfVariant& variant, PdfEncrypt* encrypt)
{
    switch (dataType)
    {
        case PdfLiteralDataType::Dictionary:
            this->ReadDictionary(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::Array:
            this->ReadArray(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::String:
            this->ReadString(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::HexString:
            this->ReadHexString(device, variant, encrypt);
            return true;
        case PdfLiteralDataType::Name:
            this->ReadName(device, variant);
            return true;
        // The following datatypes are not handled by read datatype
        // but are already parsed by DetermineDatatype
        case PdfLiteralDataType::Null:
        case PdfLiteralDataType::Bool:
        case PdfLiteralDataType::Number:
        case PdfLiteralDataType::Real:
        case PdfLiteralDataType::Reference:
            return true;
        default:
            return false;
    }
}

void PdfTokenizer::ReadDictionary(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt)
{
    PdfVariant val;
    PdfName key;
    PdfTokenType tokenType;
    string_view token;
    unique_ptr<buffer_t> contentsHexBuffer;

    variant = PdfDictionary();
    PdfDictionary& dict = variant.GetDictionary();

    while (true)
    {
        bool gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected dictionary key name or >> delim.");

        if (tokenType == PdfTokenType::DoubleAngleBracketsRight)
            break;

        this->ReadNextVariant(device, token, tokenType, val, encrypt);
        // Convert the read variant to a name; throws InvalidDataType if not a name.
        key = val.GetName();

        // Try to get the next variant
        gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant.");

        PdfLiteralDataType dataType = DetermineDataType(device, token, tokenType, val);
        if (key == "Contents" && dataType == PdfLiteralDataType::HexString)
        {
            // 'Contents' key in signature dictionaries is an unencrypted Hex string:
            // save the string buffer for later check if it needed decryption
            contentsHexBuffer = std::unique_ptr<buffer_t>(new buffer_t());
            readHexString(device, *contentsHexBuffer);
            continue;
        }

        if (!tryReadDataType(device, dataType, val, encrypt))
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Could not read variant");

        // Add the key without triggering SetDirty
        dict.addKey(key, val, true);
    }

    if (contentsHexBuffer.get() != nullptr)
    {
        PdfObject* type = dict.GetKey("Type");
        // "Contents" is unencrypted in /Type/Sig and /Type/DocTimeStamp dictionaries 
        // https://issues.apache.org/jira/browse/PDFBOX-3173
        bool contentsUnencrypted = type != nullptr && type->GetDataType() == PdfDataType::Name &&
            (type->GetName() == PdfName("Sig") || type->GetName() == PdfName("DocTimeStamp"));

        PdfEncrypt* encrypt = nullptr;
        if (!contentsUnencrypted)
            encrypt = encrypt;

        val = PdfString::FromHexData({ contentsHexBuffer->size() ? contentsHexBuffer->data() : "", contentsHexBuffer->size() }, encrypt);
        dict.AddKey("Contents", val);
    }
}

void PdfTokenizer::ReadArray(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt)
{
    string_view token;
    PdfTokenType tokenType;
    PdfVariant var;
    variant = PdfArray();
    PdfArray& array = variant.GetArray();

    for (;; )
    {
        bool gotToken = this->TryReadNextToken(device, token, tokenType);
        if (!gotToken)
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected array item or ] delim.");
        }
        if (tokenType == PdfTokenType::SquareBracketRight)
            break;

        this->ReadNextVariant(device, token, tokenType, var, encrypt);
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

void PdfTokenizer::ReadHexString(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt)
{
    readHexString(device, m_charBuffer);
    variant = PdfString::FromHexData({ m_charBuffer.size() ? m_charBuffer.data() : "", m_charBuffer.size() }, encrypt);
}

void PdfTokenizer::ReadName(PdfInputDevice& device, PdfVariant& variant)
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
        variant = PdfName();
        return;
    }

    PdfTokenType tokenType;
    string_view token;
    bool gotToken = this->TryReadNextToken(device, token, tokenType);
    if (!gotToken || tokenType != PdfTokenType::Literal)
    {
        // We got an empty name which is legal according to the PDF specification
        // Some weird PDFs even use them.
        variant = PdfName();

        // Enqueue the token again
        if (gotToken)
            EnqueueToken(token, tokenType);
    }
    else
        variant = PdfName::FromEscaped(token);
}

void PdfTokenizer::EnqueueToken(const string_view& token, PdfTokenType tokenType)
{
    m_tokenQueque.push_back(TokenizerPair(string(token), tokenType));
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

bool PdfTokenizer::IsTokenDelimiter(unsigned char ch, PdfTokenType& tokenType)
{
    switch (ch)
    {
        case '(':
            tokenType = PdfTokenType::ParenthesisLeft;
            return true;
        case ')':
            tokenType = PdfTokenType::ParenthesisRight;
            return true;
        case '[':
            tokenType = PdfTokenType::SquareBracketLeft;
            return true;
        case ']':
            tokenType = PdfTokenType::SquareBracketRight;
            return true;
        case '{':
            tokenType = PdfTokenType::BraceLeft;
            return true;
        case '}':
            tokenType = PdfTokenType::BraceRight;
            return true;
        case '/':
            tokenType = PdfTokenType::Slash;
            return true;
        default:
            tokenType = PdfTokenType::Unknown;
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
