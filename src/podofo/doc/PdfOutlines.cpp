/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfOutlines.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"

#include "PdfAction.h"
#include "PdfDestination.h"

using namespace std;
using namespace PoDoFo;

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc, const PdfString& sTitle, const shared_ptr<PdfDestination>& dest,
    PdfOutlineItem* pParentOutline) :
    PdfElement(doc),
    m_pParentOutline(pParentOutline), m_pPrev(nullptr), m_pNext(nullptr),
    m_pFirst(nullptr), m_pLast(nullptr), m_destination(nullptr), m_action(nullptr)
{
    if (pParentOutline)
        this->GetObject().GetDictionary().AddKey("Parent", pParentOutline->GetObject().GetIndirectReference());

    this->SetTitle(sTitle);
    this->SetDestination(dest);
}

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc, const PdfString& sTitle, const shared_ptr<PdfAction>& action,
    PdfOutlineItem* pParentOutline) :
    PdfElement(doc),
    m_pParentOutline(pParentOutline), m_pPrev(nullptr), m_pNext(nullptr),
    m_pFirst(nullptr), m_pLast(nullptr), m_destination(nullptr), m_action(nullptr)
{
    if (pParentOutline)
        this->GetObject().GetDictionary().AddKey("Parent", pParentOutline->GetObject().GetIndirectReference());

    this->SetTitle(sTitle);
    this->SetAction(action);
}

PdfOutlineItem::PdfOutlineItem(PdfObject& obj, PdfOutlineItem* pParentOutline, PdfOutlineItem* pPrevious)
    : PdfElement(obj), m_pParentOutline(pParentOutline), m_pPrev(pPrevious),
    m_pNext(nullptr), m_pFirst(nullptr), m_pLast(nullptr), m_destination(nullptr), m_action(nullptr)
{
    PdfReference first, next;

    if (this->GetObject().GetDictionary().HasKey("First"))
    {
        first = this->GetObject().GetDictionary().GetKey("First")->GetReference();
        m_pFirst = new PdfOutlineItem(*obj.GetDocument()->GetObjects().GetObject(first), this, nullptr);
    }

    if (this->GetObject().GetDictionary().HasKey("Next"))
    {
        next = this->GetObject().GetDictionary().GetKey("Next")->GetReference();
        PdfObject* pObj = obj.GetDocument()->GetObjects().GetObject(next);

        m_pNext = new PdfOutlineItem(*pObj, pParentOutline, this);
    }
    else
    {
        // if there is no next key,
        // we have to set ourself as the last item of the parent
        if (m_pParentOutline)
            m_pParentOutline->SetLast(this);
    }
}

PdfOutlineItem::PdfOutlineItem(PdfDocument& doc)
    : PdfElement(doc, "Outlines"), m_pParentOutline(nullptr), m_pPrev(nullptr),
    m_pNext(nullptr), m_pFirst(nullptr), m_pLast(nullptr), m_destination(nullptr), m_action(nullptr)
{
}

PdfOutlineItem::~PdfOutlineItem()
{
    delete m_pNext;
    delete m_pFirst;
}

PdfOutlineItem* PdfOutlineItem::CreateChild(const PdfString& sTitle, const shared_ptr<PdfDestination>& dest)
{
    PdfOutlineItem* pItem = new PdfOutlineItem(*this->GetObject().GetDocument(), sTitle, dest, this);
    this->InsertChildInternal(pItem, false);
    return pItem;
}

void PdfOutlineItem::InsertChild(PdfOutlineItem* pItem)
{
    this->InsertChildInternal(pItem, true);
}

void PdfOutlineItem::InsertChildInternal(PdfOutlineItem* pItem, bool bCheckParent)
{
    PdfOutlineItem* pItemToCheckParent = pItem;
    PdfOutlineItem* pRoot = nullptr;
    PdfOutlineItem* pRootOfThis = nullptr;

    if (!pItemToCheckParent)
        return;

    if (bCheckParent)
    {
        while (pItemToCheckParent)
        {
            while (pItemToCheckParent->GetParentOutline())
                pItemToCheckParent = pItemToCheckParent->GetParentOutline();

            if (pItemToCheckParent == pItem) // item can't have a parent
            {
                pRoot = pItem; // needed later, "root" can mean "standalone" here
                break;         // for performance in standalone or doc-merge case
            }

            if (!pRoot)
            {
                pRoot = pItemToCheckParent;
                pItemToCheckParent = this;
            }
            else
            {
                pRootOfThis = pItemToCheckParent;
                pItemToCheckParent = nullptr;
            }
        }

        if (pRoot == pRootOfThis) // later nullptr if check skipped for performance
            PODOFO_RAISE_ERROR(EPdfError::OutlineItemAlreadyPresent);
    }

    if (m_pLast != nullptr)
    {
        m_pLast->SetNext(pItem);
        pItem->SetPrevious(m_pLast);
    }

    m_pLast = pItem;

    if (m_pFirst == nullptr)
        m_pFirst = m_pLast;

    this->GetObject().GetDictionary().AddKey("First", m_pFirst->GetObject().GetIndirectReference());
    this->GetObject().GetDictionary().AddKey("Last", m_pLast->GetObject().GetIndirectReference());
}

PdfOutlineItem* PdfOutlineItem::CreateNext(const PdfString& sTitle, const shared_ptr<PdfDestination>& dest)
{
    PdfOutlineItem* pItem = new PdfOutlineItem(*this->GetObject().GetDocument(), sTitle, dest, m_pParentOutline);

    if (m_pNext != nullptr)
    {
        m_pNext->SetPrevious(pItem);
        pItem->SetNext(m_pNext);
    }

    m_pNext = pItem;
    m_pNext->SetPrevious(this);

    this->GetObject().GetDictionary().AddKey("Next", m_pNext->GetObject().GetIndirectReference());

    if (m_pParentOutline != nullptr && m_pNext->Next() == nullptr)
        m_pParentOutline->SetLast(m_pNext);

    return m_pNext;
}

PdfOutlineItem* PdfOutlineItem::CreateNext(const PdfString& sTitle, const shared_ptr<PdfAction>& action)
{
    PdfOutlineItem* pItem = new PdfOutlineItem(*this->GetObject().GetDocument(), sTitle, action, m_pParentOutline);

    if (m_pNext != nullptr)
    {
        m_pNext->SetPrevious(pItem);
        pItem->SetNext(m_pNext);
    }

    m_pNext = pItem;
    m_pNext->SetPrevious(this);

    this->GetObject().GetDictionary().AddKey("Next", m_pNext->GetObject().GetIndirectReference());

    if (m_pParentOutline != nullptr && m_pNext->Next() == nullptr)
        m_pParentOutline->SetLast(m_pNext);

    return m_pNext;
}

void PdfOutlineItem::SetPrevious(PdfOutlineItem* pItem)
{
    m_pPrev = pItem;
    if (m_pPrev == nullptr)
        this->GetObject().GetDictionary().RemoveKey("Prev");
    else
        this->GetObject().GetDictionary().AddKey("Prev", m_pPrev->GetObject().GetIndirectReference());
}

void PdfOutlineItem::SetNext(PdfOutlineItem* pItem)
{
    m_pNext = pItem;
    if (m_pNext == nullptr)
        this->GetObject().GetDictionary().RemoveKey("Next");
    else
        this->GetObject().GetDictionary().AddKey("Next", m_pNext->GetObject().GetIndirectReference());
}

void PdfOutlineItem::SetLast(PdfOutlineItem* pItem)
{
    m_pLast = pItem;
    if (m_pLast == nullptr)
        this->GetObject().GetDictionary().RemoveKey("Last");

    else
        this->GetObject().GetDictionary().AddKey("Last", m_pLast->GetObject().GetIndirectReference());
}

void PdfOutlineItem::SetFirst(PdfOutlineItem* pItem)
{
    m_pFirst = pItem;
    if (m_pFirst == nullptr)
        this->GetObject().GetDictionary().RemoveKey("First");
    else
        this->GetObject().GetDictionary().AddKey("First", m_pFirst->GetObject().GetIndirectReference());
}

void PdfOutlineItem::Erase()
{
    while (m_pFirst != nullptr)
    {
        // erase will set a new first
        // if it has a next item
        m_pFirst->Erase();
    }

    if (m_pPrev != nullptr)
    {
        m_pPrev->SetNext(m_pNext);
    }

    if (m_pNext != nullptr)
    {
        m_pNext->SetPrevious(m_pPrev);
    }

    if (m_pPrev == nullptr && m_pParentOutline && this == m_pParentOutline->First())
        m_pParentOutline->SetFirst(m_pNext);

    if (m_pNext == nullptr && m_pParentOutline && this == m_pParentOutline->Last())
        m_pParentOutline->SetLast(m_pPrev);

    m_pNext = nullptr;
    delete this;
}

void PdfOutlineItem::SetDestination(const shared_ptr<PdfDestination>& dest)
{
    dest->AddToDictionary(this->GetObject().GetDictionary());
    m_destination = dest;
}

shared_ptr<PdfDestination> PdfOutlineItem::GetDestination() const
{
    return const_cast<PdfOutlineItem&>(*this).getDestination();
}

shared_ptr<PdfDestination> PdfOutlineItem::getDestination()
{
    if (m_destination == nullptr)
    {
        PdfObject* obj = this->GetObject().GetIndirectKey("Dest");
        if (obj == nullptr)
            return nullptr;

        m_destination = std::make_shared<PdfDestination>(*obj);
    }

    return m_destination;
}

void PdfOutlineItem::SetAction(const shared_ptr<PdfAction>& action)
{
    action->AddToDictionary(this->GetObject().GetDictionary());
    m_action = action;
}

shared_ptr<PdfAction> PdfOutlineItem::GetAction() const
{
    return const_cast<PdfOutlineItem&>(*this).getAction();
}

shared_ptr<PdfAction> PdfOutlineItem::getAction()
{
    if (m_action == nullptr)
    {
        PdfObject* obj = this->GetObject().GetIndirectKey("A");
        if (obj == nullptr)
            return nullptr;

        m_action.reset(new PdfAction(*obj));
    }

    return m_action;
}

void PdfOutlineItem::SetTitle(const PdfString& sTitle)
{
    this->GetObject().GetDictionary().AddKey("Title", sTitle);
}

const PdfString& PdfOutlineItem::GetTitle() const
{
    return this->GetObject().GetIndirectKey("Title")->GetString();
}

void PdfOutlineItem::SetTextFormat(PdfOutlineFormat eFormat)
{
    this->GetObject().GetDictionary().AddKey("F", static_cast<int64_t>(eFormat));
}

PdfOutlineFormat PdfOutlineItem::GetTextFormat() const
{
    if (this->GetObject().GetDictionary().HasKey("F"))
        return static_cast<PdfOutlineFormat>(this->GetObject().GetIndirectKey("F")->GetNumber());

    return PdfOutlineFormat::Default;
}

void PdfOutlineItem::SetTextColor(double r, double g, double b)
{
    PdfArray color;
    color.push_back(r);
    color.push_back(g);
    color.push_back(b);

    this->GetObject().GetDictionary().AddKey("C", color);
}


double PdfOutlineItem::GetTextColorRed() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return this->GetObject().GetIndirectKey("C")->GetArray()[0].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorGreen() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return this->GetObject().GetIndirectKey("C")->GetArray()[1].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorBlue() const
{
    if (this->GetObject().GetDictionary().HasKey("C"))
        return this->GetObject().GetIndirectKey("C")->GetArray()[2].GetReal();

    return 0.0;
}

PdfOutlines::PdfOutlines(PdfDocument& doc)
    : PdfOutlineItem(doc)
{
}

PdfOutlines::PdfOutlines(PdfObject& obj)
    : PdfOutlineItem(obj, nullptr, nullptr)
{
}

PdfOutlineItem* PdfOutlines::CreateRoot(const PdfString& sTitle)
{
    return this->CreateChild(sTitle, std::make_shared<PdfDestination>(*GetObject().GetDocument()));
}
