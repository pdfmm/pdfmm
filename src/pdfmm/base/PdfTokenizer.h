/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TOKENIZER_H
#define PDF_TOKENIZER_H

#include "PdfDefines.h"
#include "PdfInputDevice.h"

#include <deque>
#include <sstream>

namespace mm {

class PdfEncrypt;
class PdfVariant;

enum class PdfTokenType
{
    Unknown = 0,
    Literal,
    ParenthesisLeft,
    ParenthesisRight,
    BraceLeft,
    BraceRight,
    AngleBracketLeft,
    AngleBracketRight,
    DoubleAngleBracketsLeft,
    DoubleAngleBracketsRight,
    SquareBracketLeft,
    SquareBracketRight,
    Slash,
};

/**
 * A simple tokenizer for PDF files and PDF content streams
 */
class PDFMM_API PdfTokenizer
{
    friend class PdfParser;
    friend class PdfParserObject;

public:
    static constexpr unsigned BufferSize = 4096;

public:
    PdfTokenizer(bool readReferences = true);
    PdfTokenizer(const std::shared_ptr<chars>& buffer, bool readReferences = true);

    /** Reads the next token from the current file position
     *  ignoring all comments.
     *
     *  \param[out] token On true return, set to a pointer to the read
     *                     token (a nullptr-terminated C string). The pointer is
     *                     to memory owned by PdfTokenizer and must NOT be
     *                     freed.  The contents are invalidated on the next
     *                     call to tryReadNextToken(..) and by the destruction of
     *                     the PdfTokenizer. Undefined on false return.
     *
     *  \param[out] tokenType On true return, if not nullptr the type of the read token
     *                     will be stored into this parameter. Undefined on false
     *                     return.
     *
     *  \returns           True if a token was read, false if there are no
     *                     more tokens to read.
     */
    bool TryReadNextToken(PdfInputDevice& device, std::string_view& token);
    bool TryReadNextToken(PdfInputDevice& device, std::string_view& token, PdfTokenType& tokenType);

    /** Reads the next token from the current file position
     *  ignoring all comments and compare the passed token
     *  to the read token.
     *
     *  If there is no next token available, throws UnexpectedEOF.
     *
     *  \param token a token that is compared to the
     *                  read token
     *
     *  \returns true if the read token equals the passed token.
     */
    bool IsNextToken(PdfInputDevice& device, const std::string_view& token);

    /** Read the next number from the current file position
     *  ignoring all comments.
     *
     *  Raises NoNumber exception if the next token is no number, and
     *  UnexpectedEOF if no token could be read. No token is consumed if
     *  NoNumber is thrown.
     *
     *  \returns a number read from the input device.
     */
    int64_t ReadNextNumber(PdfInputDevice& device);

    /** Read the next variant from the current file position
     *  ignoring all comments.
     *
     *  Raises an UnexpectedEOF exception if there is no variant left in the
     *  file.
     *
     *  \param variant write the read variant to this value
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt = nullptr);

public:
    /** Returns true if the given character is a whitespace
     *  according to the pdf reference
     *
     *  \returns true if it is a whitespace character otherwise false
     */
    static bool IsWhitespace(unsigned char ch);

    /** Returns true if the given character is a delimiter
     *  according to the pdf reference
     */
    static bool IsDelimiter(unsigned char ch);

    /** Returns true if the given character is a token delimiter
     */
    static bool IsTokenDelimiter(unsigned char ch, PdfTokenType& tokenType);

    /**
     * True if the passed character is a regular character according to the PDF
     * reference (Section 3.1.1, Character Set); ie it is neither a white-space
     * nor a delimiter character.
     */
    static bool IsRegular(unsigned char ch);

    /**
     * True if the passed character is within the generally accepted "printable"
     * ASCII range.
     */
    static bool IsPrintable(unsigned char ch);

    /**
     * Get the hex value from a static map of a given hex character (0-9, A-F, a-f).
     *
     * \param ch hex character
     *
     * \returns hex value or HEX_NOT_FOUND if invalid
     *
     * \see HEX_NOT_FOUND
     */
    static int GetHexValue(unsigned char ch);

    /**
     * Constant which is returned for invalid hex values.
     */
    static constexpr unsigned HEX_NOT_FOUND = std::numeric_limits<unsigned>::max();

protected:
    // This enum differs from regular PdfDataType in the sense
    // it enumerates only data types that can be determined literally
    // by the tokenization and specify better if the strings literals
    // are regular or hex strings
    enum class PdfLiteralDataType
    {
        Unknown = 0,
        Bool,
        Number,
        Real,
        String,
        HexString,
        Name,
        Array,
        Dictionary,
        Null,
        Reference,
    };

protected:
    /** Read the next variant from the current file position
     *  ignoring all comments.
     *
     *  Raises an exception if there is no variant left in the file.
     *
     *  \param token a token that has already been read
     *  \param type type of the passed token
     *  \param variant write the read variant to this value
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(PdfInputDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, PdfEncrypt* encrypt);
    bool TryReadNextVariant(PdfInputDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Add a token to the queue of tokens.
     *  tryReadNextToken() will return all enqueued tokens first before
     *  reading new tokens from the input device.
     *
     *  \param token string of the token
     *  \param type type of the token
     *
     *  \see tryReadNextToken
     */
    void EnqueueToken(const std::string_view& token, PdfTokenType type);

    /** Read a dictionary from the input device
     *  and store it into a variant.
     *
     *  \param variant store the dictionary into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadDictionary(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Read an array from the input device
     *  and store it into a variant.
     *
     *  \param variant store the array into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadArray(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Read a string from the input device
     *  and store it into a variant.
     *
     *  \param variant store the string into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadString(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Read a hex string from the input device
     *  and store it into a variant.
     *
     *  \param variant store the hex string into this variable
     *  \param encrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadHexString(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Read a name from the input device
     *  and store it into a variant.
     *
     *  Throws UnexpectedEOF if there is nothing to read.
     *
     *  \param variant store the name into this variable
     */
    void ReadName(PdfInputDevice& device, PdfVariant& variant);

    /** Determine the possible datatype of a token.
     *  Numbers, reals, bools or nullptr values are parsed directly by this function
     *  and saved to a variant.
     *
     *  \returns the expected datatype
     */
    PdfLiteralDataType DetermineDataType(PdfInputDevice& device, const std::string_view& token, PdfTokenType tokenType, PdfVariant& variant);

private:
    bool tryReadDataType(PdfInputDevice& device, PdfLiteralDataType dataType, PdfVariant& variant, PdfEncrypt* encrypt);

private:
    typedef std::pair<std::string, PdfTokenType> TokenizerPair;
    typedef std::deque<TokenizerPair> TokenizerQueque;

private:
    std::shared_ptr<chars> m_buffer;
    bool m_readReferences;
    TokenizerQueque m_tokenQueque;
    chars m_charBuffer;

    // An istringstream which is used
    // to read double values instead of strtod
    // which is locale depend.
    std::istringstream m_doubleParser;
};

};

#endif // PDF_TOKENIZER_H
