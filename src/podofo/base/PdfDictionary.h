/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DICTIONARY_H
#define PDF_DICTIONARY_H

#include <map>

#include "PdfDefines.h"

#include "PdfContainerDataType.h"
#include "PdfName.h"
#include "PdfObject.h"

namespace PoDoFo {

typedef std::map<PdfName,PdfObject> TKeyMap;
typedef TKeyMap::iterator TIKeyMap;
typedef TKeyMap::const_iterator TCIKeyMap;

class PdfOutputDevice;

/** The PDF dictionary data type of PoDoFo (inherits from PdfDataType,
 *  the base class for such representations)
 */
class PODOFO_API PdfDictionary final : public PdfContainerDataType
{
    friend class PdfObject;
    friend class PdfTokenizer;

public:
    /** Create a new, empty dictionary
     */
    PdfDictionary();

    /** Deep copy a dictionary
     *  \param rhs the PdfDictionary to copy
     */
    PdfDictionary(const PdfDictionary& rhs);

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
    const PdfDictionary& operator=(const PdfDictionary& rhs);

    /**
     * Comparison operator. If this dictionary contains all the same keys
     * as the other dictionary, and for each key the values compare equal,
     * the dictionaries are considered equal.
     */
    bool operator==(const PdfDictionary& rhs) const;

    /**
     * \see operator==
     */
    bool operator!=(const PdfDictionary& rhs) const;

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
    PdfObject& AddKey(const PdfName& identifier, const PdfObject& rObject);

    PdfObject& AddKeyIndirect(const PdfName& identifier, const PdfObject& rObject);

    /** Get the key's value out of the dictionary.
     *
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key named key in the dictionary
     *
     *  \returns pointer to the found value, or 0 if the key was not found.
     */
    const PdfObject* GetKey(const PdfName& key) const;

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
    PdfObject* GetKey(const PdfName& key);

    /** Get the keys value out of the dictionary
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param key look for the key names pszKey in the dictionary
     *  \returns pointer to the found value or 0 if the key was not found.
     */
    const PdfObject* FindKey(const PdfName& key) const;
    PdfObject* FindKey(const PdfName& key);

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
    const PdfObject* FindKeyParent(const PdfName& key) const;
    PdfObject* FindKeyParent(const PdfName& key);

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
    const PdfObject& MustGetKey(const PdfName& key) const;

    bool GetKeyAsBool(const PdfName& key, bool defvalue = false) const;

    int64_t GetKeyAsNumber(const PdfName& key, int64_t defvalue = 0) const;

    int64_t GetKeyAsNumberLenient(const PdfName& key, int64_t defvalue = 0) const;

    double GetKeyAsReal(const PdfName& key, double defvalue = 0.0) const;

    double GetKeyAsRealStrict(const PdfName& key, double defvalue = 0.0) const;

    PdfName GetKeyAsName(const PdfName& key, const PdfName& defvalue = PdfName::KeyNull) const;

    PdfString GetKeyAsString(const PdfName& key, const PdfString& defvalue = { }) const;

    PdfReference GetKeyAsReference(const PdfName& key, const PdfReference& defvalue = { }) const;

    /** Allows to check if a dictionary contains a certain key.
     * \param key look for the key named key.Name() in the dictionary
     *
     *  \returns true if the key is part of the dictionary, otherwise false.
     */
    bool  HasKey(const PdfName& key) const;

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
    bool RemoveKey(const PdfName& identifier);

    void Write(PdfOutputDevice& pDevice, PdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt) const override;

    /**
    *  \returns the size of the internal map
    */
    size_t GetSize() const;

public:
    TIKeyMap begin();
    TIKeyMap end();
    TCIKeyMap begin() const;
    TCIKeyMap end() const;
    size_t size() const;

protected:
    void ResetDirtyInternal() override;
    void SetOwner(PdfObject* pOwner) override;

private:
    std::pair<TKeyMap::iterator, bool> addKey(const PdfName& identifier, const PdfObject& rObject, bool noDirtySet);
    PdfObject* getKey(const PdfName& key) const;
    PdfObject* findKey(const PdfName& key) const;
    PdfObject* findKeyParent(const PdfName& key) const;

private:
    TKeyMap      m_mapKeys;
};

typedef std::vector<PdfDictionary*>      TVecDictionaries; 
typedef	TVecDictionaries::iterator       TIVecDictionaries; 
typedef	TVecDictionaries::const_iterator TCIVecDictionaries;

}

#endif // PDF_DICTIONARY_H
