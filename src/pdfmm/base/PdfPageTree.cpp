/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfPageTree.h"

#include <algorithm>
#include <iostream>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfPage.h"

using namespace std;
using namespace mm;

PdfPageTree::PdfPageTree(PdfDocument& doc)
    : PdfDictionaryElement(doc, "Pages"),
    m_cache(0)
{
    GetObject().GetDictionary().AddKey("Kids", PdfArray());
    GetObject().GetDictionary().AddKey("Count", PdfObject(static_cast<int64_t>(0)));
}

PdfPageTree::PdfPageTree(PdfObject& pagesRoot)
    : PdfDictionaryElement(pagesRoot),
    m_cache(GetChildCount(pagesRoot)) { }

PdfPageTree::~PdfPageTree()
{
    m_cache.ClearCache();
}

unsigned PdfPageTree::GetPageCount() const
{
    return GetChildCount(GetObject());
}

PdfPage& PdfPageTree::GetPage(unsigned index)
{
    if (index >= GetPageCount())
        PDFMM_RAISE_ERROR(PdfErrorCode::PageNotFound);

    return getPage(index);
}

const PdfPage& PdfPageTree::GetPage(unsigned index) const
{
    if (index >= GetPageCount())
        PDFMM_RAISE_ERROR(PdfErrorCode::PageNotFound);

    return const_cast<PdfPageTree&>(*this).getPage(index);
}

PdfPage& PdfPageTree::getPage(unsigned index)
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

    PDFMM_RAISE_ERROR(PdfErrorCode::PageNotFound);
}

PdfPage& PdfPageTree::GetPage(const PdfReference& ref)
{
    return getPage(ref);
}

const PdfPage& PdfPageTree::GetPage(const PdfReference& ref) const
{
    return const_cast<PdfPageTree&>(*this).getPage(ref);
}

PdfPage& PdfPageTree::getPage(const PdfReference& ref)
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

    PDFMM_RAISE_ERROR(PdfErrorCode::PageNotFound);
}

void PdfPageTree::InsertPage(unsigned atIndex, PdfObject* pageObj)
{
    vector<PdfObject*> objs = { pageObj };
    InsertPages(atIndex, objs);
}

void PdfPageTree::InsertPages(unsigned atIndex, const vector<PdfObject*>& pages)
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
            PdfError::LogMessage(PdfLogSeverity::Error,
                "Cannot find page {} or page {} has no parents. Cannot insert new page",
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

PdfPage* PdfPageTree::CreatePage(const PdfRect& size)
{
    auto page = new PdfPage(*GetRoot().GetDocument(), size);
    InsertPage(this->GetPageCount(), &page->GetObject());
    m_cache.SetPage(this->GetPageCount(), page);
    return page;
}

PdfPage* PdfPageTree::InsertPage(unsigned atIndex, const PdfRect& size)
{
    auto page = new PdfPage(*GetRoot().GetDocument(), size);
    unsigned pageCount = this->GetPageCount();
    if (atIndex > pageCount)
        atIndex = pageCount;

    InsertPage(atIndex, &page->GetObject());
    m_cache.SetPage(atIndex, page);
    return page;
}

void PdfPageTree::CreatePages(const vector<PdfRect>& sizes)
{
    vector<PdfPage*> pages;
    vector<PdfObject*> objects;
    for (auto& rect : sizes)
    {
        auto page = new PdfPage(*GetRoot().GetDocument(), rect);
        pages.push_back(page);
        objects.push_back(&page->GetObject());
    }

    InsertPages(this->GetPageCount(), objects);
    m_cache.SetPages(this->GetPageCount(), pages);
}

void PdfPageTree::DeletePage(unsigned atIndex)
{
    // Delete from cache
    m_cache.DeletePage(atIndex);

    // Delete from pages tree
    PdfObjectList parents;
    auto pageNode = this->GetPageNode(atIndex, this->GetRoot(), parents);
    if (pageNode == nullptr)
    {
        PdfError::LogMessage(PdfLogSeverity::Information,
            "Invalid argument to PdfPageTree::DeletePage: {} - Page not found",
            atIndex);
        PDFMM_RAISE_ERROR(PdfErrorCode::PageNotFound);
    }

    if (parents.size() > 0)
    {
        PdfObject* parent = parents.back();
        unsigned kidsIndex = (unsigned)this->GetPosInKids(*pageNode, parent);
        DeletePageFromNode(*parent, parents, kidsIndex, *pageNode);
    }
    else
    {
        PdfError::LogMessage(PdfLogSeverity::Error,
            "PdfPageTree::DeletePage: Page {} has no parent - cannot be deleted",
            atIndex);
        PDFMM_RAISE_ERROR(PdfErrorCode::PageNotFound);
    }
}

PdfObject* PdfPageTree::GetPageNode(unsigned index, PdfObject& parent,
    PdfObjectList& parents)
{
    if (!parent.GetDictionary().HasKey("Kids"))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidKey);

    auto kidsObj = parent.GetDictionary().FindKey("Kids");
    if (kidsObj == nullptr || !kidsObj->IsArray())
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    const PdfArray& kidsArray = kidsObj->GetArray();
    const size_t numKids = GetChildCount(parent);

    if (index > numKids)
    {
        PdfError::LogMessage(PdfLogSeverity::Error,
            "Cannot retrieve page {} from a document with only {} pages",
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
            PdfError::LogMessage(PdfLogSeverity::Error, "Requesting page index {}. Invalid datatype in kids array: {}",
                index, child.GetDataTypeString());
            return nullptr;
        }

        PdfObject* childObj = GetRoot().GetDocument()->GetObjects().GetObject(child.GetReference());
        if (childObj == nullptr)
        {
            PdfError::LogMessage(PdfLogSeverity::Error, "Requesting page index {}. Child not found: {}",
                index, child.GetReference().ToString());
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
                // page is in the subtree of child
                // => call GetPageNode() recursively

                parents.push_back(&parent);

                if (std::find(parents.begin(), parents.end(), childObj)
                    != parents.end()) // cycle in parent list detected, fend
                { // off security vulnerability similar to CVE-2017-8054 (infinite recursion)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::PageNotFound,
                        "Cycle in page tree: child in /Kids array of object {} back-references "
                        "to object {} one of whose descendants the former is",
                        (*(parents.rbegin()))->GetIndirectReference().ToString(),
                        childObj->GetIndirectReference().ToString());
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
            PdfError::LogMessage(PdfLogSeverity::Error,
                "Requesting page index {}. "
                "Invalid datatype referenced in kids array: {}. "
                "Reference to invalid object: {} {} R", index,
                childObj->GetDataTypeString(), nLogObjNum, nLogGenNum);
            return nullptr;
        }
    }

    return nullptr;
}

bool PdfPageTree::IsTypePage(const PdfObject& obj) const
{
    if (obj.GetDictionary().FindKeyAs<PdfName>("Type", PdfName()) == "Page")
        return true;

    return false;
}

bool PdfPageTree::IsTypePages(const PdfObject& obj) const
{
    if (obj.GetDictionary().FindKeyAs<PdfName>("Type", PdfName()) == "Pages")
        return true;

    return false;
}

unsigned PdfPageTree::GetChildCount(const PdfObject& nodeObj) const
{
    auto countObj = nodeObj.GetDictionary().FindKey("Count");
    if (countObj == nullptr)
        return 0;
    else
        return (unsigned)countObj->GetNumber();
}

int PdfPageTree::GetPosInKids(PdfObject& pageObj, PdfObject* pageParent)
{
    if (pageParent == nullptr)
        return -1;

    const PdfArray& kids = pageParent->GetDictionary().MustFindKey("Kids").GetArray();

    unsigned index = 0;
    for (auto& child : kids)
    {
        if (child.GetReference() == pageObj.GetIndirectReference())
            return index;

        index++;
    }

    return -1;
}

void PdfPageTree::InsertPagesIntoNode(PdfObject& parent, const PdfObjectList& parents,
    int index, const vector<PdfObject*>& pages)
{
    if (pages.size() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    // 1. Add the reference of the new page to the kids array of parent
    // 2. Increase count of every node in parents (which also includes parent)
    // 3. Add Parent key to the page

    // 1. Add reference
    const PdfArray oldKids = parent.GetDictionary().MustFindKey("Kids").GetArray();
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

    parent.GetDictionary().AddKey("Kids", newKids);


    // 2. increase count
    for (PdfObjectList::const_reverse_iterator itParents = parents.rbegin(); itParents != parents.rend(); itParents++)
        this->ChangePagesCount(**itParents, (int)pages.size());

    // 3. add parent key to each of the pages
    for (vector<PdfObject*>::const_iterator itPages = pages.begin(); itPages != pages.end(); itPages++)
        (*itPages)->GetDictionary().AddKey("Parent", parent.GetIndirectReference());
}

void PdfPageTree::DeletePageFromNode(PdfObject& parent, const PdfObjectList& parents,
    unsigned index, PdfObject& page)
{
    (void)page;

    // 1. Delete the reference from the kids array of parent
    // 2. Decrease count of every node in parents (which also includes parent)
    // 3. Remove empty page nodes

    // TODO: Tell cache to free page object

    // 1. Delete reference
    this->DeletePageNode(parent, index);

    // 2. Decrease count
    PdfObjectList::const_reverse_iterator itParents = parents.rbegin();
    while (itParents != parents.rend())
    {
        this->ChangePagesCount(**itParents, -1);
        itParents++;
    }

    // 3. Remove empty pages nodes
    itParents = parents.rbegin();
    while (itParents != parents.rend())
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

void PdfPageTree::DeletePageNode(PdfObject& parent, unsigned index)
{
    auto& kids = parent.GetDictionary().MustFindKey("Kids").GetArray();
    kids.erase(kids.begin() + index);
}

unsigned PdfPageTree::ChangePagesCount(PdfObject& pageObj, int delta)
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

bool PdfPageTree::IsEmptyPageNode(PdfObject& pageNode)
{
    unsigned count = GetChildCount(pageNode);
    bool bKidsEmpty = true;

    auto kids = pageNode.GetDictionary().FindKey("Kids");
    if (kids != nullptr)
        bKidsEmpty = kids->GetArray().empty();

    return count == 0 || bKidsEmpty;
}
