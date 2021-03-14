/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfVecObjects.h"

#include <stdexcept>
#include <algorithm>

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfReference.h"
#include "PdfStream.h"
#include "PdfDefinesPrivate.h"

using namespace std;
using namespace PoDoFo;

#define MAX_XREF_GEN_NUM 65535

static bool CompareObject(const PdfObject* obj1, const PdfObject* obj2);
static bool CompareReference(const PdfObject* obj, const PdfReference& ref);

struct ObjectComparatorPredicate
{
public:
    inline bool operator()( const PdfObject* const & pObj, const PdfObject* const & pObj2 ) const
    { 
        return pObj->GetIndirectReference() < pObj2->GetIndirectReference();  
    }
};

struct ReferenceComparatorPredicate
{
public:
    inline bool operator()( const PdfReference & pObj, const PdfReference & pObj2 ) const { 
        return pObj < pObj2;
    }
};

//RG: 1) Should this class not be moved to the header file
class ObjectsComparator
{
public:
    ObjectsComparator( const PdfReference & ref )
        : m_ref( ref )
        {
        }
    
    bool operator()(const PdfObject* p1) const { 
        return p1 ? (p1->GetIndirectReference() == m_ref ) : false;
    }

private:
    ObjectsComparator(void) = delete;
    ObjectsComparator(const ObjectsComparator& rhs) = delete;
    ObjectsComparator& operator=(const ObjectsComparator& rhs) = delete;

private:
    const PdfReference m_ref;
};

// This is static, IMHO (mabri) different values per-instance could cause confusion.
// It has to be defined here because of the one-definition rule.
size_t PdfVecObjects::m_nMaxReserveSize = static_cast<size_t>(8388607); // cf. Table C.1 in section C.2 of PDF32000_2008.pdf

PdfVecObjects::PdfVecObjects(PdfDocument& document) :
    m_pDocument(&document),
    m_bCanReuseObjectNumbers( true ),
    m_nObjectCount( 1 ),
    m_sorted( true ),
    m_pStreamFactory(nullptr)
{
}

PdfVecObjects::~PdfVecObjects()
{
    Clear();
}

void PdfVecObjects::Clear()
{
    for (auto obj : m_vector)
        delete obj;

    m_vector.clear();
    m_nObjectCount = 1;
    m_sorted = true;
    m_pStreamFactory = nullptr;
}

PdfObject* PdfVecObjects::GetObject( const PdfReference & ref ) const
{
    const_cast<PdfVecObjects&>(*this).Sort();
    TCIVecObjects it = std::lower_bound( m_vector.begin(), m_vector.end(), ref, CompareReference);
    if( it == m_vector.end() || (*it)->GetIndirectReference() != ref)
        return nullptr;

    return *it;
}

unique_ptr<PdfObject> PdfVecObjects::RemoveObject( const PdfReference & ref, bool bMarkAsFree )
{
    const_cast<PdfVecObjects&>(*this).Sort();
    TCIVecObjects it = std::lower_bound(m_vector.begin(), m_vector.end(), ref, CompareReference);
    if (it == m_vector.end() || (*it)->GetIndirectReference() != ref)
        return nullptr;

    auto pObj = *(it);
    if( bMarkAsFree )
        SafeAddFreeObject(pObj->GetIndirectReference());

    m_vector.erase(it);
    m_sorted = false;
    return unique_ptr<PdfObject>(pObj);
}

unique_ptr<PdfObject> PdfVecObjects::RemoveObject( const TIVecObjects & it )
{
    auto pObj = *it;
    m_vector.erase( it );
    m_sorted = false;
    return unique_ptr<PdfObject>(pObj);
}

PdfReference PdfVecObjects::getNextFreeObject()
{
    // Try to first use list of free objects
    if ( m_bCanReuseObjectNumbers && !m_lstFreeObjects.empty() )
    {
        PdfReference freeObjectRef = m_lstFreeObjects.front();
        m_lstFreeObjects.pop_front();
        return freeObjectRef;
    }

    // If no free objects are available, create a new object with generation 0
    uint16_t nextObjectNum = static_cast<uint16_t>( m_nObjectCount );
    while ( true )
    {
        if ( ( size_t )( nextObjectNum + 1 ) == m_nMaxReserveSize )
            PODOFO_RAISE_ERROR_INFO( EPdfError::ValueOutOfRange , "Reached the maximum number of indirect objects");

        // Check also if the object number it not available,
        // e.g. it reached maximum generation number (65535)
        if ( m_lstUnavailableObjects.find( nextObjectNum ) == m_lstUnavailableObjects.end() )
            break;
        
        nextObjectNum++;
    }

    return PdfReference( nextObjectNum, 0 );
}

PdfObject* PdfVecObjects::CreateDictionaryObject(const string_view& type)
{
    auto dict = PdfDictionary();
    if (!type.empty())
        dict.AddKey(PdfName::KeyType, PdfName(type));

    auto ret = new PdfObject(dict, true);
    addNewObject(ret);
    return ret;
}

PdfObject* PdfVecObjects::CreateObject( const PdfVariant & rVariant )
{
    auto ret = new PdfObject(rVariant, true);
    addNewObject(ret);
    return ret;
}

int32_t PdfVecObjects::SafeAddFreeObject( const PdfReference & rReference )
{
    // From 3.4.3 Cross-Reference Table:
    // "When an indirect object is deleted, its cross-reference
    // entry is marked free and it is added to the linked list
    // of free entries.The entry’s generation number is incremented by
    // 1 to indicate the generation number to be used the next time an
    // object with that object number is created. Thus, each time
    // the entry is reused, it is given a new generation number."
    return tryAddFreeObject( rReference.ObjectNumber(), rReference.GenerationNumber() + 1 );
}

bool PdfVecObjects::TryAddFreeObject( const PdfReference & rReference )
{
    return tryAddFreeObject( rReference.ObjectNumber() , rReference.GenerationNumber() ) != -1;
}

int32_t PdfVecObjects::tryAddFreeObject( uint32_t objnum, uint32_t gennum )
{
    // Documentation 3.4.3 Cross-Reference Table states: "The maximum
    // generation number is 65535; when a cross reference entry reaches
    // this value, it is never reused."
    // NOTE: gennum is uint32 to accomodate overflows from callers
    if ( gennum >= MAX_XREF_GEN_NUM )
    {
        m_lstUnavailableObjects.insert( gennum );
        return -1;
    }

    AddFreeObject( PdfReference( objnum, gennum ) );
    return gennum;
}

void PdfVecObjects::AddFreeObject( const PdfReference & rReference )
{
    std::pair<TIPdfReferenceList,TIPdfReferenceList> it = 
        std::equal_range( m_lstFreeObjects.begin(), m_lstFreeObjects.end(), rReference, ReferenceComparatorPredicate() );

    if( it.first != it.second && !m_lstFreeObjects.empty() ) 
    {
        // Be sure that no reference is added twice to free list
        PdfError::DebugMessage( "Adding %d to free list, is already contained in it!", rReference.ObjectNumber() );
        return;
    }
    else
    {
        // When append free objects from external doc we need plus one number objects
        SetObjectCount( rReference );

        // Insert so that list stays sorted
        m_lstFreeObjects.insert( it.first, rReference );
    }
}

void PdfVecObjects::PushObject(const PdfReference & ref, PdfObject* pObj)
{
    if (GetObject(ref))
    {
        PdfError::LogMessage(ELogSeverity::Warning, "Object: %" PDF_FORMAT_INT64 " 0 R will be deleted and loaded again.", ref.ObjectNumber());
        RemoveObject(ref, false);
    }

    pObj->SetIndirectReference(ref);
    AddObject(pObj);
}

void PdfVecObjects::addNewObject(PdfObject* obj)
{
    PdfReference ref = getNextFreeObject();
    obj->SetIndirectReference(ref);
    obj->SetDocument(*m_pDocument);
    AddObject(obj);
}

void PdfVecObjects::AddObject(PdfObject * pObj)
{
    SetObjectCount(pObj->GetIndirectReference());
    pObj->SetDocument(*m_pDocument);
    m_vector.push_back(pObj);
    m_sorted = false;
}

void PdfVecObjects::Sort()
{
    if (m_sorted)
        return;

    std::sort(m_vector.begin(), m_vector.end(), CompareObject);
    m_sorted = true;
}

#pragma region Untested

void PdfVecObjects::CollectGarbage(PdfObject& trailer)
{
    // We do not have any objects that have
    // to be on the top, like in a linearized PDF.
    // So we just use an empty list.
    TPdfReferenceSet setLinearizedGroup;
    this->RenumberObjects(trailer, &setLinearizedGroup, true);
}


void PdfVecObjects::RenumberObjects(PdfObject& trailer, TPdfReferenceSet* pNotDelete, bool bDoGarbageCollection )
{
    throw runtime_error("Fixme, we don't support taking address of PdfReference anymore. See InsertOneReferenceIntoVector");

    TVecReferencePointerList list;
    TIVecReferencePointerList it;
    TIReferencePointerList itList;
    int i = 0;

    m_lstFreeObjects.clear();

    const_cast<PdfVecObjects&>(*this).Sort();

    // The following call slows everything down
    // optimization welcome
    buildReferenceCountVector(list);
    insertReferencesIntoVector( trailer, list);

    if( bDoGarbageCollection )
    {
        garbageCollection(list, trailer, pNotDelete );
    }

    it = list.begin();
    while( it != list.end() )
    {
        PdfReference ref(i + 1, 0);
        m_vector[i]->m_IndirectReference = ref;

        itList = (*it).begin();
        while( itList != (*it).end() )
        {
            *(*itList) = ref;
            
            ++itList;
        }

        ++i;
        ++it;
    }

    
}

void PdfVecObjects::insertOneReferenceIntoVector( const PdfObject& obj, TVecReferencePointerList& list )  
{
    (void)list;
    throw runtime_error("Fixme, we don't support taking address of PdfReference anymore");

    /* FIX-ME
    PODOFO_RAISE_LOGIC_IF( !m_bSorted, 
                           "PdfVecObjects must be sorted before calling PdfVecObjects::InsertOneReferenceIntoVector!" );
    
    // we asume that pObj is a reference - no checking here because of speed
    std::pair<TCIVecObjects,TCIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), &obj, ObjectComparatorPredicate() );

    if( it.first != it.second )
    {
        // ignore this reference
        return;
    }

    size_t index = it.first - m_vector.begin();
    list[index].push_back( const_cast<PdfReference*>(&(obj.GetReference() )) );
    */
}

void PdfVecObjects::insertReferencesIntoVector(const PdfObject& obj, TVecReferencePointerList& list )
{
    TCIKeyMap itKeys;
  
    if(obj.IsReference())
    {
        insertOneReferenceIntoVector( obj, list );
    }
    else if (obj.IsArray())
    {
        for (auto& child : obj.GetArray())
        {
            if (child.IsReference())
                insertOneReferenceIntoVector(child, list);
            else if (child.IsArray() || child.IsDictionary())
                insertReferencesIntoVector(child, list);
        }
    }
    else if( obj.IsDictionary() )
    {
        itKeys = obj.GetDictionary().begin();
        while( itKeys != obj.GetDictionary().end() )
        {
            if( itKeys->second.IsReference() )
                insertOneReferenceIntoVector(itKeys->second, list );
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            else if( itKeys->second.IsArray() ||
                itKeys->second.IsDictionary() )
                insertReferencesIntoVector(itKeys->second, list );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::GetObjectDependencies( const PdfObject& obj, TPdfReferenceList& list ) const
{
    TCIKeyMap                  itKeys;
  
    if( obj.IsReference() )
    {
        std::pair<TPdfReferenceList::iterator, TPdfReferenceList::iterator> itEqualRange
            = std::equal_range( list.begin(), list.end(), obj.GetReference() );
        if( itEqualRange.first == itEqualRange.second )
        {
            list.insert(itEqualRange.first, obj.GetReference() );

            const PdfObject* referencedObject = this->GetObject(obj.GetReference());
            if( referencedObject != nullptr)
            {
                GetObjectDependencies( *referencedObject, list );
            }
        }
    }
    else if (obj.IsArray())
    {
        for (auto& child : obj.GetArray())
        {
            if (child.IsArray() || child.IsDictionary() || child.IsReference())
                GetObjectDependencies(child, list);
        }
    }
    else if( obj.IsDictionary() )
    {
        itKeys = obj.GetDictionary().begin();
        while( itKeys != obj.GetDictionary().end() )
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            if( itKeys->second.IsArray() ||
                itKeys->second.IsDictionary() ||
                itKeys->second.IsReference() )
                GetObjectDependencies( itKeys->second, list );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::buildReferenceCountVector( TVecReferencePointerList& list )
{
    TCIVecObjects it = m_vector.begin();

    list.clear();
    list.resize( !m_vector.empty() );

    while( it != m_vector.end() )
    {
        auto &obj = **it;
        if (obj.IsReference())
        {
            insertOneReferenceIntoVector(obj, list);
        }
        else if (obj.IsArray() || obj.IsDictionary())
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            insertReferencesIntoVector(obj, list);
        }

        ++it;
    }
}

void PdfVecObjects::garbageCollection( TVecReferencePointerList& list, PdfObject&, TPdfReferenceSet* pNotDelete )
{
    TIVecReferencePointerList it = list.begin();
    int pos = 0;
    bool bContains = false;

    while( it != list.end() )
    {
        bContains = pNotDelete ? ( pNotDelete->find( m_vector[pos]->GetIndirectReference() ) != pNotDelete->end() ) : false;
        if( !(*it).size() && !bContains )
        {
            m_vector.erase(m_vector.begin() + pos );
        }
        
        ++pos;
        ++it;
    }

    m_nObjectCount = ++pos;
}

#pragma endregion

void PdfVecObjects::Detach( Observer* pObserver )
{
    TIVecObservers it = m_vecObservers.begin();

    while( it != m_vecObservers.end() )
    {
        if( *it == pObserver ) 
        {
            m_vecObservers.erase( it );
            break;
        }
        else
        {
            ++it;
        }
    }
}

PdfStream* PdfVecObjects::CreateStream( PdfObject* pParent )
{
    PdfStream* pStream = m_pStreamFactory == nullptr ?
        new PdfMemStream(pParent) :
        m_pStreamFactory->CreateStream( pParent );

    return pStream;
}

void PdfVecObjects::WriteObject( PdfObject* pObject )
{
    // Tell any observers that there are new objects to write
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->WriteObject( pObject );
        ++itObservers;
    }
}

void PdfVecObjects::Finish()
{
    // always work on a copy of the vector
    // in case a child invalidates our iterators
    // with a call to attach or detach.
    
    TVecObservers copy( m_vecObservers );
    TIVecObservers itObservers = copy.begin();
    while( itObservers != copy.end() )
    {
        (*itObservers)->Finish();
        ++itObservers;
    }
}

void PdfVecObjects::BeginAppendStream( const PdfStream* pStream )
{
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->BeginAppendStream( pStream );
        ++itObservers;
    }
}
    
void PdfVecObjects::EndAppendStream( const PdfStream* pStream )
{
    TIVecObservers itObservers = m_vecObservers.begin();
    while( itObservers != m_vecObservers.end() )
    {
        (*itObservers)->EndAppendStream( pStream );
        ++itObservers;
    }
}

std::string PdfVecObjects::GetNextSubsetPrefix()
{
	if ( m_sSubsetPrefix == "" )
	{
		m_sSubsetPrefix = "AAAAAA+";
	}
	else
	{
		PODOFO_ASSERT( m_sSubsetPrefix.length() == 7 );
		PODOFO_ASSERT( m_sSubsetPrefix[6] == '+' );
	
		for ( int i = 5; i >= 0; i-- )
		{
			if ( m_sSubsetPrefix[i] < 'Z' )
			{
				m_sSubsetPrefix[i]++;
				break;
			}
			m_sSubsetPrefix[i] = 'A';
		}
	}

	return m_sSubsetPrefix;
}

void PdfVecObjects::SetCanReuseObjectNumbers( bool bCanReuseObjectNumbers )
{
    m_bCanReuseObjectNumbers = bCanReuseObjectNumbers;

    if( !m_bCanReuseObjectNumbers )
    {
        m_lstFreeObjects.clear();
    }
}

size_t PdfVecObjects::GetSize() const
{
    return m_vector.size();
}

void PdfVecObjects::Reserve(size_t size)
{
    if (size <= m_nMaxReserveSize) // Fix CVE-2018-5783
    {
        m_vector.reserve(size);
    }
    else
    {
        PdfError::DebugMessage("Call to PdfVecObjects::Reserve with %"
            PDF_SIZE_FORMAT" is over allowed limit of %"
            PDF_SIZE_FORMAT".\n", size, m_nMaxReserveSize);
    }
}

void PdfVecObjects::Attach(Observer* pObserver)
{
    m_vecObservers.push_back(pObserver);
}

void PdfVecObjects::SetStreamFactory(StreamFactory* pFactory)
{
    m_pStreamFactory = pFactory;
}

PdfObject* PdfVecObjects::GetBack()
{
    return m_vector.back();
}

void PdfVecObjects::SetObjectCount(const PdfReference& rRef)
{
    if (rRef.ObjectNumber() >= m_nObjectCount)
    {
        // "m_bObjectCount" is used for the next free object number.
        // We need to use the greatest object number + 1 for the next free object number.
        // Otherwise, object number overlap would have occurred.
        m_nObjectCount = rRef.ObjectNumber() + 1;
    }
}

PdfObject*& PdfVecObjects::operator[](size_t index) { return m_vector[index]; }

TCIVecObjects PdfVecObjects::begin() const
{
    const_cast<PdfVecObjects&>(*this).Sort();
    return m_vector.begin();
}

TCIVecObjects PdfVecObjects::end() const
{
    return m_vector.end();
}

size_t PdfVecObjects::size() const
{
    return m_vector.size();
}

bool CompareObject(const PdfObject* p1, const PdfObject* p2)
{
    return *p1 < *p2;
}

bool CompareReference(const PdfObject* obj, const PdfReference& ref)
{
    return obj->GetIndirectReference() < ref;
}
