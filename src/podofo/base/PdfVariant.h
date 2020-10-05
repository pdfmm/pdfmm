/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_VARIANT_H_
#define _PDF_VARIANT_H_

#include "PdfDefines.h"
#include "PdfRefCountedBuffer.h"
#include "PdfString.h"
#include "PdfName.h"

namespace PoDoFo {

class PdfArray;
class PdfData;
class PdfDataType;
class PdfDictionary;
class PdfEncrypt;
class PdfOutputDevice;
class PdfString;
class PdfReference;

/**
 * A variant data type which supports all data types supported by the PDF standard.
 * The data can be parsed directly from a string or set by one of the members.
 * One can also convert the variant back to a string after setting the values.
 *
 * \warning All methods not marked otherwise may trigger a deferred load. This means
 *          that they are unsafe to call while a deferred load is already in progress
 *          (as recursion will occurr).
 *
 * TODO: domseichter: Make this class implicitly shared
 */
class PODOFO_API PdfVariant final
{
    friend class PdfArray;
    friend class PdfDictionary;

private:
    PdfVariant(EPdfDataType type);

public:

    static PdfVariant NullValue;

    /** Construct an empty variant type
     *  IsNull() will return true.
     */
    PdfVariant();

    /** Construct a PdfVariant that is a bool.
     *  \param b the boolean value of this PdfVariant
     */
    PdfVariant( bool b );

    /** Construct a PdfVariant that is a number.
     *  \param l the value of the number.
     */
    PdfVariant( int64_t l );

    /** Construct a PdfVariant that is a real number.
     *  \param d the value of the real number.
     */    
    PdfVariant( double d );

    /** Construct a PdfVariant that is a string. The argument
     * string will be escaped where necessary, so it should be
     * passed in unescaped form.
     *
     *  \param rsString the value of the string
     */        
    PdfVariant( const PdfString & rsString );

    /** Construct a PdfVariant that is a name.
     *  \param rName the value of the name
     */        
    PdfVariant( const PdfName & rName );

    /** Construct a PdfVariant that is a name.
     *  \param rRef the value of the name
     */        
    PdfVariant( const PdfReference & rRef );

    /** Construct a PdfVariant object with array data.
     *  The variant will automatically get the datatype
     *  EPdfDataType::Array. This constructor is the fastest
     *  way to create a new PdfVariant that is an array.
     *
     *  \param tList a list of variants
     */
    PdfVariant( const PdfArray & tList );

    /** Construct a PdfVariant that is a dictionary.
     *  \param rDict the value of the dictionary.
     */        
    PdfVariant( const PdfDictionary & rDict );

    /** Construct a PdfVariant that contains raw PDF data.
     *  \param rData raw and valid PDF data.
     */        
    PdfVariant( const PdfData & rData );

    /** Constructs a new PdfVariant which has the same 
     *  contents as rhs.
     *  \param rhs an existing variant which is copied.
     */
    PdfVariant( const PdfVariant & rhs );

    ~PdfVariant();

    /** Clear all internal member variables and free the memory
     *  they have allocated.
     *  Sets the datatype to EPdfDataType::Null
     *
     *  This will reset the dirty flag of this object to be clean.
     *  \see IsDirty
     */
    void Clear();

    /** \returns a human readable string representation of GetDataType()
     *  The returned string must not be free'd.
     */
    const char * GetDataTypeString() const;

    /** \returns true if this variant is a bool
     */
    bool IsBool() const;

    /** \returns true if this variant is an integer
     */
    bool IsNumber() const;

    /** \returns true if this variant is a real
     *
     *  This method strictly check for a floating point number and return false on integer
     */
    bool IsRealStrict() const;

    /** \returns true if this variant is an integer or a floating point number
     */
    bool IsNumberOrReal() const;

    /** \returns true if this variant is a string
     */
    bool IsString() const;

    /** \returns true if this variant is a name
     */
    bool IsName() const;

    /** \returns true if this variant is an array
     */
    bool IsArray() const;

    /** \returns true if this variant is a dictionary
     */
    bool IsDictionary() const;

    /** \returns true if this variant is raw data
     */
    bool IsRawData() const;

    /** \returns true if this variant is null
     */
    bool IsNull() const;

    /** \returns true if this variant is a reference
     */
    bool IsReference() const;
       
    /** Converts the current object into a string representation
     *  which can be written directly to a PDF file on disc.
     *  \param rsData the object string is returned in this object.
     *  \param eWriteMode additional options for writing to a string
     */
    void ToString( std::string & rsData, EPdfWriteMode eWriteMode = EPdfWriteMode::Clean ) const;

    /** Get the value if this object is a bool.
     *  \returns the bool value.
     */
    bool GetBool() const;
    bool TryGetBool(bool& value) const;

    /** Get the value of the object as int64_t.
     *
     *  This method is lenient and narrows floating point numbers
     *  \return the value of the number
     */
    int64_t GetNumberLenient() const;
    bool TryGetNumberLenient(int64_t& value) const;

    /** Get the value of the object as int64_t
     *
     *  This method throws if the numer is a floating point number
     *  \return the value of the number
     */
    int64_t GetNumber() const;
    bool TryGetNumber(int64_t& value) const;

    /** Get the value of the object as a floating point
     *
     *  This method is lenient and returns also strictly integral numbers
     *  \return the value of the number
     */
    double GetReal() const;
    bool TryGetReal(double& value) const;

    /** Get the value of the object as floating point number
     *
     *  This method throws if the numer is integer
     *  \return the value of the number
     */
    double GetRealStrict() const;
    bool TryGetRealStrict(double& value) const;

    /** \returns the value of the object as string.
     */
    const PdfString & GetString() const;
    bool TryGetString(const PdfString *& str) const;

    /** \returns the value of the object as name
     */
    const PdfName & GetName() const;
    bool TryGetName(const PdfName*& str) const;

    /** Get the reference values of this object.
     *  \returns a PdfReference
     */
    PdfReference GetReference() const;
    bool TryGetReference(PdfReference& ref) const;

    /** Get the reference values of this object.
     *  \returns a reference to the PdfData instance.
     */
    const PdfData& GetRawData() const;
    PdfData& GetRawData();
    bool TryGetRawData(const PdfData*& data) const;
    bool TryGetRawData(PdfData*& data);

    /** Returns the value of the object as array
     *  \returns a array
     */
    const PdfArray & GetArray() const;
    PdfArray & GetArray();
    bool TryGetArray(const PdfArray*& arr) const;
    bool TryGetArray(PdfArray*& arr);

    /** Returns the dictionary value of this object
     *  \returns a PdfDictionary
     */
    const PdfDictionary & GetDictionary() const;
    PdfDictionary & GetDictionary();
    bool TryGetDictionary(const PdfDictionary *& dict) const;
    bool TryGetDictionary(PdfDictionary*& dict);

    /** Set the value of this object as bool
     *  \param b the value as bool.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetBool(bool b);

    /** Set the value of this object as int64_t
     *  \param l the value as int64_t.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetNumber(int64_t l);

    /** Set the value of this object as double
     *  \param d the value as double.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void SetReal(double d);

    /** Set the name value of this object
    *  \param d the name value
    *
    *  This will set the dirty flag of this object.
    *  \see IsDirty
    */
    void SetName(const PdfName &name);

    /** Set the string value of this object.
     * \param str the string value
     *
     * This will set the dirty flag of this object.
     * \see IsDirty
     */
    void SetString(const PdfString & str);

    void SetReference(const PdfReference &ref);

    /** Write the complete variant to an output device.
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    void Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode,
        const PdfEncrypt* pEncrypt) const;

    /** Assign the values of another PdfVariant to this one.
     *  \param rhs an existing variant which is copied.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    const PdfVariant & operator=( const PdfVariant & rhs );

    /**
     * Test to see if the value contained by this variant is the same
     * as the value of the other variant.
     */
    bool operator==( const PdfVariant & rhs ) const;

    /**
     * \see operator==
     */
    bool operator!=( const PdfVariant & rhs) const;

public:
    inline EPdfDataType GetDataType() const { return m_eDataType; }

private:
    bool tryGetDictionary(PdfDictionary*& dict) const;
    bool tryGetArray(PdfArray*& arr) const;

private:
    /**
     * It's an easy mistake to pass a pointer to a PdfVariant when trying to
     * copy a PdfVariant, especially with heap allocators like `new'. This can
     * produce confusing and unexpected results like getting a PdfVariant(bool).
     *
     * A similar issue can arise when the user passes a `char*' and expects a PdfName
     * or PdfString based variant to be created. We can't know which they wanted, so
     * we should fail, especially since the compiler tends to convert pointers to bool
     * for extra confusion value.
     *
     * We provide this overload so that such attempts will fail with an error about
     * a private ctor. If you're reading this, you wrote:
     *
     *  PdfVariant( my_ptr_to_something )
     *
     *... not ...
     * 
     *  PdfVariant( *my_ptr_to_something )
     *
     * If you need to modify PdfVariant to legitimately take a pointer in the future,
     * you can do so by providing a template specialization, or by removing this check
     * and replacing it with a couple of overloads specific to PdfObject*, PdfVariant*,
     * and char* (at least).
     */
    template<typename T> PdfVariant(T*);

    /** To reduce memory usage of this very often used class,
     *  we use a union here, as there is always only
     *  one of those members used.
     */
    typedef union
    {
        /** Holds references, strings, 
         *  names, dictionaries and arrays
         */
        PdfDataType* pData;
        bool bBoolValue;
        double dNumber;
        int64_t nNumber;
    } UVariant;

    UVariant m_Data;
    EPdfDataType m_eDataType;
};

};

#endif // _PDF_VARIANT_H_
