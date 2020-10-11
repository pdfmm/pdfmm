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

#ifndef _PDF_DICTIONARY_H_
#define _PDF_DICTIONARY_H_

#include <map>

#include "PdfDefines.h"
#include "PdfContainerDataType.h"

#include "PdfName.h"
#include "PdfObject.h"

namespace PoDoFo {

typedef std::map<PdfName,PdfObject>      TKeyMap;
typedef TKeyMap::iterator                TIKeyMap;
typedef TKeyMap::const_iterator          TCIKeyMap;

class PdfOutputDevice;

/** The PDF dictionary data type of PoDoFo (inherits from PdfDataType,
 *  the base class for such representations)
 */
class PODOFO_API PdfDictionary : public PdfContainerDataType
{
    friend class PdfObject;
public:
    /** Create a new, empty dictionary
     */
    PdfDictionary();

    /** Deep copy a dictionary
     *  \param rhs the PdfDictionary to copy
     */
    PdfDictionary( const PdfDictionary & rhs );

    /** Asignment operator.
     *  Asign another PdfDictionary to this dictionary. This is a deep copy;
     *  all elements of the source dictionary are duplicated.
     *
     *  \param rhs the PdfDictionary to copy.
     *
     *  \return this PdfDictionary
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    const PdfDictionary & operator=( const PdfDictionary & rhs );

    /**
     * Comparison operator. If this dictionary contains all the same keys
     * as the other dictionary, and for each key the values compare equal,
     * the dictionaries are considered equal.
     */
    bool operator==( const PdfDictionary& rhs ) const;

    /**
     * \see operator==
     */
    bool operator!=( const PdfDictionary& rhs ) const;

    /** Removes all keys from the dictionary
     */
    void Clear();

    /** Add a key to the dictionary. If an existing key of this name exists, its
     *  value is replaced and the old value object will be deleted. The passed
     *  object is copied.
     *
     *  \param identifier the key is identified by this name in the dictionary
     *  \param rObject a variant object containing the data. The object is copied.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    PdfObject & AddKey( const PdfName & identifier, const PdfObject & rObject );

    // REMOVE-ME
    void AddKey( const PdfName & identifier, const PdfObject* pObject );

    /** Get the key's value out of the dictionary.
     *
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key named key in the dictionary
     * 
     *  \returns pointer to the found value, or 0 if the key was not found.
     */
    const PdfObject* GetKey( const PdfName & key ) const;

    /** Get the key's value out of the dictionary.  This is an overloaded member
     * function.
     *
     * The returned value is a pointer to the internal object in the dictionary.
     * It may be modified but is still owned by the dictionary so it MUST not
     * be deleted.
     *
     *  \param key look for the key named key in the dictionary
     * 
     *  \returns the found value, or 0 if the key was not found.
     */
    PdfObject* GetKey( const PdfName & key );

    /** Get the keys value out of the dictionary
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key names pszKey in the dictionary
     *  \returns pointer to the found value or 0 if the key was not found.
     */
    const PdfObject* FindKey( const PdfName & key ) const;
    PdfObject* FindKey( const PdfName & key );

    /** Get the keys value out of the dictionary
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * Also lookup the parent objects, if /Parent key is found in the dictionary.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key names pszKey in the dictionary
     *  \returns pointer to the found value or 0 if the key was not found.
     */
    const PdfObject* FindKeyParent( const PdfName & key ) const;
    PdfObject* FindKeyParent( const PdfName & key );

    /** Get the key's value out of the dictionary.
     *
     * The returned value is a reference to the internal object in the dictionary
     * so it MUST not be deleted. If the key is not found, this throws a PdfError
     * exception with error code EPdfError::NoObject, instead of returning.
     * This is intended to make code more readable by sparing (especially multiple)
     * nullptr checks.
     *
     *  \param key look for the key named key in the dictionary
     * 
     *  \returns reference to the found value (never 0).
     *  \throws PdfError(EPdfError::NoObject).
     */
    const PdfObject& MustGetKey( const PdfName & key ) const;

    bool GetKeyAsBool(const PdfName& key, bool defvalue = false) const;

    int64_t GetKeyAsNumber(const PdfName & key, int64_t defvalue = 0) const;

    int64_t GetKeyAsNumberLenient(const PdfName& key, int64_t defvalue = 0) const;

    double GetKeyAsReal(const PdfName & key, double defvalue = 0.0) const;

    double GetKeyAsRealStrict(const PdfName& key, double defvalue = 0.0) const;

    PdfName GetKeyAsName(const PdfName& key, const PdfName& defvalue = PdfName::KeyNull) const;

    PdfString GetKeyAsString(const PdfName & key, const PdfString & defvalue = PdfString::StringNull) const;

    PdfReference GetKeyAsReference(const PdfName & key, const PdfReference & defvalue = { }) const;

    /** Allows to check if a dictionary contains a certain key.
     * \param key look for the key named key.Name() in the dictionary
     *
     *  \returns true if the key is part of the dictionary, otherwise false.
     */
    bool  HasKey( const PdfName & key  ) const;

    /** Remove a key from this dictionary.  If the key does not exist, this
     * function does nothing.
     *
     *  \param identifier the name of the key to delete
     * 
     *  \returns true if the key was found in the object and was removed.
     *  If there was no key with this name, false is returned.
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    bool RemoveKey( const PdfName & identifier );

    /** Write the complete dictionary to a file.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object
     */
    void Write( PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt ) const override;

    /**
    *  \returns the size of the internal map
    */
    size_t GetSize() const;

 public:
     TIKeyMap begin();
     TIKeyMap end();
     TCIKeyMap begin() const;
     TCIKeyMap end() const;

 protected:
     void ResetDirtyInternal() override;
     void SetOwner( PdfObject* pOwner ) override;

 private:
     PdfObject& addKey(const PdfName& identifier, const PdfObject& rObject);
     PdfObject * getKey(const PdfName & key) const;
     PdfObject * findKey(const PdfName & key) const;
     PdfObject * findKeyParent(const PdfName & key) const;

 private: 
    TKeyMap      m_mapKeys; 
};

typedef std::vector<PdfDictionary*>      TVecDictionaries; 
typedef	TVecDictionaries::iterator       TIVecDictionaries; 
typedef	TVecDictionaries::const_iterator TCIVecDictionaries;

};

#endif // _PDF_DICTIONARY_H_
