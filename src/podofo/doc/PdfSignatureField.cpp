/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                      by Petr Pytelka                                    *
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

#include "PdfSignatureField.h"

#include <doc/PdfDocument.h>
#include "../base/PdfDictionary.h"
#include "../base/PdfData.h"

#include "PdfDocument.h"
#include "PdfXObject.h"
#include "PdfPage.h"

#include <string.h>

using namespace std;
using namespace PoDoFo;

PdfSignatureField::PdfSignatureField( PdfPage* pPage, const PdfRect & rRect)
	:PdfField(EPdfField::Signature, pPage, rRect), m_pSignatureObj(nullptr)
{

    Init(*pPage->GetDocument().GetAcroForm());
}

PdfSignatureField::PdfSignatureField( PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
	: PdfField(EPdfField::Signature, pWidget, pDoc, insertInAcroform), m_pSignatureObj(nullptr)
{
    Init(*pDoc.GetAcroForm());
}

PdfSignatureField::PdfSignatureField( PdfObject* pObject, PdfAnnotation* pWidget )
	: PdfField(EPdfField::Signature, pObject, pWidget ), m_pSignatureObj(nullptr)
{
    // do not call Init() here
    if( this->GetFieldObject()->GetDictionary().HasKey( "V" ) )
    {
        m_pSignatureObj = this->GetFieldObject()->GetDocument()->GetObjects().GetObject( this->GetFieldObject()->GetDictionary().GetKey( "V" )->GetReference() );
    }
}

void PdfSignatureField::SetAppearanceStream( PdfXObject* pObject, EPdfAnnotationAppearance eAppearance, const PdfName & state )
{
    if( !pObject )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    GetWidgetAnnotation()->SetAppearanceStream( pObject, eAppearance, state );
    
    this->GetAppearanceCharacteristics( true );
}

void PdfSignatureField::Init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    acroForm.GetObject()->GetDictionary().AddKey("SigFlags", PdfObject((int64_t)3));
}

void PdfSignatureField::SetSignerName(const PdfString & rsText)
{
    if (!m_pSignatureObj)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    m_pSignatureObj->GetDictionary().AddKey(PdfName("Name"), rsText);
}

void PdfSignatureField::SetSignatureReason(const PdfString & rsText)
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("Reason")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Reason"));
    }
    m_pSignatureObj->GetDictionary().AddKey(PdfName("Reason"), rsText);
}

void PdfSignatureField::SetSignatureDate(const PdfDate &sigDate)
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("M")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("M"));
    }
	PdfString sDate = sigDate.ToString();
    m_pSignatureObj->GetDictionary().AddKey(PdfName("M"), sDate);
}

void PdfSignatureField::PrepareForSigning(const string_view& filter,
    const string_view& subFilter, const std::string_view& type,
    const PdfSignatureBeacons& beacons)
{
    EnsureSignatureObject();
    auto& dict = GetSignatureObject()->GetDictionary();
    // This must be ensured before any signing operation
    dict.AddKey(PdfName::KeyFilter, PdfName(filter));
    dict.AddKey("SubFilter", PdfName(subFilter));
    dict.AddKey(PdfName::KeyType, PdfName(type));

    // Prepare contents data
    PdfData contentsData(beacons.ContentsBeacon, beacons.ContentsOffset);
    m_pSignatureObj->GetDictionary().AddKey(PdfName::KeyContents, PdfVariant(contentsData));

    // Prepare byte range data
    PdfData byteRangeData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    m_pSignatureObj->GetDictionary().AddKey("ByteRange", PdfVariant(byteRangeData) );
}

void PdfSignatureField::SetSignatureLocation( const PdfString & rsText )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("Location")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Location"));
    }
    m_pSignatureObj->GetDictionary().AddKey(PdfName("Location"), rsText);
}

void PdfSignatureField::SetSignatureCreator( const PdfName & creator )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if( m_pSignatureObj->GetDictionary().HasKey( PdfName( "Prop_Build" ) ) )
    {
        PdfObject* propBuild = m_pSignatureObj->GetDictionary().GetKey( PdfName( "Prop_Build" ) );
        if( propBuild->GetDictionary().HasKey( PdfName( "App" ) ) )
        {
            PdfObject* app = propBuild->GetDictionary().GetKey( PdfName( "App" ) );
            if( app->GetDictionary().HasKey( PdfName( "Name" ) ) )
            {
                app->GetDictionary().RemoveKey( PdfName( "Name" ) );
            }

            propBuild->GetDictionary().RemoveKey( PdfName("App") );
        }

        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Prop_Build"));
    }

    m_pSignatureObj->GetDictionary().AddKey( PdfName( "Prop_Build" ), PdfDictionary() );
    PdfObject* propBuild = m_pSignatureObj->GetDictionary().GetKey( PdfName( "Prop_Build" ) );
    propBuild->GetDictionary().AddKey( PdfName( "App" ), PdfDictionary() );
    PdfObject* app = propBuild->GetDictionary().GetKey( PdfName( "App" ) );
    app->GetDictionary().AddKey( PdfName( "Name" ), creator );
}

void PdfSignatureField::AddCertificationReference( PdfObject* pDocumentCatalog, EPdfCertPermission perm )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if (m_pSignatureObj->GetDictionary().HasKey(PdfName("Reference")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Reference"));
    }

    PdfObject *pSigRef = this->GetFieldObject()->GetDocument()->GetObjects().CreateDictionaryObject( "SigRef" );
    pSigRef->GetDictionary().AddKey(PdfName("TransformMethod"), PdfName("DocMDP"));

    PdfObject *pTransParams = this->GetFieldObject()->GetDocument()->GetObjects().CreateDictionaryObject( "TransformParams" );
    pTransParams->GetDictionary().AddKey(PdfName("V"), PdfName("1.2"));
    pTransParams->GetDictionary().AddKey(PdfName("P"), PdfVariant((int64_t)perm));
    pSigRef->GetDictionary().AddKey(PdfName("TransformParams"), pTransParams);

    if (pDocumentCatalog != nullptr)
    {
        PdfObject permObject;
        permObject.GetDictionary().AddKey("DocMDP", this->GetFieldObject()->GetDictionary().GetKey("V")->GetReference());

        if (pDocumentCatalog->GetDictionary().HasKey(PdfName("Perms")))
        {
            pDocumentCatalog->GetDictionary().RemoveKey(PdfName("Perms"));
        }

        pDocumentCatalog->GetDictionary().AddKey(PdfName("Perms"), permObject);
    }

    PdfArray refers;
    refers.push_back(*pSigRef);

    m_pSignatureObj->GetDictionary().AddKey(PdfName("Reference"), PdfVariant(refers));
}

const PdfObject * PdfSignatureField::GetSignatureReason() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Reason");
}

const PdfObject * PdfSignatureField::GetSignatureLocation() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Location");
}

const PdfObject * PdfSignatureField::GetSignatureDate() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("M");
}

const PdfObject * PdfSignatureField::GetSignerName() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Name");
}

PdfObject* PdfSignatureField::GetSignatureObject( void ) const
{
    return m_pSignatureObj;
}

void PdfSignatureField::EnsureSignatureObject( void )
{
    if( m_pSignatureObj != nullptr )
        return;

    m_pSignatureObj = this->GetFieldObject()->GetDocument()->GetObjects().CreateDictionaryObject( "Sig" );
    if( !m_pSignatureObj )
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    GetFieldObject()->GetDictionary().AddKey( "V" , m_pSignatureObj->GetIndirectReference() );
}

PdfSignatureBeacons::PdfSignatureBeacons()
{
    ContentsOffset = std::make_shared<size_t>();
    ByteRangeOffset = std::make_shared<size_t>();
}
