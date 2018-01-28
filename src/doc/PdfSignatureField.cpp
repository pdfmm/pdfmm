/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                      by Petr Pytelka                                    *
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
#include "../base/PdfDictionary.h"
#include "../base/PdfData.h"

#include "PdfXObject.h"

#include <string.h>

namespace PoDoFo {

PdfSignatureField::PdfSignatureField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
	:PdfField(PoDoFo::ePdfField_Signature, pPage, rRect, pDoc)
{
    m_pSignatureObj = NULL;
    Init();
}

PdfSignatureField::PdfSignatureField( PdfAnnotation* pWidget, PdfAcroForm* pParent, PdfDocument* pDoc, bool bInit )
	:PdfField(PoDoFo::ePdfField_Signature, pWidget,  pParent, pDoc)
{
    m_pSignatureObj = NULL;
    if( bInit )
        Init();
}

PdfSignatureField::PdfSignatureField( PdfAnnotation* pWidget )
	:PdfField( pWidget->GetObject(), pWidget )
{
    m_pSignatureObj = NULL;

    // do not call Init() here
    if( this->GetFieldObject()->GetDictionary().HasKey( "V" ) )
    {
        m_pSignatureObj = this->GetFieldObject()->GetOwner()->GetObject( this->GetFieldObject()->GetDictionary().GetKey( "V" )->GetReference() );
    }
}

void PdfSignatureField::SetAppearanceStream( PdfXObject* pObject, EPdfAnnotationAppearance eAppearance, const PdfName & state )
{
    if( !pObject )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    SetAppearanceStreamForObject( m_pObject, pObject, eAppearance, state );
    
    this->GetAppearanceCharacteristics( true );
}

void PdfSignatureField::Init()
{
    m_pSignatureObj = NULL;

    EnsureSignatureObject ();
}

void PdfSignatureField::SetSignatureReason(const PdfString & rsText)
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
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
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName("M")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("M"));
    }
	PdfString sDate;
	sigDate.ToString(sDate);
    m_pSignatureObj->GetDictionary().AddKey(PdfName("M"), sDate);
}

void PdfSignatureField::SetSignature(const PdfData &sSignatureData)
{
    // Prepare source data
    size_t lSigLen = sSignatureData.data().size();
    char* pData = static_cast<char*>(podofo_malloc( lSigLen + 2 ));
    if (!pData)
    {
        PODOFO_RAISE_ERROR(ePdfError_OutOfMemory);
    }

    pData[0] = '<';
    pData[lSigLen + 1] = '>';
    memcpy(pData + 1, sSignatureData.data().c_str(), lSigLen);
    PdfData signatureData(pData, lSigLen + 2);
    podofo_free(pData);
    // Content of the signature
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    // Remove old data
    if(m_pSignatureObj->GetDictionary().HasKey("ByteRange"))
    {
        m_pSignatureObj->GetDictionary().RemoveKey("ByteRange");
    }
    if(m_pSignatureObj->GetDictionary().HasKey(PdfName::KeyContents))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName::KeyContents);
    }	

    // Byte range
    PdfData rangeData("[ 0 1234567890 1234567890 1234567890]");
    m_pSignatureObj->GetDictionary().AddKey("ByteRange", PdfVariant(rangeData) );

    m_pSignatureObj->GetDictionary().AddKey(PdfName::KeyContents, PdfVariant(signatureData) );
}

void PdfSignatureField::SetSignatureLocation( const PdfString & rsText )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
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
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
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
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }

    if (m_pSignatureObj->GetDictionary().HasKey(PdfName("Reference")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Reference"));
    }

    PdfObject *pSigRef = this->GetFieldObject()->GetOwner()->CreateObject( "SigRef" );
    pSigRef->GetDictionary().AddKey(PdfName("TransformMethod"), PdfName("DocMDP"));

    PdfObject *pTransParams = this->GetFieldObject()->GetOwner()->CreateObject( "TransformParams" );
    pTransParams->GetDictionary().AddKey(PdfName("V"), PdfName("1.2"));
    pTransParams->GetDictionary().AddKey(PdfName("P"), PdfVariant((pdf_int64)perm));
    pSigRef->GetDictionary().AddKey(PdfName("TransformParams"), pTransParams);

    if (pDocumentCatalog != NULL)
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

PdfObject* PdfSignatureField::GetSignatureObject( void ) const
{
    return m_pSignatureObj;
}

void PdfSignatureField::EnsureSignatureObject( void )
{
    if( m_pSignatureObj )
        return;

    m_pSignatureObj = this->GetFieldObject()->GetOwner()->CreateObject( "Sig" );
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( ePdfError_InvalidHandle );
    }
    GetFieldObject()->GetDictionary().AddKey( "V" , m_pSignatureObj->Reference() );

    PdfDictionary &dict = m_pSignatureObj->GetDictionary();

    dict.AddKey( PdfName::KeyFilter, PdfName( "Adobe.PPKLite" ) );
    dict.AddKey( "SubFilter", PdfName( "adbe.pkcs7.detached" ) );
}

}
