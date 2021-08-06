/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfPagesTreeCache.h"

#include "PdfPage.h"
#include "PdfPagesTree.h"

using namespace std;
using namespace PoDoFo;

PdfPagesTreeCache::PdfPagesTreeCache(unsigned initialSize)
{
    m_PageObjs.resize(initialSize);
}

PdfPage* PdfPagesTreeCache::GetPage(unsigned atIndex)
{
    if (atIndex >= m_PageObjs.size())
        return nullptr;

    return m_PageObjs[atIndex];
}

void PdfPagesTreeCache::SetPage(unsigned atIndex, PdfPage* page)
{
    // Delete an old page if it is at the same position
    PdfPage* oldPage = GetPage(atIndex);
    delete oldPage;

    if (atIndex >= m_PageObjs.size())
        m_PageObjs.resize(atIndex + 1);

    m_PageObjs[atIndex] = page;
}

void PdfPagesTreeCache::SetPages(unsigned atIndex, const vector<PdfPage*>& pages)
{
    if ((atIndex + pages.size()) >= m_PageObjs.size())
        m_PageObjs.resize(atIndex + pages.size() + 1);

    for (unsigned i = 0; i < pages.size(); i++)
    {
        // Delete any old pages if it is at the same position
        PdfPage* pOldPage = GetPage(atIndex + i);
        delete pOldPage;

        // Assign the new page
        m_PageObjs[atIndex + i] = pages.at(i);
    }
}

void PdfPagesTreeCache::InsertPlaceHolder(unsigned atIndex)
{
    m_PageObjs.insert(m_PageObjs.begin() + atIndex, static_cast<PdfPage*>(nullptr));
}

void PdfPagesTreeCache::InsertPlaceHolders(unsigned atIndex, unsigned count)
{
    for (unsigned i = 0; i < count; i++)
        m_PageObjs.insert(m_PageObjs.begin() + atIndex + i, static_cast<PdfPage*>(nullptr));
}

void PdfPagesTreeCache::DeletePage(unsigned atIndex)
{
    if (atIndex >= m_PageObjs.size())
        return;

    delete m_PageObjs[atIndex];
    m_PageObjs.erase(m_PageObjs.begin() + atIndex);
}

void PdfPagesTreeCache::ClearCache()
{
    for (auto page : m_PageObjs)
        delete page;

    m_PageObjs.clear();
}
