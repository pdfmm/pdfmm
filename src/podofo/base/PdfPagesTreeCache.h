/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PAGES_TREE_CACHE_H
#define PDF_PAGES_TREE_CACHE_H

#include "PdfDefines.h"

namespace PoDoFo {

class PdfPage;

/**
 *  This class implements a cache infront of a PdfPagesTree
 *
 *  \see PdfCachedPagesTree
 */
class PODOFO_DOC_API PdfPagesTreeCache final
{
    typedef std::deque<PdfPage*> PdfPageList;

public:
    /** Construct a new PdfCachedPagesTree.
     *
     *  \param nInitialSize initial size of the pagestree
     */
    PdfPagesTreeCache(unsigned initialSize);

    /** Return a PdfPage for the specified Page index
     *  The returned page is owned by the pages tree and
     *  deleted along with it.
     *
     *  \param atIndex page index, 0-based
     *  \returns a pointer to the requested page or nullptr if it is not cached
     */
    PdfPage* GetPage(unsigned index);

    /**
     * Set a PdfPage object in the cache at the given index.
     * Existing object will be replaced
     *
     * \param atIndex index of the page
     * \param page page object
     */
    void SetPage(unsigned atIndex, PdfPage* page);

    /**
     * Add several PdfPage objects to the cache, replacing any existing at the given index
     * \param atIndex zero based index of where the first page will be placed
     * \param pages vector of the page objects to add
     */
    void SetPages(unsigned atIndex, const std::vector<PdfPage*>& pages);

    /**
     * Insert a page placeholder at the given index
     * therefore the cache has to be updated
     *
     * \param atIndex zero based index of the page we are inserting
     */
    void InsertPlaceHolder(unsigned atIndex);

    /**
     * Insert several page placeholders at the given index
     * therefore the cache has to be updated
     *
     * \param atIndex zero based index of the page we are inserting
     * \param nCount number of pages that were inserted
     */
    void InsertPlaceHolders(unsigned atIndex, unsigned count);

    /**
     * Delete a PdfPage from the cache
     * \param atIndex index of the page
     */
    void DeletePage(unsigned atIndex);

    /**
     * Clear cache, i.e. remove all elements from the
     * cache.
     */
    void ClearCache();

private:
    PdfPageList m_PageObjs;
};

};

#endif // PDF_PAGES_TREE_CACHE_H
