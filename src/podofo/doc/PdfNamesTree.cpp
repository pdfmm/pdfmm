/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfNamesTree.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfOutputDevice.h"

#include <sstream>

using namespace PoDoFo;

#define BALANCE_TREE_MAX 65

/*
#define BALANCE_TREE_MAX 9
*/

class PdfNameTreeNode
{
public:
    PdfNameTreeNode(PdfNameTreeNode* pParent, PdfObject* pObject)
        : m_pParent(pParent), m_pObject(pObject)
    {
        m_bHasKids = m_pObject->GetDictionary().HasKey("Kids");
    }

    bool AddValue(const PdfString& key, const PdfObject& value);

    void SetLimits();

    inline PdfObject* GetObject() { return m_pObject; }

private:
    bool Rebalance();

private:
    PdfNameTreeNode* m_pParent;
    PdfObject* m_pObject;

    bool m_bHasKids;
};

bool PdfNameTreeNode::AddValue(const PdfString& key, const PdfObject& rValue)
{
    if (m_bHasKids)
    {
        const PdfArray& kids = this->GetObject()->GetDictionary().GetKey("Kids")->GetArray();
        auto it = kids.begin();
        PdfObject* pChild = nullptr;
        EPdfNameLimits eLimits = EPdfNameLimits::Before; // RG: TODO Compiler complains that this variable should be initialised

        while (it != kids.end())
        {
            pChild = this->GetObject()->GetDocument()->GetObjects().GetObject((*it).GetReference());
            if (pChild == nullptr)
                PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

            eLimits = PdfNamesTree::CheckLimits(pChild, key);
            if ((eLimits == EPdfNameLimits::Before) ||
                (eLimits == EPdfNameLimits::Inside))
            {
                break;
            }

            it++;
        }

        if (it == kids.end())
        {
            // not added, so add to last child
            pChild = this->GetObject()->GetDocument()->GetObjects().GetObject(kids.back().GetReference());
            if (pChild == nullptr)
                PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

            eLimits = EPdfNameLimits::After;
        }

        PdfNameTreeNode child(this, pChild);
        if (child.AddValue(key, rValue))
        {
            // if a child insert the key in a way that the limits
            // are changed, we have to change our limits as well!
            // our parent has to change his parents too!
            if (eLimits != EPdfNameLimits::Inside)
                this->SetLimits();

            this->Rebalance();
            return true;
        }
        else
            return false;
    }
    else
    {
        bool bRebalance = false;
        PdfArray limits;

        if (this->GetObject()->GetDictionary().HasKey("Names"))
        {
            PdfArray& array = this->GetObject()->GetDictionary().GetKey("Names")->GetArray();
            PdfArray::iterator it = array.begin();

            while (it != array.end())
            {
                if (it->GetString() == key)
                {
                    // no need to write the key as it is anyways the same
                    it++;
                    // write the value
                    *it = rValue;
                    break;
                }
                else if (it->GetString().GetString() > key.GetString())
                {
                    it = array.insert(it, rValue); // array.insert invalidates the iterator
                    it = array.insert(it, key);
                    break;
                }

                it += 2;
            }

            if (it == array.end())
            {
                array.push_back(key);
                array.push_back(rValue);
            }

            limits.push_back(*array.begin());
            limits.push_back(*(array.end() - 2));
            bRebalance = true;
        }
        else
        {
            // we create a completely new node
            PdfArray array;
            array.push_back(key);
            array.push_back(rValue);

            limits.push_back(key);
            limits.push_back(key);

            // create a child object
            PdfObject* pChild = this->GetObject()->GetDocument()->GetObjects().CreateDictionaryObject();
            pChild->GetDictionary().AddKey("Names", array);
            pChild->GetDictionary().AddKey("Limits", limits);

            PdfArray kids(pChild->GetIndirectReference());
            this->GetObject()->GetDictionary().AddKey("Kids", kids);
            m_bHasKids = true;
        }

        if (m_pParent != nullptr)
        {
            // Root node is not allowed to have a limits key!
            this->GetObject()->GetDictionary().AddKey("Limits", limits);
        }

        if (bRebalance)
            this->Rebalance();

        return true;
    }
}

void PdfNameTreeNode::SetLimits()
{
    PdfArray limits;

    if (m_bHasKids)
    {
        if (this->GetObject()->GetDictionary().HasKey(PdfName("Kids")) &&
            this->GetObject()->GetDictionary().GetKey(PdfName("Kids"))->IsArray())
        {
            const PdfReference& rRefFirst = (*this->GetObject()->GetDictionary().GetKey("Kids")->GetArray().begin()).GetReference();
            PdfObject* pChild = this->GetObject()->GetDocument()->GetObjects().GetObject(rRefFirst);
            if (pChild && pChild->GetDictionary().HasKey(PdfName("Limits")) &&
                pChild->GetDictionary().GetKey(PdfName("Limits"))->IsArray())
                limits.push_back(*(pChild->GetDictionary().GetKey("Limits")->GetArray().begin()));

            const PdfReference& rRefLast = this->GetObject()->GetDictionary().GetKey("Kids")->GetArray().back().GetReference();
            pChild = this->GetObject()->GetDocument()->GetObjects().GetObject(rRefLast);
            if (pChild && pChild->GetDictionary().HasKey(PdfName("Limits")) &&
                pChild->GetDictionary().GetKey(PdfName("Limits"))->IsArray())
                limits.push_back(pChild->GetDictionary().GetKey("Limits")->GetArray().back());
        }
        else
        {
            PdfError::LogMessage(LogSeverity::Error,
                "Object %i %si does not have Kids array.",
                this->GetObject()->GetIndirectReference().ObjectNumber(),
                this->GetObject()->GetIndirectReference().GenerationNumber());
        }
    }
    else // has "Names"
    {
        if (this->GetObject()->GetDictionary().HasKey(PdfName("Names")) &&
            this->GetObject()->GetDictionary().GetKey(PdfName("Names"))->IsArray())
        {
            limits.push_back((*this->GetObject()->GetDictionary().GetKey("Names")->GetArray().begin()));
            limits.push_back((*(this->GetObject()->GetDictionary().GetKey("Names")->GetArray().end() - 2)));
        }
        else
            PdfError::LogMessage(LogSeverity::Error,
                "Object %i %si does not have Names array.",
                this->GetObject()->GetIndirectReference().ObjectNumber(),
                this->GetObject()->GetIndirectReference().GenerationNumber());
    }

    if (m_pParent != nullptr)
    {
        // Root node is not allowed to have a limits key!
        this->GetObject()->GetDictionary().AddKey("Limits", limits);
    }
}

bool PdfNameTreeNode::Rebalance()
{
    PdfArray* pArray = m_bHasKids ? &(this->GetObject()->GetDictionary().GetKey("Kids")->GetArray()) :
        &(this->GetObject()->GetDictionary().GetKey("Names")->GetArray());
    const PdfName& key = m_bHasKids ? PdfName("Kids") : PdfName("Names");
    const unsigned nLength = m_bHasKids ? BALANCE_TREE_MAX : BALANCE_TREE_MAX * 2;

    if (pArray == nullptr)
        return false;

    if (pArray->size() > nLength)
    {
        PdfArray   first;
        PdfArray   second;
        PdfArray   kids;

        first.insert(first.end(), pArray->begin(), pArray->begin() + (nLength / 2) + 1);
        second.insert(second.end(), pArray->begin() + (nLength / 2) + 1, pArray->end());

        PdfObject* pChild1;
        PdfObject* pChild2 = this->GetObject()->GetDocument()->GetObjects().CreateDictionaryObject();

        if (m_pParent == nullptr)
        {
            m_bHasKids = true;
            pChild1 = this->GetObject()->GetDocument()->GetObjects().CreateDictionaryObject();
            this->GetObject()->GetDictionary().RemoveKey("Names");
        }
        else
        {
            pChild1 = this->GetObject();
            kids = m_pParent->GetObject()->GetDictionary().GetKey("Kids")->GetArray();
        }

        pChild1->GetDictionary().AddKey(key, first);
        pChild2->GetDictionary().AddKey(key, second);

        PdfArray::iterator it = kids.begin();
        while (it != kids.end())
        {
            if (it->GetReference() == pChild1->GetIndirectReference())
            {
                it++;
                it = kids.insert(it, pChild2->GetIndirectReference());
                break;
            }

            it++;
        }

        if (it == kids.end())
        {
            kids.push_back(pChild1->GetIndirectReference());
            kids.push_back(pChild2->GetIndirectReference());
        }

        if (m_pParent == nullptr)
            this->GetObject()->GetDictionary().AddKey("Kids", kids);
        else
            m_pParent->GetObject()->GetDictionary().AddKey("Kids", kids);

        // Important is to the the limits
        // of the children first,
        // because SetLimits( pParent )
        // depends on the /Limits key of all its children!
        PdfNameTreeNode(m_pParent != nullptr ? m_pParent : this, pChild1).SetLimits();
        PdfNameTreeNode(this, pChild2).SetLimits();

        // limits do only change if splitting name arrays
        if (m_bHasKids)
            this->SetLimits();
        else if (m_pParent != nullptr)
            m_pParent->SetLimits();

        return true;
    }

    return false;
}


// NOTE: The NamesTree dict does NOT have a /Type key!
PdfNamesTree::PdfNamesTree(PdfDocument& doc)
    : PdfElement(doc), m_pCatalog(nullptr)
{
}

PdfNamesTree::PdfNamesTree(PdfObject& obj, PdfObject* pCatalog)
    : PdfElement(obj), m_pCatalog(pCatalog)
{
}

void PdfNamesTree::AddValue(const PdfName& tree, const PdfString& key, const PdfObject& rValue)
{
    PdfNameTreeNode root(nullptr, this->GetRootNode(tree, true));
    if (!root.AddValue(key, rValue))
        PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

PdfObject* PdfNamesTree::GetValue(const PdfName& tree, const PdfString& key) const
{
    PdfObject* pObject = this->GetRootNode(tree);
    PdfObject* pResult = nullptr;

    if (pObject != nullptr)
    {
        pResult = this->GetKeyValue(pObject, key);
        if (pResult && pResult->IsReference())
            pResult = this->GetObject().GetDocument()->GetObjects().GetObject(pResult->GetReference());
    }

    return pResult;
}

PdfObject* PdfNamesTree::GetKeyValue(PdfObject* pObj, const PdfString& key) const
{
    if (PdfNamesTree::CheckLimits(pObj, key) != EPdfNameLimits::Inside)
        return nullptr;

    if (pObj->GetDictionary().HasKey("Kids"))
    {
        const PdfArray& kids = pObj->GetDictionary().GetKey("Kids")->GetArray();
        for (auto& child : kids)
        {
            PdfObject* pChild = this->GetObject().GetDocument()->GetObjects().GetObject(child.GetReference());
            if (pChild == nullptr)
            {
                PdfError::LogMessage(LogSeverity::Debug, "Object %lu %lu is child of nametree but was not found!",
                    child.GetReference().ObjectNumber(),
                    child.GetReference().GenerationNumber());
            }
            else
            {
                PdfObject* pResult = GetKeyValue(pChild, key);
                if (pResult) // If recursive call returns nullptr, 
                              // continue with the next element
                              // in the kids array.
                    return pResult;
            }
        }
    }
    else
    {
        PdfArray& names = pObj->GetDictionary().GetKey("Names")->GetArray();
        PdfArray::iterator it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while (it != names.end())
        {
            if (it->GetString() == key)
            {
                it++;
                if (it->IsReference())
                    return this->GetObject().GetDocument()->GetObjects().GetObject((*it).GetReference());

                return &(*it);
            }

            it += 2;
        }

    }

    return nullptr;
}

PdfObject* PdfNamesTree::GetRootNode(const PdfName& name, bool bCreate) const
{
    PdfObject* pObj = this->GetObject().GetIndirectKey(name);
    if (pObj == nullptr && bCreate)
    {
        pObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
        GetNonConstObject()->GetDictionary().AddKey(name, pObj->GetIndirectReference());
    }

    return pObj;
}

bool PdfNamesTree::HasValue( const PdfName & tree, const PdfString & key ) const
{
    return ( this->GetValue( tree, key ) != nullptr );
}

EPdfNameLimits PdfNamesTree::CheckLimits(const PdfObject* pObj, const PdfString& key)
{
    if (pObj->GetDictionary().HasKey("Limits"))
    {
        const PdfArray& limits = pObj->GetDictionary().GetKey("Limits")->GetArray();

        if (limits[0].GetString().GetString() > key.GetString())
            return EPdfNameLimits::Before;

        if (limits[1].GetString().GetString() < key.GetString())
            return EPdfNameLimits::After;
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Debug, "Name tree object %lu %lu does not have a limits key!",
            pObj->GetIndirectReference().ObjectNumber(),
            pObj->GetIndirectReference().GenerationNumber());
    }

    return EPdfNameLimits::Inside;
}

void PdfNamesTree::ToDictionary( const PdfName & tree, PdfDictionary& rDict )
{
    rDict.Clear();
    PdfObject* pObj = this->GetRootNode( tree );
    if( pObj )
        AddToDictionary( pObj, rDict );
}

void PdfNamesTree::AddToDictionary(PdfObject* pObj, PdfDictionary& rDict)
{
    if (pObj->GetDictionary().HasKey("Kids"))
    {
        const PdfArray& kids = pObj->GetDictionary().GetKey("Kids")->GetArray();
        auto it = kids.begin();

        while (it != kids.end())
        {
            PdfObject* pChild = this->GetObject().GetDocument()->GetObjects().GetObject((*it).GetReference());
            if (pChild == nullptr)
            {
                PdfError::LogMessage(LogSeverity::Debug, "Object %lu %lu is child of nametree but was not found!",
                    it->GetReference().ObjectNumber(),
                    it->GetReference().GenerationNumber());
            }
            else
            {
                this->AddToDictionary(pChild, rDict);
            }

            it++;
        }
    }
    else if (pObj->GetDictionary().HasKey("Names"))
    {
        const PdfArray& names = pObj->GetDictionary().GetKey("Names")->GetArray();
        auto it = names.begin();

        // a names array is a set of PdfString/PdfObject pairs
        // so we loop in sets of two - getting each pair
        while (it != names.end())
        {
            // convert all strings into names 
            PdfName name(it->GetString().GetString());
            it++;
            // fixes (security) issue #39 in PoDoFo's tracker (sourceforge.net)
            if (it == names.end())
            {
                PdfError::LogMessage(LogSeverity::Warning,
                    "No reference in /Names array last element in "
                    "object %lu %lu, possible exploit attempt!",
                    pObj->GetIndirectReference().ObjectNumber(),
                    pObj->GetIndirectReference().GenerationNumber());
                break;
            }
            rDict.AddKey(name, *it);
            it++;
        }
    }
}

PdfObject* PdfNamesTree::GetJavaScriptNode(bool bCreate) const
{
    return this->GetRootNode(PdfName("JavaScript"), bCreate);
}

PdfObject* PdfNamesTree::GetDestsNode(bool bCreate) const
{
    return this->GetRootNode(PdfName("Dests"), bCreate);
}
