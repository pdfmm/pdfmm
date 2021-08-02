/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_NAMES_TREE_H
#define PDF_NAMES_TREE_H

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfDictionary;
class PdfName;
class PdfObject;
class PdfString;
class PdfVecObjects;

enum class EPdfNameLimits
{
    Before,
    Inside,
    After
};

class PODOFO_DOC_API PdfNamesTree final : public PdfElement
{
public:
    /** Create a new PdfNamesTree object
     *  \param pParent parent of this action
     */
    PdfNamesTree(PdfDocument& doc);

    /** Create a PdfNamesTree object from an existing PdfObject
     *	\param pObject the object to create from
     *  \param pCatalog the Catalog dictionary of the owning PDF
     */
    PdfNamesTree(PdfObject& obj, PdfObject* pCatalog);

    /** Insert a key and value in one of the dictionaries of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to insert. If it exists, it will be overwritten.
     *  \param rValue the value to insert.
     */
    void AddValue(const PdfName& tree, const PdfString& key, const PdfObject& rValue);

    /** Get the object referenced by a string key in one of the dictionaries
     *  of the name tree.
     *  \param tree name of the tree to search for the key.
     *  \param key the key to search for
     *  \returns the value of the key or nullptr if the key was not found.
     *           if the value is a reference, the object referenced by
     *           this reference is returned.
     */
    PdfObject* GetValue(const PdfName& tree, const PdfString& key) const;

    /** Tests whether a certain nametree has a value.
     *
     *  It is generally faster to use GetValue and check for nullptr
     *  as return value.
     *
     *  \param tree name of the tree to search for the key.
     *  \param key name of the key to look for
     *  \returns true if the dictionary has such a key.
     */
    bool HasValue(const PdfName& tree, const PdfString& key) const;

    /** Tests whether a key is in the range of a limits entry of a name tree node
     *  \returns EPdfNameLimits::Inside if the key is inside of the range
     *  \returns EPdfNameLimits::After if the key is greater than the specified range
     *  \returns EPdfNameLimits::Before if the key is smalelr than the specified range
     *
     *  Internal use only.
     */
    static EPdfNameLimits CheckLimits(const PdfObject* pObj, const PdfString& key);

    /**
     * Adds all keys and values from a name tree to a dictionary.
     * Removes all keys that have been previously in the dictionary.
     *
     * \param tree the name of the tree to convert into a dictionary
     * \param rDict add all keys and values to this dictionary
     */
    void ToDictionary(const PdfName& dictionary, PdfDictionary& rDict);

    /** Peter Petrov: 23 May 2008
     * I have made it for access to "JavaScript" dictonary. This is "document-level javascript storage"
     *  \param bCreate if true the javascript node is created if it does not exists.
     */
    PdfObject* GetJavaScriptNode(bool bCreate = false) const;

    /** Peter Petrov: 6 June 2008
     * I have made it for access to "Dest" dictionary. This is "document-level named destination storage"
     *  \param bCreate if true the dests node is created if it does not exists.
     */
    PdfObject* GetDestsNode(bool bCreate = false) const;

private:
    /** Get a PdfNameTrees root node for a certain name.
     *  \param name that identifies a specific name tree.
     *         Valid names are:
     *            - Dests
     *            - AP
     *            - JavaScript
     *            - Pages
     *            - Templates
     *            - IDS
     *            - URLS
     *            - EmbeddedFiles
     *            - AlternatePresentations
     *            - Renditions
     *
     *  \param bCreate if true the root node is created if it does not exists.
     *  \returns the root node of the tree or nullptr if it does not exists
     */
    PdfObject* GetRootNode(const PdfName& name, bool bCreate = false) const;

    /** Recursively walk through the name tree and find the value for key.
     *  \param pObj the name tree
     *  \param key the key to find a value for
     *  \return the value for the key or nullptr if it was not found
     */
    PdfObject* GetKeyValue(PdfObject* pObj, const PdfString& key) const;

    /**
     *  Add all keys and values from an object and its children to a dictionary.
     *  \param pObj a pdf name tree node
     *  \param rDict a dictionary
     */
    void AddToDictionary(PdfObject* pObj, PdfDictionary& rDict);

private:
    PdfObject* m_pCatalog;
};

};

#endif // PDF_NAMES_TREE_H
