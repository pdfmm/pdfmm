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

#ifndef _PDF_NAME_H_
#define _PDF_NAME_H_

#include "PdfDefines.h"

#include <memory>

#include "PdfDataType.h"

namespace PoDoFo {

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
class PODOFO_API PdfName : public PdfDataType
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
    PdfName(const std::string_view& view);
    PdfName(const std::string& str);

    /** Create a new PdfName object.
     *  \param pszName the unescaped value of this name. Please specify
     *                 the name without the leading '/'.
     *  \param lLen    length of the name
     */
    PdfName(const char* pszName);
    PdfName(const char* pszName, size_t lLen);

    /** Create a copy of an existing PdfName object.
     *  \param rhs another PdfName object
     */
    PdfName(const PdfName& rhs);

    static PdfName FromRaw(const std::string_view &rawcontent);

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *
     *  \param sName A string containing the escaped name
     *  \return A new PdfName
     */
    static PdfName FromEscaped(const std::string_view& sName);

    /** Create a new PdfName object from a string containing an escaped
     *  name string without the leading / .
     *  \param pszName A string containing the escaped name
     *  \param ilength length of the escaped string data. If a length
     *                 of 0 is passed, the string data is expected to 
     *                 be a zero terminated string.
     *  \return A new PdfName
     */
    static PdfName FromEscaped(const char * pszName, size_t ilength = 0 );

    /** \return an escaped representation of this name
     *          without the leading / .
     *
     *  There is no corresponding GetEscapedLength(), since
     *  generating the return value is somewhat expensive.
     */
    std::string GetEscapedName() const;

    /** Write the name to an output device in PDF format.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object     
     */
    void Write( PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt) const override;

    /** \returns the unescaped value of this name object
     *           without the leading slash
     */
    const std::string& GetString() const;

    /** \returns the unescaped length of this
     *           name object
     */
    size_t GetLength() const;

    /** Assign another name to this object
     *  \param rhs another PdfName object
     */
    const PdfName& operator=( const PdfName & rhs );

    /** compare to PdfName objects.
     *  \returns true if both PdfNames have the same value.
     */
    bool operator==( const PdfName & rhs ) const;
    bool operator==(const char * str) const;
    bool operator==(const std::string & str) const;
    bool operator==(const std::string_view &view) const;

    /** compare two PdfName objects.
     *  \returns true if both PdfNames have different values.
     */
    bool operator!=( const PdfName & rhs ) const;
    bool operator!=(const char* str) const;
    bool operator!=(const std::string& str) const;
    bool operator!=(const std::string_view& view) const;

    /** compare two PdfName objects.
     *  Used for sorting in lists
     *  \returns true if this object is smaller than rhs
     */
    bool operator<( const PdfName & rhs ) const;

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
    PdfName(const std::shared_ptr<std::string> &rawdata);
    void expandUtf8String() const;
    void initFromUtf8String(const std::string_view &view);

private:
    // The unescaped name raw data, without leading '/'.
    // It can store also the utf8 expanded string, if coincident
    std::shared_ptr<std::string> m_data;

    bool m_isUtf8Expanded;
    std::shared_ptr<std::string> m_utf8String;
};

};

#endif /* _PDF_NAME_H_ */

