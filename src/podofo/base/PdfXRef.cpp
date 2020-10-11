/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfDefinesPrivate.h"
#include "PdfXRef.h"

#include "PdfObject.h"
#include "PdfOutputDevice.h"
#include "PdfWriter.h"

#include <algorithm>

#define EMPTY_OBJECT_OFFSET 65535

using namespace std;
using namespace PoDoFo;

PdfXRef::PdfXRef(PdfWriter& writer)
    : m_maxObjNum(0), m_writer(&writer), m_offset( 0 )
{

}

PdfXRef::~PdfXRef() { }

void PdfXRef::AddInUseObject(const PdfReference& ref, optional<uint64_t> offset)
{
    AddObject(ref, offset, true);
}

void PdfXRef::AddFreeObject(const PdfReference& ref)
{
    AddObject(ref, std::nullopt, false);
}

void PdfXRef::AddObject(const PdfReference& ref, optional<uint64_t> offset, bool inUse)
{
    if (ref.ObjectNumber() > m_maxObjNum)
        m_maxObjNum = ref.ObjectNumber();

    if (inUse && offset == std::nullopt)
    {
        // Objects with no offset provided will not be written
        // in the entry list
        return;
    }

    bool insertDone = false;

    for (auto &block : m_vecBlocks)
    {
        if(block.InsertItem(ref, offset, inUse) )
        {
            insertDone = true;
            break;
        }
    }

    if( !insertDone ) 
    {
        PdfXRefBlock block;
        block.First = ref.ObjectNumber();
        block.Count = 1;
        if( inUse )
            block.Items.push_back(XRefItem(ref, offset.value()));
        else
            block.FreeItems.push_back( ref );

        m_vecBlocks.push_back( block );
        std::sort( m_vecBlocks.begin(), m_vecBlocks.end() );
    }
}

void PdfXRef::Write(PdfOutputDevice& device)
{
    PdfXRef::TCIVecXRefBlock it = m_vecBlocks.begin();
    PdfXRef::TCIVecXRefItems itItems;
    PdfXRef::TCIVecReferences itFree;
    const PdfReference* pNextFree  = nullptr;

    uint32_t nFirst = 0;
    uint32_t nCount = 0;

    MergeBlocks();

    m_offset = device.Tell();
    this->BeginWrite(device);
    while( it != m_vecBlocks.end() )
    {
        auto& block = *it;
        nCount = block.Count;
        nFirst = block.First;
        itFree = block.FreeItems.begin();
        itItems = block.Items.begin();

        if( nFirst == 1 )
        {
            --nFirst;
            ++nCount;
        }

        // when there is only one, then we need to start with 0 and the bogus object...
        this->WriteSubSection(device, nFirst, nCount );

        if( !nFirst ) 
        {
            const PdfReference* pFirstFree = this->GetFirstFreeObject( it, itFree );
            this->WriteXRefEntry(device, PdfXRefEntry::CreateFree(pFirstFree ? pFirstFree->ObjectNumber() : 0, EMPTY_OBJECT_OFFSET));
        }

        while( itItems != block.Items.end() )
        {
            // check if there is a free object at the current position
            while( itFree != block.FreeItems.end() &&
                   *itFree < itItems->Reference )
            {
                uint16_t nGen = itFree->GenerationNumber();

                // get a pointer to the next free object
                pNextFree = this->GetNextFreeObject( it, itFree );
                
                // write free object
                this->WriteXRefEntry(device, PdfXRefEntry::CreateFree(pNextFree ? pNextFree->ObjectNumber() : 0, nGen));
                ++itFree;
            }

            this->WriteXRefEntry(device, PdfXRefEntry::CreateInUse(itItems->Offset, itItems->Reference.GenerationNumber()));
            ++itItems;
        }

        // Check if there are any free objects left!
        while( itFree != block.FreeItems.end() )
        {
            uint16_t nGen = itFree->GenerationNumber();
            
            // get a pointer to the next free object
            pNextFree = this->GetNextFreeObject( it, itFree );
            
            // write free object
            this->WriteXRefEntry(device, PdfXRefEntry::CreateFree(pNextFree ? pNextFree->ObjectNumber() : 0, nGen));
            ++itFree;
        }

        ++it;
    }

    this->EndWrite(device);
}

const PdfReference* PdfXRef::GetFirstFreeObject( PdfXRef::TCIVecXRefBlock itBlock, PdfXRef::TCIVecReferences itFree ) const 
{
    // find the next free object
    while( itBlock != m_vecBlocks.end() )
    {
        if( itFree != itBlock->FreeItems.end() )
            break; // got a free object
        
        ++itBlock;
        if(itBlock != m_vecBlocks.end())
            itFree = itBlock->FreeItems.begin();
    }

    // if there is another free object, return it
    if( itBlock != m_vecBlocks.end() &&
        itFree != itBlock->FreeItems.end() )
    {
        return &(*itFree);
    }

    return nullptr;
}

const PdfReference* PdfXRef::GetNextFreeObject( PdfXRef::TCIVecXRefBlock itBlock, PdfXRef::TCIVecReferences itFree ) const 
{
    // check if itFree points to a valid free object at the moment
    if( itFree != itBlock->FreeItems.end() )
        ++itFree; // we currently have a free object, so go to the next one

    // find the next free object
    while( itBlock != m_vecBlocks.end() )
    {
        if( itFree != itBlock->FreeItems.end() )
            break; // got a free object
        
        ++itBlock;
        if( itBlock != m_vecBlocks.end() )
            itFree = itBlock->FreeItems.begin();
    }

    // if there is another free object, return it
    if( itBlock != m_vecBlocks.end() &&
        itFree != itBlock->FreeItems.end() )
    {
        return &(*itFree);
    }

    return nullptr;
}

uint32_t PdfXRef::GetSize() const
{
    // From the PdfReference: /Size's value is 1 greater than the highes object number used in the file.
    return m_maxObjNum + 1;
}

void PdfXRef::MergeBlocks() 
{
    PdfXRef::TIVecXRefBlock it = m_vecBlocks.begin();
    PdfXRef::TIVecXRefBlock itNext = it + 1;

    // Stop in case we have no blocks at all
    if( it == m_vecBlocks.end() )
	    PODOFO_RAISE_ERROR( EPdfError::NoXRef );

    while( itNext != m_vecBlocks.end() )
    {
        auto& curr = *it;
        auto& next = *itNext;
        if(next.First == curr.First + curr.Count )
        {
            // merge the two 
            curr.Count += next.Count;

            curr.Items.reserve(curr.Items.size() + next.Items.size() );
            curr.Items.insert(curr.Items.end(), next.Items.begin(), next.Items.end() );

            curr.FreeItems.reserve(curr.FreeItems.size() + next.FreeItems.size() );
            curr.FreeItems.insert(curr.FreeItems.end(), next.FreeItems.begin(), next.FreeItems.end() );

            itNext = m_vecBlocks.erase(itNext);
            it = itNext - 1;
        }
        else
        {
            it = itNext++;
        }
    }
}

void PdfXRef::BeginWrite( PdfOutputDevice& device)
{
    device.Print( "xref\n" );
}

void PdfXRef::WriteSubSection( PdfOutputDevice& device, uint32_t nFirst, uint32_t nCount )
{
#ifdef DEBUG
    PdfError::DebugMessage("Writing XRef section: %u %u\n", nFirst, nCount );
#endif // DEBUG
    device.Print( "%u %u\n", nFirst, nCount );
}

void PdfXRef::WriteXRefEntry( PdfOutputDevice& device, const PdfXRefEntry& entry)
{
    uint64_t variant;
    switch (entry.Type)
    {
        case EXRefEntryType::Free:
        {
            variant = entry.ObjectNumber;
            break;
        }
        case EXRefEntryType::InUse:
        {
            variant = entry.Offset;
            break;
        }
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }

    device.Print("%0.10" PDF_FORMAT_UINT64 " %0.5hu %c \n", variant, entry.Generation, XRefEntryType(entry.Type));
}

void PdfXRef::EndWriteImpl(PdfOutputDevice& device)
{
    PdfObject  trailer;

    // if we have a dummy offset we write also a prev entry to the trailer
    m_writer->FillTrailerObject(trailer, GetSize(), false);

    device.Print("trailer\n");

    // NOTE: Do not encrypt the trailer dictionary
    trailer.Write(device, m_writer->GetWriteMode(), nullptr);
}

void PdfXRef::EndWrite(PdfOutputDevice& device)
{
    EndWriteImpl(device);
    device.Print("startxref\n%" PDF_FORMAT_UINT64 "\n%%%%EOF\n", GetOffset());
}

void PdfXRef::SetFirstEmptyBlock() 
{
    PdfXRefBlock block;
    block.First = 0;
    block.Count = 1;
    m_vecBlocks.insert(m_vecBlocks.begin(), block );
}

bool PdfXRef::ShouldSkipWrite(const PdfReference& ref)
{
    (void)ref;
    // Nothing to skip writing for PdfXRef table
    return false;
}

bool PdfXRef::PdfXRefBlock::InsertItem(const PdfReference& ref, std::optional<uint64_t> offset, bool inUse)
{
    PODOFO_ASSERT(!inUse || offset.has_value());
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
