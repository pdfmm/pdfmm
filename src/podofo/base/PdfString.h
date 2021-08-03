/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_STRING_H
#define PDF_STRING_H

#include "PdfDefines.h"

#include "PdfDataType.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfOutputDevice;

/** A string that can be written to a PDF document.
 *  If it contains binary data it is automatically
 *  converted into a hex string, otherwise a normal PDF
 *  string is written to the document.
 *
 *  PdfString is an implicitly shared class. As a reason
 *  it is very fast to copy PdfString objects.
 *
 */
class PODOFO_API PdfString final : public PdfDataType
{
    friend class PdfTokenizer;
public:
    /** Create an empty and invalid string
     */
    PdfString();

    PdfString(const char* str);

    /** Construct a new PdfString from a utf-8 string
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
    PdfString(const std::string_view& view);

    /** Copy an existing PdfString
     *  \param rhs another PdfString to copy
     */
    PdfString(const PdfString& rhs);

    // Delete constructor with nullptr
    PdfString(nullptr_t) = delete;

    /** Construct a new PdfString from an utf-8 encoded string.
     *
     *  \param view a buffer
     *  \param hex true if the string should be written as hex string
     */
    static PdfString FromRaw(const std::string_view& view, bool hex = true);

    /** Set hex-encoded data as the strings data.
     *  \param pszHex must be hex-encoded data.
     *  \param lLen   length of the hex-encoded data.
     *  \param pEncrypt if !nullptr, assume the hex data is encrypted and should be decrypted after hex-decoding.
     */
    static PdfString FromHexData(const std::string_view& view, PdfEncrypt* encrypt = nullptr);

    /** Check if this is a hex string.
     *
     *  If true the data will be hex-encoded when the string is written to
     *  a PDF file.
     *
     *  \returns true if this is a hex string.
     *  \see GetString() will return the raw string contents (not hex-encoded)
     */
    inline bool IsHex() const { return m_isHex; }

    /**
     * PdfStrings are either PdfDocEncoded, or Unicode encoded (UTF-16BE or UTF-8) strings.
     *
     * This function returns true if this is an Unicode string object.
     *
     * \returns true if this is an Unicode string.
     */
    bool IsUnicode() const;

    /** The contents of the string as UTF-8 string.
     *
     *  The string's contents are always returned as
     *  UTF-8 by this function. Works for Unicode strings
     *  and for non-Unicode strings.
     *
     *  This is the preferred way to access the string's contents.
     *
     *  \returns the string's contents always as UTF-8
     */
    const std::string& GetString() const;

    const std::string& GetRawData() const;

    /** The length of the string data returned by GetString()
     *  in bytes not including terminating zero ('\0') bytes.
     *
     *  \returns the length of the string,
     *           returns zero if PdfString::IsValid() returns false
     *
     *  \see GetCharacterLength to determine the number of characters in the string
     */
    size_t GetLength() const;

    /** Write this PdfString in PDF format to a PdfOutputDevice.
     *
     *  \param pDevice the output device.
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object,
     *                  or nullptr to not encrypt this object
     */
    void Write(PdfOutputDevice& device, PdfWriteMode writeMode, const PdfEncrypt* encrypt) const;

    /** Copy an existing PdfString
     *  \param rhs another PdfString to copy
     *  \returns this object
     */
    const PdfString& operator=(const PdfString& rhs);

    /** Comparison operator
     *
     *  UTF-8 and strings of the same data compare equal. Whether
     *  the string will be written out as hex is not considered - only the real "text"
     *  is tested for equality.
     *
     *  \param rhs compare to this string object
     *  \returns true if both strings have the same contents
     */
    bool operator==(const PdfString& rhs) const;
    bool operator==(const char* str) const;
    bool operator==(const std::string& str) const;
    bool operator==(const std::string_view& view) const;

    /** Comparison operator
     *  \param rhs compare to this string object
     *  \returns true if strings have different contents
     */
    bool operator!=(const PdfString& rhs) const;
    bool operator!=(const char* str) const;
    bool operator!=(const std::string& str) const;
    bool operator!=(const std::string_view& view) const;

private:
    PdfString(const std::shared_ptr<std::string>& data, bool isHex);

    /** Construct a new PdfString from a 0-terminated string.
     *
     *  The input string will be copied.
     *  if m_bHex is true the copied data will be hex-encoded.
     *
     *  \param pszString the string to copy, must not be nullptr
     *  \param lLen length of the string data to copy
     *
     */
    void initFromUtf8String(const std::string_view& view);
    void evaluateString() const;
    bool isValidText() const;
    static bool canPerformComparison(const PdfString& lhs, const PdfString& rhs);

private:
    enum class StringState : uint8_t
    {
        RawBuffer,
        PdfDocEncoding,
        Unicode
    };

private:
    std::shared_ptr<std::string> m_data;
    StringState m_state;
    bool m_isHex;    // This string is converted to hex during writing it out
};

}

#endif // PDF_STRING_H
