/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfXRef.h"

#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfWriter.h"

#include <algorithm>

#define EMPTY_OBJECT_GENERATION 65535

using namespace std;
using namespace mm;

PdfXRef::PdfXRef(PdfWriter& writer)
    : m_maxObjCount(0), m_writer(&writer), m_offset(0)
{

}

PdfXRef::~PdfXRef() { }

void PdfXRef::AddInUseObject(const PdfReference& ref, nullable<uint64_t> offset)
{
    AddObject(ref, offset, true);
}

void PdfXRef::AddFreeObject(const PdfReference& ref)
{
    AddObject(ref, nullptr, false);
}

void PdfXRef::AddObject(const PdfReference& ref, nullable<uint64_t> offset, bool inUse)
{
    if (ref.ObjectNumber() > m_maxObjCount)
        m_maxObjCount = ref.ObjectNumber();

    if (inUse && offset == nullptr)
    {
        // Objects with no offset provided will not be written
        // in the entry list
        return;
    }

    bool insertDone = false;

    for (auto& block : m_blocks)
    {
        if (block.InsertItem(ref, offset, inUse))
        {
            insertDone = true;
            break;
        }
    }

    if (!insertDone)
    {
        PdfXRefBlock block;
        block.First = ref.ObjectNumber();
        block.Count = 1;
        if (inUse)
            block.Items.push_back(XRefItem(ref, offset.value()));
        else
            block.FreeItems.push_back(ref);

        m_blocks.push_back(block);
        std::sort(m_blocks.begin(), m_blocks.end());
    }
}

void PdfXRef::Write(PdfOutputDevice& device)
{
    auto it = m_blocks.begin();
    XRefItemList::const_iterator itItems;
    ReferenceList::const_iterator itFree;
    const PdfReference* nextFree = nullptr;

    uint32_t first = 0;
    uint32_t count = 0;

    MergeBlocks();

    m_offset = device.Tell();
    this->BeginWrite(device);
    while (it != m_blocks.end())
    {
        auto& block = *it;
        count = block.Count;
        first = block.First;
        itFree = block.FreeItems.begin();
        itItems = block.Items.begin();

        if (first == 1)
        {
            first--;
            count++;
        }

        // when there is only one, then we need to start with 0 and the bogus object...
        this->WriteSubSection(device, first, count);

        if (first == 0)
        {
            const PdfReference* firstFree = this->GetFirstFreeObject(it, itFree);
            this->WriteXRefEntry(device, PdfReference(0, EMPTY_OBJECT_GENERATION),
                PdfXRefEntry::CreateFree(firstFree == nullptr ? 0 : firstFree->ObjectNumber(), EMPTY_OBJECT_GENERATION));
        }

        while (itItems != block.Items.end())
        {
            // check if there is a free object at the current position
            while (itFree != block.FreeItems.end() &&
                *itFree < itItems->Reference)
            {
                uint16_t genNo = itFree->GenerationNumber();

                // get a pointer to the next free object
                nextFree = this->GetNextFreeObject(it, itFree);

                // write free object
                this->WriteXRefEntry(device, *itFree, PdfXRefEntry::CreateFree(nextFree == nullptr ? 0 : nextFree->ObjectNumber(), genNo));
                itFree++;
            }

            this->WriteXRefEntry(device, itItems->Reference, PdfXRefEntry::CreateInUse(itItems->Offset, itItems->Reference.GenerationNumber()));
            itItems++;
        }

        // Check if there are any free objects left!
        while (itFree != block.FreeItems.end())
        {
            uint16_t genNo = itFree->GenerationNumber();

            // get a pointer to the next free object
            nextFree = this->GetNextFreeObject(it, itFree);

            // write free object
            this->WriteXRefEntry(device, *itFree, PdfXRefEntry::CreateFree(nextFree  == nullptr ? 0 : nextFree->ObjectNumber(), genNo));
            itFree++;
        }

        it++;
    }

    this->EndWrite(device);
}

const PdfReference* PdfXRef::GetFirstFreeObject(XRefBlockList::const_iterator itBlock, ReferenceList::const_iterator itFree) const
{
    // find the next free object
    while (itBlock != m_blocks.end())
    {
        if (itFree != itBlock->FreeItems.end())
            break; // got a free object

        itBlock++;
        if (itBlock != m_blocks.end())
            itFree = itBlock->FreeItems.begin();
    }

    // if there is another free object, return it
    if (itBlock != m_blocks.end() &&
        itFree != itBlock->FreeItems.end())
    {
        return &(*itFree);
    }

    return nullptr;
}

const PdfReference* PdfXRef::GetNextFreeObject(XRefBlockList::const_iterator itBlock, ReferenceList::const_iterator itFree) const
{
    // check if itFree points to a valid free object at the moment
    if (itFree != itBlock->FreeItems.end())
        itFree++; // we currently have a free object, so go to the next one

    // find the next free object
    while (itBlock != m_blocks.end())
    {
        if (itFree != itBlock->FreeItems.end())
            break; // got a free object

        itBlock++;
        if (itBlock != m_blocks.end())
            itFree = itBlock->FreeItems.begin();
    }

    // if there is another free object, return it
    if (itBlock != m_blocks.end() &&
        itFree != itBlock->FreeItems.end())
    {
        return &(*itFree);
    }

    return nullptr;
}

uint32_t PdfXRef::GetSize() const
{
    // From the PdfReference: /Size's value is 1 greater than the highes object number used in the file.
    return m_maxObjCount + 1;
}

void PdfXRef::MergeBlocks()
{
    auto it = m_blocks.begin();
    auto itNext = it + 1;

    // Stop in case we have no blocks at all
    if (it == m_blocks.end())
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    while (itNext != m_blocks.end())
    {
        auto& curr = *it;
        auto& next = *itNext;
        if (next.First == curr.First + curr.Count)
        {
            // merge the two 
            curr.Count += next.Count;

            curr.Items.reserve(curr.Items.size() + next.Items.size());
            curr.Items.insert(curr.Items.end(), next.Items.begin(), next.Items.end());

            curr.FreeItems.reserve(curr.FreeItems.size() + next.FreeItems.size());
            curr.FreeItems.insert(curr.FreeItems.end(), next.FreeItems.begin(), next.FreeItems.end());

            itNext = m_blocks.erase(itNext);
            it = itNext - 1;
        }
        else
        {
            it = itNext++;
        }
    }
}

void PdfXRef::BeginWrite(PdfOutputDevice& device)
{
    device.Write("xref\n");
}

void PdfXRef::WriteSubSection(PdfOutputDevice& device, uint32_t first, uint32_t count)
{
#ifndef VERBOSE_DEBUG_DISABLED
    PdfError::LogMessage(PdfLogSeverity::Debug, "Writing XRef section: {} {}", first, count);
#endif // DEBUG
    device.Write(COMMON_FORMAT("{} {}\n", first, count));
}

void PdfXRef::WriteXRefEntry(PdfOutputDevice& device, const PdfReference& ref, const PdfXRefEntry& entry)
{
    (void)ref;
    uint64_t variant;
    switch (entry.Type)
    {
        case XRefEntryType::Free:
        {
            variant = entry.ObjectNumber;
            break;
        }
        case XRefEntryType::InUse:
        {
            variant = entry.Offset;
            break;
        }
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    device.Write(COMMON_FORMAT("{:010d} {:05d} {} \n", variant, entry.Generation, XRefEntryTypeToChar(entry.Type)));
}

void PdfXRef::EndWriteImpl(PdfOutputDevice& device)
{
    PdfObject  trailer;

    // if we have a dummy offset we write also a prev entry to the trailer
    m_writer->FillTrailerObject(trailer, GetSize(), false);

    device.Write("trailer\n");

    // NOTE: Do not encrypt the trailer dictionary
    trailer.Write(device, m_writer->GetWriteFlags(), nullptr);
}

void PdfXRef::EndWrite(PdfOutputDevice& device)
{
    EndWriteImpl(device);
    device.Write(COMMON_FORMAT("startxref\n{}\n%%EOF\n", GetOffset()));
}

void PdfXRef::SetFirstEmptyBlock()
{
    PdfXRefBlock block;
    block.First = 0;
    block.Count = 1;
    m_blocks.insert(m_blocks.begin(), block);
}

bool PdfXRef::ShouldSkipWrite(const PdfReference& ref)
{
    (void)ref;
    // No object to skip in PdfXRef table
    return false;
}

bool PdfXRef::PdfXRefBlock::InsertItem(const PdfReference& ref, nullable<uint64_t> offset, bool inUse)
{
    PDFMM_ASSERT(!inUse || offset.has_value());
    if (ref.ObjectNumber() == First + Count)
    {
        // Insert at back
        Count++;

        if (inUse)
            Items.push_back(XRefItem(ref, offset.value()));
        else
            FreeItems.push_back(ref);

        return true; // no sorting required
    }
    else if (ref.ObjectNumber() == First - 1)
    {
        // Insert at front 
        First--;
        Count++;

        // This is known to be slow, but should not occur actually
        if (inUse)
            Items.insert(Items.begin(), XRefItem(ref, offset.value()));
        else
            FreeItems.insert(FreeItems.begin(), ref);

        return true; // no sorting required
    }
    else if (ref.ObjectNumber() > First - 1 &&
        ref.ObjectNumber() < First + Count)
    {
        // Insert at back
        Count++;

        if (inUse)
        {
            Items.push_back(XRefItem(ref, offset.value()));
            std::sort(Items.begin(), Items.end());
        }
        else
        {
            FreeItems.push_back(ref);
            std::sort(FreeItems.begin(), FreeItems.end());
        }

        return true;
    }

    return false;
}
