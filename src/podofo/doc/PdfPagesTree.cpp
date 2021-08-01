/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfPagesTree.h"

#include <algorithm>
#include <sstream>

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"
#include "base/PdfOutputDevice.h"

#include "PdfPage.h"

#include <iostream>

using namespace std;
using namespace PoDoFo;

PdfPagesTree::PdfPagesTree(PdfDocument& doc)
    : PdfElement(doc, "Pages"),
    m_cache(0)
{
    GetObject().GetDictionary().AddKey("Kids", PdfArray());
    GetObject().GetDictionary().AddKey("Count", PdfObject(static_cast<int64_t>(0)));
}

PdfPagesTree::PdfPagesTree(PdfObject& pagesRoot)
    : PdfElement(pagesRoot),
    m_cache(GetChildCount(pagesRoot)) { }

PdfPagesTree::~PdfPagesTree() 
{
    m_cache.ClearCache();
}

unsigned PdfPagesTree::GetPageCount() const
{
    return GetChildCount(GetObject());
}

PdfPage& PdfPagesTree::GetPage(unsigned index)
{
    if (index >= GetPageCount())
        PODOFO_RAISE_ERROR(EPdfError::PageNotFound);

    return getPage(index);
}

const PdfPage& PdfPagesTree::GetPage(unsigned index) const
{
    if (index >= GetPageCount())
        PODOFO_RAISE_ERROR(EPdfError::PageNotFound);

    return const_cast<PdfPagesTree&>(*this).getPage(index);
}

PdfPage& PdfPagesTree::getPage(unsigned index)
{
    // Take a look into the cache first
    auto page = m_cache.GetPage(index);
    if (page != nullptr)
        return *page;

    // Not in cache -> search tree
    PdfObjectList parents;
    PdfObject* pageObj = this->GetPageNode(index, this->GetRoot(), parents);
    if (pageObj != nullptr)
    {
        page = new PdfPage(*pageObj, parents);
        m_cache.SetPage(index, page);
        return *page;
    }

    PODOFO_RAISE_ERROR(EPdfError::PageNotFound);
}

PdfPage& PdfPagesTree::GetPage(const PdfReference& ref)
{
    return getPage(ref);
}

const PdfPage& PdfPagesTree::GetPage(const PdfReference& ref) const
{
    return const_cast<PdfPagesTree&>(*this).getPage(ref);
}

PdfPage& PdfPagesTree::getPage(const PdfReference& ref)
{
    // We have to search through all pages,
    // as this is the only way
    // to instantiate the PdfPage with a correct list of parents
    for (unsigned i = 0; i < this->GetPageCount(); i++)
    {
        auto& page = this->getPage(i);
        if (page.GetObject().GetIndirectReference() == ref)
            return page;
    }

    PODOFO_RAISE_ERROR(EPdfError::PageNotFound);
}

void PdfPagesTree::InsertPage(unsigned atIndex, PdfObject* pageObj)
{
    vector<PdfObject*> objs = { pageObj };
    InsertPages(atIndex, objs);
}

void PdfPagesTree::InsertPages(unsigned atIndex, const vector<PdfObject*>& pages)
{
    bool insertAfterPivot = false;
    PdfObjectList parents;
    PdfObject* pivotPage = nullptr;
    unsigned pageCount = this->GetPageCount();
    if (pageCount != 0)
    {
        if (atIndex >= pageCount)
        {
            // If atIndex is after the last page, normalize it
            // and select the last page as the pivot
            atIndex = pageCount;
            insertAfterPivot = true;
            pivotPage = this->GetPageNode(pageCount - 1, this->GetRoot(), parents);
        }
        else
        {
            // The pivot page is the page exactly at the given index
            pivotPage = this->GetPageNode(atIndex, this->GetRoot(), parents);
        }
    }

    if (pivotPage == nullptr || parents.size() == 0)
    {
        if (this->GetPageCount() != 0)
        {
            PdfError::LogMessage(LogSeverity::Critical,
                "Cannot find page %i or page %i has no parents. Cannot insert new page.",
                atIndex, atIndex);
            return;
        }
        else
        {
            // We insert the first page into an empty pages tree
            PdfObjectList pagesTree;
            pagesTree.push_back(&this->GetObject());
            // Use -1 as index to insert before the empty kids array
            InsertPagesIntoNode(this->GetObject(), pagesTree, -1, pages);
        }
    }
    else
    {
        PdfObject* parentNode = parents.back();
        int kidsIndex = insertAfterPivot ? this->GetPosInKids(*pivotPage, parentNode) : -1;
        InsertPagesIntoNode(*parentNode, parents, kidsIndex, pages);
    }

    m_cache.InsertPlaceHolders(atIndex, (unsigned)pages.size());
}

PdfPage* PdfPagesTree::CreatePage(const PdfRect& size)
{
    auto page = new PdfPage(*GetRoot().GetDocument(), size);
    InsertPage(this->GetPageCount(), &page->GetObject());
    m_cache.SetPage(this->GetPageCount(), page);
    return page;
}

PdfPage* PdfPagesTree::InsertPage(unsigned atIndex, const PdfRect& size)
{
    PdfPage* page = new PdfPage(*GetRoot().GetDocument(), size);
    unsigned pageCount = this->GetPageCount();
    if (atIndex > pageCount)
        atIndex = pageCount;

    InsertPage(atIndex, &page->GetObject());
    m_cache.SetPage(atIndex, page);
    return page;
}

void PdfPagesTree::CreatePages(const vector<PdfRect>& sizes)
{
    vector<PdfPage*> vecPages;
    vector<PdfObject*> vecObjects;
    for (auto &rect : sizes)
    {
        PdfPage* pPage = new PdfPage(*GetRoot().GetDocument(), rect);
        vecPages.push_back(pPage);
        vecObjects.push_back(&pPage->GetObject());
    }

    InsertPages(this->GetPageCount(), vecObjects);
    m_cache.SetPages(this->GetPageCount(), vecPages);
}

void PdfPagesTree::DeletePage(unsigned atIndex)
{
    // Delete from cache
    m_cache.DeletePage(atIndex);

    // Delete from pages tree
    PdfObjectList parents;
    PdfObject* pageNode = this->GetPageNode(atIndex, this->GetRoot(), parents);

    if (pageNode == nullptr)
    {
        PdfError::LogMessage(LogSeverity::Information,
            "Invalid argument to PdfPagesTree::DeletePage: %i - Page not found",
            atIndex);
        PODOFO_RAISE_ERROR(EPdfError::PageNotFound);
    }

    if (parents.size() > 0)
    {
        PdfObject* parent = parents.back();
        unsigned kidsIndex = (unsigned)this->GetPosInKids(*pageNode, parent);
        DeletePageFromNode(*parent, parents, kidsIndex, *pageNode);
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Error,
            "PdfPagesTree::DeletePage: Page %i has no parent - cannot be deleted.",
            atIndex);
        PODOFO_RAISE_ERROR(EPdfError::PageNotFound);
    }
}

PdfObject* PdfPagesTree::GetPageNode(unsigned index, PdfObject& parent,
    PdfObjectList& parents)
{
    if (!parent.GetDictionary().HasKey("Kids"))
        PODOFO_RAISE_ERROR(EPdfError::InvalidKey);

    const PdfObject* kidsObj = parent.GetIndirectKey("Kids");
    if (kidsObj == nullptr || !kidsObj->IsArray())
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    const PdfArray& kidsArray = kidsObj->GetArray();
    const size_t numKids = GetChildCount(parent);

    if (index > numKids)
    {
        PdfError::LogMessage(LogSeverity::Critical,
            "Cannot retrieve page %u from a document with only %u pages.",
            index, numKids);
        return nullptr;
    }

    // We have to traverse the tree
    //
    // BEWARE: There is no valid shortcut for tree traversal.
    // Even if eKidsArray.size()==numKids, this does not imply that
    // eKidsArray can be accessed with the index of the page directly.
    // The tree could have an arbitrary complex structure because
    // internal nodes with no leaves (page objects) are not forbidden
    // by the PDF spec.
    for (auto& child : kidsArray)
    {
        if (!child.IsReference())
        {
            PdfError::LogMessage(LogSeverity::Critical, "Requesting page index %i. Invalid datatype in kids array: %s",
                index, child.GetDataTypeString());
            return nullptr;
        }

        PdfObject* childObj = GetRoot().GetDocument()->GetObjects().GetObject(child.GetReference());
        if (childObj == nullptr)
        {
            PdfError::LogMessage(LogSeverity::Critical, "Requesting page index %i. Child not found: %s",
                index, child.GetReference().ToString().c_str());
            return nullptr;
        }

        if (this->IsTypePages(*childObj))
        {
            unsigned childCount = GetChildCount(*childObj);
            if (childCount < index + 1) // Pages are 0 based
            {
                // skip this page tree node
                // and go to the next child in rKidsArray
                index -= childCount;
            }
            else
            {
                // page is in the subtree of pChild
                // => call GetPageNode() recursively

                parents.push_back(&parent);

                if (std::find(parents.begin(), parents.end(), childObj)
                    != parents.end()) // cycle in parent list detected, fend
                { // off security vulnerability similar to CVE-2017-8054 (infinite recursion)
                    ostringstream oss;
                    oss << "Cycle in page tree: child in /Kids array of object "
                        << (*(parents.rbegin()))->GetIndirectReference().ToString()
                        << " back-references to object " << childObj->GetIndirectReference()
                        .ToString() << " one of whose descendants the former is.";
                    PODOFO_RAISE_ERROR_INFO(EPdfError::PageNotFound, oss.str());
                }

                return this->GetPageNode(index, *childObj, parents);
            }
        }
        else if (this->IsTypePage(*childObj))
        {
            if (index == 0)
            {
                // page found
                parents.push_back(&parent);
                return childObj;
            }

            // Skip a normal page
            if (index > 0)
                index--;
        }
        else
        {
            const PdfReference& rLogRef = childObj->GetIndirectReference();
            uint32_t nLogObjNum = rLogRef.ObjectNumber();
            uint16_t nLogGenNum = rLogRef.GenerationNumber();
            PdfError::LogMessage(LogSeverity::Critical,
                "Requesting page index %u. "
                "Invalid datatype referenced in kids array: %s\n"
                "Reference to invalid object: %u %u R", index,
                childObj->GetDataTypeString(), nLogObjNum, nLogGenNum);
            return nullptr;
        }
    }

    return nullptr;
}

bool PdfPagesTree::IsTypePage(const PdfObject& obj) const
{
    if (obj.GetDictionary().FindKeyAs<PdfName>("Type", PdfName()) == "Page")
        return true;

    return false;
}

bool PdfPagesTree::IsTypePages(const PdfObject& obj) const
{
    if (obj.GetDictionary().FindKeyAs<PdfName>("Type", PdfName()) == "Pages")
        return true;

    return false;
}

unsigned PdfPagesTree::GetChildCount(const PdfObject& nodeObj) const
{
    auto countObj = nodeObj.GetDictionary().FindKey("Count");
    if (countObj == nullptr)
        return 0;
    else
        return (unsigned)countObj->GetNumber();
}

int PdfPagesTree::GetPosInKids(PdfObject& pageObj, PdfObject* pageParent)
{
    if (pageParent == nullptr)
        return -1;

    const PdfArray& rKids = pageParent->GetDictionary().GetKey("Kids")->GetArray();

    unsigned index = 0;
    for (auto& child : rKids)
    {
        if (child.GetReference() == pageObj.GetIndirectReference())
            return index;

        index++;
    }

    return -1;
}

void PdfPagesTree::InsertPagesIntoNode(PdfObject& pParent, const PdfObjectList& parents,
    int index, const vector<PdfObject*>& pages)
{
    if (pages.size() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    // 1. Add the reference of the new page to the kids array of pParent
    // 2. Increase count of every node in lstParents (which also includes pParent)
    // 3. Add Parent key to the page

    // 1. Add reference
    const PdfArray oldKids = pParent.GetDictionary().GetKey("Kids")->GetArray();
    PdfArray newKids;
    newKids.reserve(oldKids.GetSize() + pages.size());

    bool isPushedIn = false;
    int i = 0;
    for (auto& oldKid : oldKids)
    {
        if (!isPushedIn && (index < i))    // Pushing before
        {
            for (vector<PdfObject*>::const_iterator itPages = pages.begin(); itPages != pages.end(); itPages++)
            {
                newKids.push_back((*itPages)->GetIndirectReference());    // Push all new kids at once
            }
            isPushedIn = true;
        }
        newKids.push_back(oldKid);    // Push in the old kids
        i++;
    }

    // If new kids are still not pushed in then they may be appending to the end
    if (!isPushedIn && ((index + 1) == (int)oldKids.size()))
    {
        for (vector<PdfObject*>::const_iterator itPages = pages.begin(); itPages != pages.end(); itPages++)
        {
            newKids.push_back((*itPages)->GetIndirectReference());    // Push all new kids at once
        }
        isPushedIn = true;
    }

    pParent.GetDictionary().AddKey("Kids", newKids);


    // 2. increase count
    for (PdfObjectList::const_reverse_iterator itParents = parents.rbegin(); itParents != parents.rend(); itParents++)
        this->ChangePagesCount(**itParents, (int)pages.size());

    // 3. add parent key to each of the pages
    for (vector<PdfObject*>::const_iterator itPages = pages.begin(); itPages != pages.end(); itPages++)
        (*itPages)->GetDictionary().AddKey("Parent", pParent.GetIndirectReference());
}

void PdfPagesTree::DeletePageFromNode(PdfObject& pParent, const PdfObjectList& rlstParents,
    unsigned index, PdfObject& pPage)
{
    // 1. Delete the reference from the kids array of pParent
    // 2. Decrease count of every node in lstParents (which also includes pParent)
    // 3. Remove empty page nodes

    // TODO: Tell cache to free page object

    // 1. Delete reference
    this->DeletePageNode(pParent, index);

    // 2. Decrease count
    PdfObjectList::const_reverse_iterator itParents = rlstParents.rbegin();
    while (itParents != rlstParents.rend())
    {
        this->ChangePagesCount(**itParents, -1);
        itParents++;
    }

    // 3. Remove empty pages nodes
    itParents = rlstParents.rbegin();
    while (itParents != rlstParents.rend())
    {
        // Never delete root node
        if (IsEmptyPageNode(**itParents) && *itParents != &GetRoot())
        {
            PdfObject* parentOfNode = *(itParents + 1);
            unsigned kidsIndex = (unsigned)this->GetPosInKids(**itParents, parentOfNode);
            DeletePageNode(*parentOfNode, kidsIndex);

            // Delete empty page nodes
            this->GetObject().GetDocument()->GetObjects().RemoveObject((*itParents)->GetIndirectReference());
        }

        itParents++;
    }
}

void PdfPagesTree::DeletePageNode(PdfObject& pParent, unsigned nIndex)
{
    PdfArray kids = pParent.GetDictionary().GetKey("Kids")->GetArray();
    kids.erase(kids.begin() + nIndex);
    pParent.GetDictionary().AddKey("Kids", kids);
}

unsigned PdfPagesTree::ChangePagesCount(PdfObject& pageObj, int delta)
{
    // Increment or decrement inPagesDict's Count by inDelta, and return the new count.
    // Simply return the current count if inDelta is 0.
    int cnt = (int)GetChildCount(pageObj);
    if (delta != 0)
    {
        cnt += delta;
        pageObj.GetDictionary().AddKey("Count", PdfVariant(static_cast<int64_t>(cnt)));
    }

    return cnt;
}

bool PdfPagesTree::IsEmptyPageNode(PdfObject& pPageNode)
{
    unsigned count = GetChildCount(pPageNode);
    bool bKidsEmpty = true;

    auto kids = pPageNode.GetDictionary().FindKey("Kids");
    if (kids != nullptr)
        bKidsEmpty = kids->GetArray().empty();

    return count == 0 || bKidsEmpty;
}
