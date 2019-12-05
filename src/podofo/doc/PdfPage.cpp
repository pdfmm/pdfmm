/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#include "PdfPage.h" 

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"
#include "base/PdfWriter.h"
#include "base/PdfStream.h"
#include "base/PdfColor.h"

#include "PdfDocument.h"

namespace PoDoFo {

PdfPage::PdfPage( const PdfRect & rSize, PdfDocument* pParent )
    : PdfElement( "Page", pParent ), PdfCanvas(), m_pContents( NULL )
{
    InitNewPage( rSize );
}

PdfPage::PdfPage( const PdfRect & rSize, PdfVecObjects* pParent )
    : PdfElement( "Page", pParent ), PdfCanvas(), m_pContents( NULL )
{
    InitNewPage( rSize );
}

PdfPage::PdfPage( PdfObject* pObject, const std::deque<PdfObject*> & rListOfParents )
    : PdfElement( "Page", pObject ), PdfCanvas()
{
    m_pResources = GetObject()->GetDictionary().FindKey( "Resources" );
    if( !m_pResources ) 
    {
        // Resources might be inherited
        std::deque<PdfObject*>::const_reverse_iterator it = rListOfParents.rbegin();

        while( it != rListOfParents.rend() && !m_pResources )
        {
            m_pResources = (*it)->GetIndirectKey( "Resources" );
            ++it;
        }
    }

    PdfObject* contents = GetObject()->GetDictionary().FindKey("Contents");
    if (contents != nullptr)
        m_pContents = new PdfContents(*this, *contents);
}

PdfPage::~PdfPage()
{
    TIMapAnnotation dit, dend = m_mapAnnotations.end();

    for( dit = m_mapAnnotations.begin(); dit != dend; dit++ )
    {
        delete (*dit).second;
    }

    delete m_pContents;	// just clears the C++ object from memory, NOT the PdfObject
}

PdfRect PdfPage::GetSize() const
{
    return this->GetMediaBox();
}

void PdfPage::InitNewPage( const PdfRect & rSize )
{
    SetMediaBox(rSize);

    // The PDF specification suggests that we send all available PDF Procedure sets
    this->GetObject()->GetDictionary().AddKey( "Resources", PdfObject( PdfDictionary() ) );

    m_pResources = this->GetObject()->GetIndirectKey( "Resources" );
    m_pResources->GetDictionary().AddKey( "ProcSet", PdfCanvas::GetProcSet() );
}

void PdfPage::CreateContents() 
{
    if( !m_pContents ) 
    {
        m_pContents = new PdfContents(*this);
        this->GetObject()->GetDictionary().AddKey( PdfName::KeyContents, 
                                                   m_pContents->GetContents()->GetIndirectReference());   
    }
}

PdfObject* PdfPage::GetContents() const 
{ 
    if( !m_pContents ) 
    {
        const_cast<PdfPage*>(this)->CreateContents();
    }

    return m_pContents->GetContents(); 
}

PdfStream & PdfPage::GetStreamForAppending()
{
    if (!m_pContents)
    {
        const_cast<PdfPage*>(this)->CreateContents();
    }

    return m_pContents->GetStreamForAppending();
}

PdfRect PdfPage::CreateStandardPageSize( const EPdfPageSize ePageSize, bool bLandscape )
{
    PdfRect rect;

    switch( ePageSize ) 
    {
        case ePdfPageSize_A0:
            rect.SetWidth( 2384.0 );
            rect.SetHeight( 3370.0 );
            break;

        case ePdfPageSize_A1:
            rect.SetWidth( 1684.0 );
            rect.SetHeight( 2384.0 );
            break;

        case ePdfPageSize_A2:
            rect.SetWidth( 1191.0 );
            rect.SetHeight( 1684.0 );
            break;
            
        case ePdfPageSize_A3:
            rect.SetWidth( 842.0 );
            rect.SetHeight( 1190.0 );
            break;

        case ePdfPageSize_A4:
            rect.SetWidth( 595.0 );
            rect.SetHeight( 842.0 );
            break;

        case ePdfPageSize_A5:
            rect.SetWidth( 420.0 );
            rect.SetHeight( 595.0 );
            break;

        case ePdfPageSize_A6:
            rect.SetWidth( 297.0 );
            rect.SetHeight( 420.0 );
            break;

        case ePdfPageSize_Letter:
            rect.SetWidth( 612.0 );
            rect.SetHeight( 792.0 );
            break;
            
        case ePdfPageSize_Legal:
            rect.SetWidth( 612.0 );
            rect.SetHeight( 1008.0 );
            break;

        case ePdfPageSize_Tabloid:
            rect.SetWidth( 792.0 );
            rect.SetHeight( 1224.0 );
            break;

        default:
            break;
    }

    if( bLandscape ) 
    {
        double dTmp = rect.GetWidth();
        rect.SetWidth ( rect.GetHeight() );
        rect.SetHeight(  dTmp );
    }

    return rect;
}

const PdfObject* PdfPage::GetInheritedKeyFromObject( const char* inKey, const PdfObject* inObject, int depth ) const
{
    const PdfObject* pObj = NULL;

    // check for it in the object itself
    if ( inObject->GetDictionary().HasKey( inKey ) ) 
    {
        pObj = inObject->GetDictionary().GetKey( inKey );
        if ( !pObj->IsNull() ) 
            return pObj;
    }
    
    // if we get here, we need to go check the parent - if there is one!
    if( inObject->GetDictionary().HasKey( "Parent" ) ) 
    {
        // CVE-2017-5852 - prevent stack overflow if Parent chain contains a loop, or is very long
        // e.g. pObj->GetParent() == pObj or pObj->GetParent()->GetParent() == pObj
        // default stack sizes
        // Windows: 1 MB
        // Linux: 2 MB
        // macOS: 8 MB for main thread, 0.5 MB for secondary threads
        // 0.5 MB is enough space for 1000 512 byte stack frames and 2000 256 byte stack frames
        const int maxRecursionDepth = 1000;

        if ( depth > maxRecursionDepth )
            PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );

        pObj = inObject->GetIndirectKey( "Parent" );
        if( pObj == inObject )
        {
            std::ostringstream oss;
            oss << "Object " << inObject->GetIndirectReference().ObjectNumber() << " "
                << inObject->GetIndirectReference().GenerationNumber() << " references itself as Parent";
            PODOFO_RAISE_ERROR_INFO( ePdfError_BrokenFile, oss.str().c_str() );
        }

        if( pObj )
            pObj = GetInheritedKeyFromObject( inKey, pObj, depth + 1 );
    }

    return pObj;
}

const PdfRect PdfPage::GetPageBox( const char* inBox ) const
{
    PdfRect	 pageBox;
    const PdfObject*   pObj;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObj = GetInheritedKeyFromObject( inBox, this->GetObject() );
    

    // Sometime page boxes are defined using reference objects
    while ( pObj && pObj->IsReference() )
    {
        pObj = this->GetObject()->GetOwner()->GetObject( pObj->GetReference() );
    }

    // assign the value of the box from the array
    if ( pObj && pObj->IsArray() )
    {
        pageBox.FromArray( pObj->GetArray() );
    }
    else if ( strcmp( inBox, "ArtBox" ) == 0   ||
              strcmp( inBox, "BleedBox" ) == 0 ||
              strcmp( inBox, "TrimBox" ) == 0  )
    {
        // If those page boxes are not specified then
        // default to CropBox per PDF Spec (3.6.2)
        pageBox = GetPageBox( "CropBox" );
    }
    else if ( strcmp( inBox, "CropBox" ) == 0 )
    {
        // If crop box is not specified then
        // default to MediaBox per PDF Spec (3.6.2)
        pageBox = GetPageBox( "MediaBox" );
    }
    
    return pageBox;
}

int PdfPage::GetRotation() const 
{ 
    int rot = 0;
    
    const PdfObject* pObj = GetInheritedKeyFromObject( "Rotate", this->GetObject() ); 
    if ( pObj && pObj->IsNumber() )
        rot = static_cast<int>(pObj->GetNumber());
    
    return rot;
}

void PdfPage::SetRotation(int nRotation)
{
    if( nRotation != 0 && nRotation != 90 && nRotation != 180 && nRotation != 270 )
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );

    this->GetObject()->GetDictionary().AddKey( "Rotate", PdfVariant(static_cast<int64_t>(nRotation)) );
}

PdfArray * PdfPage::GetAnnotationsArray() const
{
    auto obj = const_cast<PdfPage &>(*this).GetObject()->GetDictionary().FindKey("Annots");
    if (obj == nullptr)
        return nullptr;

    return &obj->GetArray();
}

PdfArray & PdfPage::GetOrCreateAnnotationsArray()
{
    auto &dict = GetObject()->GetDictionary();
    PdfObject* pObj = dict.FindKey("Annots");

    if (pObj == nullptr)
        pObj = &dict.AddKey("Annots", PdfArray());

    return pObj->GetArray();
}

int PdfPage::GetAnnotationCount() const
{
    auto arr = GetAnnotationsArray();
    return arr == nullptr ? 0 : arr->GetSize();
}

PdfAnnotation* PdfPage::CreateAnnotation( EPdfAnnotation eType, const PdfRect & rRect )
{
    PdfAnnotation* pAnnot = new PdfAnnotation( this, eType, rRect, this->GetObject()->GetOwner() );
    PdfReference   ref    = pAnnot->GetObject()->GetIndirectReference();

    auto &arr = GetOrCreateAnnotationsArray();
    arr.push_back( ref );
    m_mapAnnotations[pAnnot->GetObject()] = pAnnot;

    // Default set print flag
    auto flags = pAnnot->GetFlags();
    pAnnot->SetFlags(flags | EPdfAnnotationFlags::ePdfAnnotationFlags_Print);

    return pAnnot;
}

PdfAnnotation* PdfPage::GetAnnotation(int index)
{
    PdfAnnotation* pAnnot;
    PdfReference   ref;

    auto arr = GetAnnotationsArray();

    if (arr == nullptr)
        PODOFO_RAISE_ERROR(ePdfError_InvalidHandle);
    
    if(index < 0 || index >= arr->GetSize())
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    auto &obj = arr->FindAt(index);
    pAnnot = m_mapAnnotations[&obj];
    if (!pAnnot)
    {
        pAnnot = new PdfAnnotation(&obj, this);
        m_mapAnnotations[&obj] = pAnnot;
    }

    return pAnnot;
}

void PdfPage::DeleteAnnotation(int index)
{
    auto arr = GetAnnotationsArray();
    if (arr == nullptr)
        return;

    if (index < 0 || index >= arr->GetSize())
        PODOFO_RAISE_ERROR(ePdfError_ValueOutOfRange);

    auto &pItem = arr->FindAt(index);
    auto found = m_mapAnnotations.find(&pItem);
    if (found != m_mapAnnotations.end())
    {
        delete found->second;
        m_mapAnnotations.erase(found);
    }

    // Delete the PdfObject in the document
    if (pItem.GetIndirectReference().IsIndirect())
        delete pItem.GetOwner()->RemoveObject(pItem.GetIndirectReference());

    // Delete the annotation from the annotation array.
    // Has to be performed at last
    arr->RemoveAt(index);
}

void PdfPage::DeleteAnnotation(PdfObject &annotObj)
{
    PdfArray *arr = GetAnnotationsArray();
    if (arr == nullptr)
        return;

    // find the array iterator pointing to the annotation, so it can be deleted later
    int index = -1;
    for (int i = 0; i < arr->GetSize(); i++)
    {
        // CLEAN-ME: The following is ugly. Fix operator== in PdfOject and PdfVariant and use operator==
        auto &obj = arr->FindAt(i);
        if (&annotObj == &obj)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        // The object was not found as annotation in this page
        return;
    }

    // Delete any cached PdfAnnotations
    auto found = m_mapAnnotations.find((PdfObject *)&annotObj);
    if (found != m_mapAnnotations.end())
    {
        delete found->second;
        m_mapAnnotations.erase(found);
    }

    // Delete the PdfObject in the document
    if (annotObj.GetIndirectReference().IsIndirect())
        delete GetObject()->GetOwner()->RemoveObject(annotObj.GetIndirectReference());
    
    // Delete the annotation from the annotation array.
	// Has to be performed at last
    arr->RemoveAt(index);
}

// added by Petr P. Petrov 21 Febrary 2010
bool PdfPage::SetPageWidth(int newWidth)
{
    PdfObject*   pObjMediaBox;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObjMediaBox = const_cast<PdfObject*>(GetInheritedKeyFromObject( "MediaBox", this->GetObject() ));
    
    // assign the value of the box from the array
    if ( pObjMediaBox && pObjMediaBox->IsArray() )
    {
        auto &mediaBoxArr = pObjMediaBox->GetArray();

        // in PdfRect::FromArray(), the Left value is subtracted from Width
        double dLeftMediaBox = mediaBoxArr[0].GetReal();
        mediaBoxArr[2] = PdfObject( newWidth + dLeftMediaBox );

        PdfObject*   pObjCropBox;

        // Take advantage of inherited values - walking up the tree if necessary
        pObjCropBox = const_cast<PdfObject*>(GetInheritedKeyFromObject( "CropBox", this->GetObject() ));

        if ( pObjCropBox && pObjCropBox->IsArray() )
        {
            auto &cropBoxArr = pObjCropBox->GetArray();
            // in PdfRect::FromArray(), the Left value is subtracted from Width
            double dLeftCropBox = cropBoxArr[0].GetReal();
            cropBoxArr[2] = PdfObject( newWidth + dLeftCropBox );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

// added by Petr P. Petrov 21 Febrary 2010
bool PdfPage::SetPageHeight(int newHeight)
{
    PdfObject*   pObj;
        
    // Take advantage of inherited values - walking up the tree if necessary
    pObj = const_cast<PdfObject*>(GetInheritedKeyFromObject( "MediaBox", this->GetObject() ));
    
    // assign the value of the box from the array
    if ( pObj && pObj->IsArray() )
    {
        auto &mediaBoxArr = pObj->GetArray();
        // in PdfRect::FromArray(), the Bottom value is subtracted from Height
        double dBottom = mediaBoxArr[1].GetReal();
        mediaBoxArr[3] = PdfObject(newHeight + dBottom);

        PdfObject*   pObjCropBox;

        // Take advantage of inherited values - walking up the tree if necessary
        pObjCropBox = const_cast<PdfObject*>(GetInheritedKeyFromObject( "CropBox", this->GetObject() ));

        if ( pObjCropBox && pObjCropBox->IsArray() )
        {
            auto &cropBoxArr = pObjCropBox->GetArray();
            // in PdfRect::FromArray(), the Bottom value is subtracted from Height
            double dBottomCropBox = cropBoxArr[1].GetReal();
            cropBoxArr[3] = PdfObject( newHeight + dBottomCropBox );
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void PdfPage::SetMediaBox(const PdfRect & rSize)
{
    PdfVariant mediaBox;
    rSize.ToVariant(mediaBox);
    this->GetObject()->GetDictionary().AddKey("MediaBox", mediaBox);
}

void PdfPage::SetTrimBox( const PdfRect & rSize )
{
    PdfVariant trimbox;
    rSize.ToVariant( trimbox );
    this->GetObject()->GetDictionary().AddKey( "TrimBox", trimbox );
}

int PdfPage::GetPageNumber() const
{
    int nPageNumber = 0;
    PdfObject*          pParent     = this->GetObject()->GetIndirectKey( "Parent" );
    PdfReference ref                = this->GetObject()->GetIndirectReference();

    // CVE-2017-5852 - prevent infinite loop if Parent chain contains a loop
    // e.g. pParent->GetIndirectKey( "Parent" ) == pParent or pParent->GetIndirectKey( "Parent" )->GetIndirectKey( "Parent" ) == pParent
    const int maxRecursionDepth = 1000;
    int depth = 0;

    while( pParent ) 
    {
        PdfObject* pKids = pParent->GetIndirectKey( "Kids" );
        if ( pKids != NULL )
        {
            const PdfArray& kids        = pKids->GetArray();
            PdfArray::const_iterator it = kids.begin();

            while( it != kids.end() && (*it).GetReference() != ref )
            {
                PdfObject* pNode = this->GetObject()->GetOwner()->GetObject( (*it).GetReference() );
                if (!pNode)
                {
                    std::ostringstream oss;
                    oss << "Object " << (*it).GetReference().ToString() << " not found from Kids array "
                        << pKids->GetIndirectReference().ToString(); 
                    PODOFO_RAISE_ERROR_INFO( ePdfError_NoObject, oss.str() );
                }

                if( pNode->GetDictionary().GetKey( PdfName::KeyType ) != NULL 
                    && pNode->GetDictionary().GetKey( PdfName::KeyType )->GetName() == PdfName( "Pages" ) )
                {
                    PdfObject* pCount = pNode->GetIndirectKey( "Count" );
                    if( pCount != NULL ) {
                        nPageNumber += static_cast<int>(pCount->GetNumber());
                    }
                } else {
                    // if we do not have a page tree node, 
                    // we most likely have a page object:
                    // so the page count is 1
                    ++nPageNumber;
                }
                ++it;
            }
        }

        ref     = pParent->GetIndirectReference();
        pParent = pParent->GetIndirectKey( "Parent" );
        ++depth;

        if ( depth > maxRecursionDepth )
        {
            PODOFO_RAISE_ERROR_INFO( ePdfError_BrokenFile, "Loop in Parent chain" );
        }
    }

    return ++nPageNumber;
}

PdfObject* PdfPage::GetFromResources( const PdfName & rType, const PdfName & rKey )
{
    if( m_pResources == NULL ) // Fix CVE-2017-7381
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidHandle, "No Resources" );
    } 
    if( m_pResources->GetDictionary().HasKey( rType ) ) 
    {
        // OC 15.08.2010 BugFix: Ghostscript creates here sometimes an indirect reference to a directory
     // PdfObject* pType = m_pResources->GetDictionary().GetKey( rType );
        PdfObject* pType = m_pResources->GetIndirectKey( rType );
        if( pType && pType->IsDictionary() && pType->GetDictionary().HasKey( rKey ) )
        {
            PdfObject* pObj = pType->GetDictionary().GetKey( rKey ); // CB 08.12.2017 Can be an array
            if (pObj->IsReference())
            {
                const PdfReference & ref = pType->GetDictionary().GetKey( rKey )->GetReference();
                return this->GetObject()->GetOwner()->GetObject( ref );
            }
            return pObj; // END
        }
    }
    
    return NULL;
}

void PdfPage::SetICCProfile( const char *pszCSTag, PdfInputStream *pStream, int64_t nColorComponents, EPdfColorSpace eAlternateColorSpace )
{
    // Check nColorComponents for a valid value
    if ( nColorComponents != 1 &&
         nColorComponents != 3 &&
         nColorComponents != 4 )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_ValueOutOfRange, "SetICCProfile nColorComponents must be 1, 3 or 4!" );
    }

    // Create a colorspace object
    PdfObject* iccObject = this->GetObject()->GetOwner()->CreateObject();
    PdfName nameForCS = PdfColor::GetNameForColorSpace( eAlternateColorSpace );
    iccObject->GetDictionary().AddKey( PdfName("Alternate"), nameForCS );
    iccObject->GetDictionary().AddKey( PdfName("N"), nColorComponents );
    iccObject->GetOrCreateStream().Set( pStream );

    // Add the colorspace
    PdfArray array;
    array.push_back( PdfName("ICCBased") );
    array.push_back( iccObject->GetIndirectReference() );

    PoDoFo::PdfDictionary iccBasedDictionary;
    iccBasedDictionary.AddKey( PdfName(pszCSTag), array );

    // Add the colorspace to resource
    GetResources()->GetDictionary().AddKey( PdfName("ColorSpace"), iccBasedDictionary );
}


};
