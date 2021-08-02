/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2011 by Petr Pytelka
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfSignature.h"

#include <doc/PdfDocument.h>
#include "../base/PdfDictionary.h"
#include "../base/PdfData.h"

#include "PdfDocument.h"
#include "PdfXObject.h"
#include "PdfPage.h"

using namespace std;
using namespace PoDoFo;

PdfSignature::PdfSignature(PdfPage& page, const PdfRect& rect)
    : PdfField(PdfFieldType::Signature, page, rect), m_pSignatureObj(nullptr)
{

    Init(*page.GetDocument().GetAcroForm());
}

PdfSignature::PdfSignature(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfField(PdfFieldType::Signature, doc, widget, insertInAcroform), m_pSignatureObj(nullptr)
{
    Init(*doc.GetAcroForm());
}

PdfSignature::PdfSignature(PdfObject& obj, PdfAnnotation* widget)
    : PdfField(PdfFieldType::Signature, obj, widget), m_pSignatureObj(nullptr)
{
    // do not call Init() here
    if (this->GetObject().GetDictionary().HasKey("V"))
    {
        m_pSignatureObj = this->GetObject().GetDocument()->GetObjects().GetObject(this->GetObject().GetDictionary().GetKey("V")->GetReference());
    }
}

void PdfSignature::SetAppearanceStream(PdfXObject& pObject, PdfAnnotationAppearance eAppearance, const PdfName& state)
{
    GetWidgetAnnotation()->SetAppearanceStream(pObject, eAppearance, state);
    this->GetAppearanceCharacteristics(true);
}

void PdfSignature::Init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    acroForm.GetObject().GetDictionary().AddKey("SigFlags", PdfObject((int64_t)3));
}

void PdfSignature::SetSignerName(const PdfString & rsText)
{
    if (!m_pSignatureObj)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    m_pSignatureObj->GetDictionary().AddKey(PdfName("Name"), rsText);
}

void PdfSignature::SetSignatureReason(const PdfString & rsText)
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

void PdfSignature::SetSignatureDate(const PdfDate &sigDate)
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

void PdfSignature::PrepareForSigning(const string_view& filter,
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

void PdfSignature::SetSignatureLocation( const PdfString & rsText )
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

void PdfSignature::SetSignatureCreator( const PdfName & creator )
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

void PdfSignature::AddCertificationReference( PdfObject* pDocumentCatalog, EPdfCertPermission perm )
{
    if( !m_pSignatureObj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if (m_pSignatureObj->GetDictionary().HasKey(PdfName("Reference")))
    {
        m_pSignatureObj->GetDictionary().RemoveKey(PdfName("Reference"));
    }

    PdfObject *pSigRef = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject( "SigRef" );
    pSigRef->GetDictionary().AddKey(PdfName("TransformMethod"), PdfName("DocMDP"));

    PdfObject *pTransParams = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject( "TransformParams" );
    pTransParams->GetDictionary().AddKey(PdfName("V"), PdfName("1.2"));
    pTransParams->GetDictionary().AddKey(PdfName("P"), PdfVariant((int64_t)perm));
    pSigRef->GetDictionary().AddKey(PdfName("TransformParams"), *pTransParams);

    if (pDocumentCatalog != nullptr)
    {
        PdfObject permObject;
        permObject.GetDictionary().AddKey("DocMDP", this->GetObject().GetDictionary().GetKey("V")->GetReference());

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

const PdfObject * PdfSignature::GetSignatureReason() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Reason");
}

const PdfObject * PdfSignature::GetSignatureLocation() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Location");
}

const PdfObject * PdfSignature::GetSignatureDate() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("M");
}

const PdfObject * PdfSignature::GetSignerName() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Name");
}

PdfObject* PdfSignature::GetSignatureObject() const
{
    return m_pSignatureObj;
}

void PdfSignature::EnsureSignatureObject()
{
    if( m_pSignatureObj != nullptr )
        return;

    m_pSignatureObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject( "Sig" );
    if( !m_pSignatureObj )
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    GetObject().GetDictionary().AddKey( "V" , m_pSignatureObj->GetIndirectReference() );
}

PdfSignatureBeacons::PdfSignatureBeacons()
{
    ContentsOffset = std::make_shared<size_t>();
    ByteRangeOffset = std::make_shared<size_t>();
}
