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
#include "PdfRefCountedBuffer.h"
#include "PdfRefCountedInputDevice.h"

#include <deque>
#include <sstream>

namespace PoDoFo {

class PdfEncrypt;
class PdfVariant;

enum class EPdfTokenType
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

typedef std::pair<std::string,EPdfTokenType> TTokenizerPair;
typedef std::deque<TTokenizerPair> TTokenizerQueque;
typedef TTokenizerQueque::iterator TITokenizerQueque;
typedef TTokenizerQueque::const_iterator TCITokenizerQueque;

/**
 * A simple tokenizer for PDF files and PDF content streams
 */
class PODOFO_API PdfTokenizer
{
    friend class PdfParser;
    friend class PdfParserObject;

public:
    static constexpr unsigned BufferSize = 4096;

public:
    PdfTokenizer(bool readReferences = true);
    PdfTokenizer(const PdfRefCountedBuffer& rBuffer, bool readReferences = true);

    /** Reads the next token from the current file position
     *  ignoring all comments.
     *
     *  \param[out] pszToken On true return, set to a pointer to the read
     *                     token (a nullptr-terminated C string). The pointer is
     *                     to memory owned by PdfTokenizer and must NOT be
     *                     freed.  The contents are invalidated on the next
     *                     call to tryReadNextToken(..) and by the destruction of
     *                     the PdfTokenizer. Undefined on false return.
     *
     *  \param[out] peType On true return, if not nullptr the type of the read token
     *                     will be stored into this parameter. Undefined on false
     *                     return.
     *
     *  \returns           True if a token was read, false if there are no
     *                     more tokens to read.
     */
    bool TryReadNextToken(PdfInputDevice& device, std::string_view& token);
    bool TryReadNextToken(PdfInputDevice& device, std::string_view& token, EPdfTokenType& tokenType);

    /** Reads the next token from the current file position
     *  ignoring all comments and compare the passed token
     *  to the read token.
     *
     *  If there is no next token available, throws UnexpectedEOF.
     *
     *  \param pszToken a token that is compared to the
     *                  read token
     *
     *  \returns true if the read token equals the passed token.
     */
    bool IsNextToken(PdfInputDevice& device, const std::string_view& pszToken);

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
     *  \param rVariant write the read variant to this value
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt = nullptr);

public:
    PdfRefCountedBuffer& GetBuffer() { return m_buffer; }

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
    static bool IsTokenDelimiter(unsigned char ch, EPdfTokenType& tokenType);

    /**
     * True if the passed character is a regular character according to the PDF
     * reference (Section 3.1.1, Character Set); ie it is neither a white-space
     * nor a delimeter character.
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
    enum class EPdfLiteralDataType
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
     *  \param pszToken a token that has already been read
     *  \param eType type of the passed token
     *  \param rVariant write the read variant to this value
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(PdfInputDevice& device, const std::string_view& token, EPdfTokenType eType, PdfVariant& variant, PdfEncrypt* encrypt);
    bool TryReadNextVariant(PdfInputDevice& device, const std::string_view& token, EPdfTokenType eType, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Add a token to the queue of tokens.
     *  tryReadNextToken() will return all enqueued tokens first before
     *  reading new tokens from the input device.
     *
     *  \param pszToken string of the token
     *  \param eType type of the token
     *
     *  \see tryReadNextToken
     */
    void EnqueueToken(const std::string_view& pszToken, EPdfTokenType eType);

    /** Read a dictionary from the input device
     *  and store it into a variant.
     *
     *  \param rVariant store the dictionary into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadDictionary(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Read an array from the input device
     *  and store it into a variant.
     *
     *  \param rVariant store the array into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadArray(PdfInputDevice& device, PdfVariant& variant, PdfEncrypt* encrypt);

    /** Read a string from the input device
     *  and store it into a variant.
     *
     *  \param rVariant store the string into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadString(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt );

    /** Read a hex string from the input device
     *  and store it into a variant.
     *
     *  \param rVariant store the hex string into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadHexString(PdfInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt);

    /** Read a name from the input device
     *  and store it into a variant.
     *
     *  Throws UnexpectedEOF if there is nothing to read.
     *
     *  \param rVariant store the name into this variable
     */
    void ReadName(PdfInputDevice & device, PdfVariant & variant);

    /** Determine the possible datatype of a token.
     *  Numbers, reals, bools or nullptr values are parsed directly by this function
     *  and saved to a variant.
     *
     *  \returns the expected datatype
     */
    EPdfLiteralDataType DetermineDataType(PdfInputDevice& device, const std::string_view& token, EPdfTokenType eType, PdfVariant& variant);

private:
    bool tryReadDataType(PdfInputDevice& device, EPdfLiteralDataType eDataType, PdfVariant& rVariant, PdfEncrypt* pEncrypt);

private:
    PdfRefCountedBuffer m_buffer;
    bool m_readReferences;
    TTokenizerQueque m_deqQueque;
    buffer_t m_charBuffer;

    // An istringstream which is used
    // to read double values instead of strtod
    // which is locale depend.
    std::istringstream m_doubleParser;
};

};

#endif // PDF_TOKENIZER_H
