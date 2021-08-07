/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_NAME_H
#define PDF_NAME_H

#include "PdfDefines.h"

#include "PdfDataType.h"

namespace mm {

class PdfOutputDevice;
class PdfName;

/** This class represents a PdfName.
 *  Whenever a key is required you have to use a PdfName object.
 *
 *  PdfName are required as keys in PdfObject and PdfVariant objects.
 *
 *  PdfName may have a maximum length of 127 characters.
 *
 *  \see PdfObject \see PdfVariant
 */
class PDFMM_API PdfName final : public PdfDataType
{
public:
    /** Constructor to create nullptr strings.
     *  use PdfName::KeyNull instead of this constructor
     */
    PdfName();

    /** Create a new PdfName object.
     *  \param str the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *                 Has to be a zero terminated string.
     */
    PdfName(const std::string_view& str);
    PdfName(const char* str);
    PdfName(const std::string& str);

    // Delete constructor with nullptr
    PdfName(nullptr_t) = delete;

    /** Create a copy of an existing PdfName object.
     *  \param rhs another PdfName object
     */
    PdfName(const PdfName& rhs);

    static PdfName FromRaw(const std::string_view& rawcontent);

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *
     *  \param name A string containing the escaped name
     *  \return A new PdfName
     */
    static PdfName FromEscaped(const std::string_view& name);

    /** \return an escaped representation of this name
     *          without the leading / .
     *
     *  There is no corresponding GetEscapedLength(), since
     *  generating the return value is somewhat expensive.
     */
    std::string GetEscapedName() const;

    void Write(PdfOutputDevice& device, PdfWriteMode writeMode, const PdfEncrypt* encrypt) const override;

    /** \returns the unescaped value of this name object
     *           without the leading slash
     */
    const std::string& GetString() const;

    /** \returns the unescaped length of this
     *           name object
     */
    size_t GetLength() const;

    /** \returns the raw data of this name object
     */
    const std::string& GetRawData() const;

    /** Assign another name to this object
     *  \param rhs another PdfName object
     */
    const PdfName& operator=(const PdfName& rhs);

    /** compare to PdfName objects.
     *  \returns true if both PdfNames have the same value.
     */
    bool operator==(const PdfName& rhs) const;
    bool operator==(const char* str) const;
    bool operator==(const std::string& str) const;
    bool operator==(const std::string_view& view) const;

    /** compare two PdfName objects.
     *  \returns true if both PdfNames have different values.
     */
    bool operator!=(const PdfName& rhs) const;
    bool operator!=(const char* str) const;
    bool operator!=(const std::string& str) const;
    bool operator!=(const std::string_view& view) const;

    /** compare two PdfName objects.
     *  Used for sorting in lists
     *  \returns true if this object is smaller than rhs
     */
    bool operator<(const PdfName& rhs) const;

    // TODO: Move these somewhere else
    static const PdfName KeyContents;
    static const PdfName KeyFlags;
    static const PdfName KeyLength;
    static const PdfName KeyNull;
    static const PdfName KeyRect;
    static const PdfName KeySize;
    static const PdfName KeySubtype;
    static const PdfName KeyType;
    static const PdfName KeyFilter;

private:
    PdfName(const std::shared_ptr<std::string>& rawdata);
    void expandUtf8String() const;
    void initFromUtf8String(const std::string_view& view);

private:
    // The unescaped name raw data, without leading '/'.
    // It can store also the utf8 expanded string, if coincident
    std::shared_ptr<std::string> m_data;

    bool m_isUtf8Expanded;
    std::shared_ptr<std::string> m_utf8String;
};

};

#endif // PDF_NAME_H
