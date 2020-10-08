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

#define MAX_XREF_GEN_NUM    65535

static bool ObjectLittle(const PdfObject* p1, const PdfObject* p2);

struct ObjectComparatorPredicate
{
public:
    inline bool operator()( const PdfObject* const & pObj, const PdfObject* const & pObj2 ) const { 
        return pObj->GetIndirectReference() < pObj2->GetIndirectReference();  
    }  
};


struct ReferenceComparatorPredicate {
public:
    inline bool operator()( const PdfReference & pObj, const PdfReference & pObj2 ) const { 
        return pObj < pObj2;
    }
};

//RG: 1) Should this class not be moved to the header file
class ObjectsComparator { 
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
    m_bAutoDelete( false ),
    m_bCanReuseObjectNumbers( true ),
    m_nObjectCount( 1 ),
    m_bSorted( true ),
    m_pStreamFactory(nullptr)
{
}

PdfVecObjects::~PdfVecObjects()
{
    this->Clear();
}

void PdfVecObjects::Clear()
{
    // always work on a copy of the vector
    // in case a child invalidates our iterators
    // with a call to attach or detach.
    
    TVecObservers copy( m_vecObservers );
    TIVecObservers itObservers = copy.begin();
    while( itObservers != copy.end() )
    {
        (*itObservers)->ParentDestructed();
        ++itObservers;
    }

    if( m_bAutoDelete ) 
    {
        TIVecObjects it = this->begin();
        while( it != this->end() )
        {
            delete *it;
            ++it;
        }
    }

    m_vector.clear();

    m_bAutoDelete    = false;
    m_nObjectCount   = 1;
    m_bSorted        = true; // an emtpy vector is sorted
    m_pStreamFactory = nullptr;
}

PdfObject* PdfVecObjects::GetObject( const PdfReference & ref ) const
{
    if( !m_bSorted )
        const_cast<PdfVecObjects*>(this)->Sort();

    PdfObject refObj( ref, nullptr );
    TCIVecObjects it = std::lower_bound( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );
    if( it != m_vector.end() && (refObj.GetIndirectReference() == (*it)->GetIndirectReference()) )
    {
        return *it;
    }

    return nullptr;
}

size_t PdfVecObjects::GetIndex( const PdfReference & ref ) const
{
    if( !m_bSorted )
        const_cast<PdfVecObjects*>(this)->Sort();

    PdfObject refObj( ref, nullptr);
    std::pair<TCIVecObjects,TCIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );

    if( it.first == it.second )
    {
        PODOFO_RAISE_ERROR( EPdfError::NoObject );
    }

    return (it.first - this->begin());
}

PdfObject* PdfVecObjects::RemoveObject( const PdfReference & ref, bool bMarkAsFree )
{
    if( !m_bSorted )
        this->Sort();


    PdfObject*         pObj;
    PdfObject refObj( ref, nullptr);
    std::pair<TIVecObjects,TIVecObjects> it = 
        std::equal_range( m_vector.begin(), m_vector.end(), &refObj, ObjectComparatorPredicate() );

    if( it.first != it.second )
    {
        pObj = *(it.first);
        if( bMarkAsFree )
            this->SafeAddFreeObject( pObj->GetIndirectReference() );
        m_vector.erase( it.first );
        return pObj;
    }
    
    return nullptr;
}

PdfObject* PdfVecObjects::RemoveObject( const TIVecObjects & it )
{
    PdfObject* pObj = *it;
    m_vector.erase( it );
    return pObj;
}

void PdfVecObjects::CollectGarbage(PdfObject& trailer)
{
    // We do not have any objects that have
    // to be on the top, like in a linearized PDF.
    // So we just use an empty list.
    TPdfReferenceSet setLinearizedGroup;
    this->RenumberObjects(trailer, &setLinearizedGroup, true );
}

PdfReference PdfVecObjects::GetNextFreeObject()
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

PdfObject* PdfVecObjects::CreateObject( const char* pszType )
{
    PdfReference ref = this->GetNextFreeObject();
    PdfObject*  pObj = new PdfObject( ref, pszType );
    pObj->SetDocument(*m_pDocument);

    this->AddObject( pObj );

    return pObj;
}

PdfObject* PdfVecObjects::CreateObject( const PdfVariant & rVariant )
{
    PdfReference ref = this->GetNextFreeObject();
    PdfObject*  pObj = new PdfObject(rVariant);
    pObj->SetIndirectReference(ref);
    pObj->SetDocument(*m_pDocument);    

    this->AddObject( pObj );

    return pObj;
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

void PdfVecObjects::PushObject(PdfObject * pObj, const PdfReference & reference)
{
    if (GetObject(reference))
    {
        PdfError::LogMessage(ELogSeverity::Warning, "Object: %" PDF_FORMAT_INT64 " 0 R will be deleted and loaded again.", reference.ObjectNumber());
        delete RemoveObject(reference, false);
    }

    pObj->SetIndirectReference(reference);
    AddObject(pObj);
}

void PdfVecObjects::AddObject(PdfObject * pObj)
{
    SetObjectCount(pObj->GetIndirectReference());
    pObj->SetDocument(*m_pDocument);

    if( m_bSorted && !m_vector.empty() && pObj->GetIndirectReference() < m_vector.back()->GetIndirectReference() )
    {
        TVecObjects::iterator i_pos = 
            std::lower_bound(m_vector.begin(),m_vector.end(),pObj,ObjectLittle);
        m_vector.insert(i_pos, pObj );
    }
    else 
    {
        m_vector.push_back( pObj );
    }
}

void PdfVecObjects::RenumberObjects(PdfObject& trailer, TPdfReferenceSet* pNotDelete, bool bDoGarbageCollection )
{
    throw runtime_error("Fixme, we don't support taking address of PdfReference anymore. See InsertOneReferenceIntoVector");

    TVecReferencePointerList list;
    TIVecReferencePointerList it;
    TIReferencePointerList itList;
    int i = 0;

    m_lstFreeObjects.clear();

    if( !m_bSorted )
        const_cast<PdfVecObjects*>(this)->Sort();

    // The following call slows everything down
    // optimization welcome
    BuildReferenceCountVector(list);
    InsertReferencesIntoVector( trailer, list);

    if( bDoGarbageCollection )
    {
        GarbageCollection(list, trailer, pNotDelete );
    }

    it = list.begin();
    while( it != list.end() )
    {
        PdfReference ref(i + 1, 0);
        m_vector[i]->m_reference = ref;

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

void PdfVecObjects::InsertOneReferenceIntoVector( const PdfObject& obj, TVecReferencePointerList& list )  
{
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

    // FIX-ME: We don't support taking address of reference anynmore
    (void)list;
    ////size_t index = it.first - this->begin();
    ////list[index].push_back( const_cast<PdfReference*>(&(obj.GetReference() )) );
}

void PdfVecObjects::InsertReferencesIntoVector(const PdfObject& obj, TVecReferencePointerList& list )
{
    PdfArray::const_iterator   itArray;
    TCIKeyMap                  itKeys;
  
    if(obj.IsReference())
    {
        InsertOneReferenceIntoVector( obj, list );
    }
    else if( obj.IsArray() )
    {
        itArray = obj.GetArray().begin(); 
        while( itArray != obj.GetArray().end() )
        {
            if( (*itArray).IsReference() )
                InsertOneReferenceIntoVector(*itArray, list );
            else if( (*itArray).IsArray() ||
                     (*itArray).IsDictionary() )
                InsertReferencesIntoVector(*itArray, list );

            ++itArray;
        }
    }
    else if( obj.IsDictionary() )
    {
        itKeys = obj.GetDictionary().begin();
        while( itKeys != obj.GetDictionary().end() )
        {
            if( itKeys->second.IsReference() )
                InsertOneReferenceIntoVector(itKeys->second, list );
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            else if( itKeys->second.IsArray() ||
                itKeys->second.IsDictionary() )
                InsertReferencesIntoVector(itKeys->second, list );
            
            ++itKeys;
        }
    }
}

void PdfVecObjects::GetObjectDependencies( const PdfObject& obj, TPdfReferenceList& list ) const
{
    PdfArray::const_iterator   itArray;
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
    else if( obj.IsArray() )
    {
        itArray = obj.GetArray().begin(); 
        while( itArray != obj.GetArray().end() )
        {
            if( itArray->IsArray() ||
                itArray->IsDictionary() ||
                itArray->IsReference() )
                GetObjectDependencies( *itArray, list );

            ++itArray;
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

void PdfVecObjects::BuildReferenceCountVector( TVecReferencePointerList& list )
{
    TCIVecObjects it = this->begin();

    list.clear();
    list.resize( !m_vector.empty() );

    while( it != this->end() )
    {
        auto &obj = **it;
        if (obj.IsReference())
        {
            InsertOneReferenceIntoVector(obj, list);
        }
        else if (obj.IsArray() || obj.IsDictionary())
        {
            // optimization as this is really slow:
            // Call only for dictionaries, references and arrays
            InsertReferencesIntoVector(obj, list);
        }

        ++it;
    }
}

void PdfVecObjects::Sort()
{
    if( !m_bSorted )
    {
        std::sort( this->begin(), this->end(), ObjectLittle );
        m_bSorted = true;
    }
}

void PdfVecObjects::GarbageCollection( TVecReferencePointerList& list, PdfObject&, TPdfReferenceSet* pNotDelete )
{
    TIVecReferencePointerList it = list.begin();
    int pos = 0;
    bool bContains = false;

    while( it != list.end() )
    {
        bContains = pNotDelete ? ( pNotDelete->find( m_vector[pos]->GetIndirectReference() ) != pNotDelete->end() ) : false;
        if( !(*it).size() && !bContains )
        {
            m_vector.erase( this->begin() + pos );
        }
        
        ++pos;
        ++it;
    }

    m_nObjectCount = ++pos;
}

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
            ++it;
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

PdfStream* PdfVecObjects::CreateStream( const PdfStream & )
{
    return nullptr;
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

TIVecObjects PdfVecObjects::begin()
{
    return m_vector.begin();
}

TCIVecObjects PdfVecObjects::begin() const
{
    return m_vector.begin();
}

TIVecObjects PdfVecObjects::end()
{
    return m_vector.end();
}

TCIVecObjects PdfVecObjects::end() const
{
    return m_vector.end();
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


bool ObjectLittle(const PdfObject* p1, const PdfObject* p2)
{
    return *p1 < *p2;
}


