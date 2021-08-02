/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfPagesTreeCache.h"

#include "PdfPage.h"
#include "PdfPagesTree.h"

using namespace std;
using namespace PoDoFo;

PdfPagesTreeCache::PdfPagesTreeCache(unsigned initialSize)
{
    m_deqPageObjs.resize(initialSize);
}

PdfPage* PdfPagesTreeCache::GetPage(unsigned atIndex)
{
    if (atIndex >= m_deqPageObjs.size())
        return nullptr;

    return m_deqPageObjs[atIndex];
}

void PdfPagesTreeCache::SetPage(unsigned atIndex, PdfPage* page)
{
    // Delete an old page if it is at the same position
    PdfPage* oldPage = GetPage(atIndex);
    delete oldPage;

    if (atIndex >= m_deqPageObjs.size())
        m_deqPageObjs.resize(atIndex + 1);

    m_deqPageObjs[atIndex] = page;
}

void PdfPagesTreeCache::SetPages(unsigned atIndex, const vector<PdfPage*>& vecPages)
{
    if ((atIndex + vecPages.size()) >= m_deqPageObjs.size())
        m_deqPageObjs.resize(atIndex + vecPages.size() + 1);

    for (unsigned i = 0; i < vecPages.size(); i++)
    {
        // Delete any old pages if it is at the same position
        PdfPage* pOldPage = GetPage(atIndex + i);
        delete pOldPage;

        // Assign the new page
        m_deqPageObjs[atIndex + i] = vecPages.at(i);
    }
}

void PdfPagesTreeCache::InsertPlaceHolder(unsigned atIndex)
{
    m_deqPageObjs.insert(m_deqPageObjs.begin() + atIndex, static_cast<PdfPage*>(nullptr));
}

void PdfPagesTreeCache::InsertPlaceHolders(unsigned atIndex, unsigned count)
{
    for (unsigned i = 0; i < count; i++)
        m_deqPageObjs.insert(m_deqPageObjs.begin() + atIndex + i, static_cast<PdfPage*>(nullptr));
}

void PdfPagesTreeCache::DeletePage(unsigned atIndex)
{
    if (atIndex >= m_deqPageObjs.size())
        return;

    delete m_deqPageObjs[atIndex];
    m_deqPageObjs.erase(m_deqPageObjs.begin() + atIndex);
}

void PdfPagesTreeCache::ClearCache()
{
    for (auto page : m_deqPageObjs)
        delete page;

    m_deqPageObjs.clear();
}
