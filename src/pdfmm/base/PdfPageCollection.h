/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PAGES_TREE_H
#define PDF_PAGES_TREE_H

#include "PdfDeclarations.h"

#include "PdfElement.h"
#include "PdfArray.h"
#include "PdfPageTreeCache.h"

namespace mm {

class PdfObject;
class PdfPage;
class PdfRect;

/** Class for managing the tree of Pages in a PDF document
 *  Don't use this class directly. Use PdfDocument instead.
 *
 *  \see PdfDocument
 */
class PDFMM_API PdfPageCollection final : public PdfDictionaryElement
{
    friend class PdfDocument;
    using PdfObjectList = std::deque<PdfObject*>;

public:
    /** Construct a new PdfPageTree
     */
    PdfPageCollection(PdfDocument& doc);

    /** Construct a PdfPageTree from the root /Pages object
     *  \param pagesRoot pointer to page tree dictionary
     */
    PdfPageCollection(PdfObject& pagesRoot);

    /** Close/down destruct a PdfPageTree
     */
    virtual ~PdfPageCollection();

    /** Return the number of pages in the entire tree
     *  \returns number of pages
     */
    unsigned GetCount() const;

    /** Return a PdfPage for the specified Page index
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param index page index, 0-based
     *  \returns a pointer to the requested page
     */
    PdfPage& GetPage(unsigned index);
    const PdfPage& GetPage(unsigned index) const;

    /** Return a PdfPage for the specified Page reference.
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param ref the reference of the pages object
     *  \returns a pointer to the requested page
     */
    PdfPage& GetPage(const PdfReference& ref);
    const PdfPage& GetPage(const PdfReference& ref) const;

    /** Creates a new page object and inserts it into the internal
     *  page tree.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param size a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* CreatePage(const PdfRect& size);

    /** Creates several new page objects and inserts them into the internal
     *  page tree.
     *  The new pages are owned by the pages tree and will get deleted along
     *  with it!
     *  Note: this function will attach all new pages onto the same page node
     *  which can cause the tree to be unbalanced if
     *
     *  \param sizes a vector of PdfRect specifying the size of each of the pages to create (i.e the /MediaBox key) in PDF units
     */
    void CreatePages(const std::vector<PdfRect>& sizes);

    /** Creates a new page object and inserts it at index atIndex.
     *  The returned page is owned by the pages tree and will get deleted along
     *  with it!
     *
     *  \param size a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param atIndex index where to insert the new page (0-based)
     *  \returns a pointer to a PdfPage object
     */
    PdfPage* InsertPage(unsigned atIndex, const PdfRect& size);

    /**  Delete the specified page object from the internal pages tree.
     *   It does NOT remove any PdfObjects from memory - just the reference from the tree
     *
     *   \param atIndex the page number (0-based) to be removed
     *
     *   The PdfPage object refering to this page will be deleted by this call!
     *   Empty page nodes will also be deleted.
     *
     *   \see PdfMemDocument::DeletePages
     */
    void DeletePage(unsigned atIndex);

private:
    PdfPage& getPage(unsigned index);
    PdfPage& getPage(const PdfReference& ref);

    /**
     * Insert page at the given index
     * \remarks Can be used by PdfDocument
     */
    void InsertPage(unsigned atIndex, PdfObject* pageObj);

    /**
     * Insert pages at the given index
     * \remarks Can be used by PdfDocument
     */
    void InsertPages(unsigned atIndex, const std::vector<PdfObject*>& pages);

     PdfObject* GetPageNode(unsigned index, PdfObject& parent, PdfObjectList& parents);

     unsigned GetChildCount(const PdfObject& nodeObj) const;

     /**
      * Test if a PdfObject is a page node
      * \return true if PdfObject is a page node
      */
     bool IsTypePage(const PdfObject& obj) const;

     /**
      * Test if a PdfObject is a pages node
      * \return true if PdfObject is a pages node
      */
     bool IsTypePages(const PdfObject& obj) const;

     /**
      * Find the position of pageObj in the kids array of pageParent
      *
      * \returns the index in the kids array or -1 if pageObj is no child of pageParent
      */
     int GetPosInKids(PdfObject& pageObj, PdfObject* pageParent);

     /** Private method for adjusting the page count in a tree
      */
     unsigned ChangePagesCount(PdfObject& pageObj, int delta);

     /**
     * Insert a vector of page objects into a pages node
     * Same as InsertPageIntoNode except that it allows for adding multiple pages at one time
     * Note that adding many pages onto the same node will create an unbalanced page tree
     *
     * \param node the pages node whete page is to be inserted
     * \param parents list of all (future) parent pages nodes in the pages tree
     *                   of page
     * \param index index where page is to be inserted in node's kids array
     * \param pages a vector of the page objects which are to be inserted
     */
    void InsertPagesIntoNode(PdfObject& node, const PdfObjectList& parents,
        int index, const std::vector<PdfObject*>& pages);

    /**
     * Delete a page object from a pages node
     *
     * \param node which is the direct parent of page and where the page must be deleted
     * \param parents list of all parent pages nodes in the pages tree
     *                   of page
     * \param index index where page is to be deleted in node's kids array
     * \param page the page object which is to be deleted
     */
    void DeletePageFromNode(PdfObject& node, const PdfObjectList& parents,
        unsigned index, PdfObject& page);

    /**
     * Delete a single page node or page object from the kids array of parent
     *
     * \param parent the parent of the page node which is deleted
     * \param index index to remove from the kids array of parent
     */
    void DeletePageNode(PdfObject& parent, unsigned index);

    /**
     * Tests if a page node is emtpy
     *
     * \returns true if Count of page is 0 or the Kids array is empty
     */
    bool IsEmptyPageNode(PdfObject& pageNode);

    /** Private method to access the Root of the tree using a logical name
     */
    inline PdfObject& GetRoot() { return this->GetObject(); }
    inline const PdfObject& GetRoot() const { return this->GetObject(); }

private:
      PdfPageTreeCache m_cache;
};

};

#endif // PDF_PAGES_TREE_H
