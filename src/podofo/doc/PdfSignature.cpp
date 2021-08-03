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
    m_pSignatureObj = this->GetObject().GetDictionary().FindKey("V");
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

void PdfSignature::SetSignerName(const PdfString& rsText)
{
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_pSignatureObj->GetDictionary().AddKey("Name", rsText);
}

void PdfSignature::SetSignatureReason(const PdfString& rsText)
{
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_pSignatureObj->GetDictionary().AddKey("Reason", rsText);
}

void PdfSignature::SetSignatureDate(const PdfDate& sigDate)
{
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    PdfString sDate = sigDate.ToString();
    m_pSignatureObj->GetDictionary().AddKey("M", sDate);
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
    m_pSignatureObj->GetDictionary().AddKey(PdfName::KeyContents, PdfObject(contentsData));

    // Prepare byte range data
    PdfData byteRangeData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    m_pSignatureObj->GetDictionary().AddKey("ByteRange", PdfObject(byteRangeData));
}

void PdfSignature::SetSignatureLocation(const PdfString& rsText)
{
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_pSignatureObj->GetDictionary().AddKey("Location", rsText);
}

void PdfSignature::SetSignatureCreator(const PdfName& creator)
{
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    if (m_pSignatureObj->GetDictionary().HasKey("Prop_Build"))
    {
        PdfObject* propBuild = m_pSignatureObj->GetDictionary().GetKey("Prop_Build");
        if (propBuild->GetDictionary().HasKey("App"))
        {
            PdfObject* app = propBuild->GetDictionary().GetKey("App");
            app->GetDictionary().RemoveKey("Name");
            propBuild->GetDictionary().RemoveKey("App");
        }

        m_pSignatureObj->GetDictionary().RemoveKey("Prop_Build");
    }

    m_pSignatureObj->GetDictionary().AddKey("Prop_Build", PdfDictionary());
    PdfObject* propBuild = m_pSignatureObj->GetDictionary().GetKey("Prop_Build");
    propBuild->GetDictionary().AddKey("App", PdfDictionary());
    PdfObject* app = propBuild->GetDictionary().GetKey("App");
    app->GetDictionary().AddKey("Name", creator);
}

void PdfSignature::AddCertificationReference(PdfObject* pDocumentCatalog, EPdfCertPermission perm)
{
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_pSignatureObj->GetDictionary().RemoveKey("Reference");

    PdfObject* pSigRef = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("SigRef");
    pSigRef->GetDictionary().AddKey("TransformMethod", PdfName("DocMDP"));

    PdfObject* pTransParams = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("TransformParams");
    pTransParams->GetDictionary().AddKey("V", PdfName("1.2"));
    pTransParams->GetDictionary().AddKey("P", PdfObject((int64_t)perm));
    pSigRef->GetDictionary().AddKey("TransformParams", *pTransParams);

    if (pDocumentCatalog != nullptr)
    {
        PdfObject permObject;
        permObject.GetDictionary().AddKey("DocMDP", this->GetObject().GetDictionary().GetKey("V")->GetReference());
        pDocumentCatalog->GetDictionary().AddKey("Perms", permObject);
    }

    PdfArray refers;
    refers.push_back(*pSigRef);

    m_pSignatureObj->GetDictionary().AddKey("Reference", PdfVariant(refers));
}

const PdfObject* PdfSignature::GetSignatureReason() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Reason");
}

const PdfObject* PdfSignature::GetSignatureLocation() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("Location");
}

const PdfObject* PdfSignature::GetSignatureDate() const
{
    if (m_pSignatureObj == nullptr)
        return nullptr;

    return m_pSignatureObj->GetDictionary().GetKey("M");
}

const PdfObject* PdfSignature::GetSignerName() const
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
    if (m_pSignatureObj != nullptr)
        return;

    m_pSignatureObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("Sig");
    if (m_pSignatureObj == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    GetObject().GetDictionary().AddKey("V", m_pSignatureObj->GetIndirectReference());
}

PdfSignatureBeacons::PdfSignatureBeacons()
{
    ContentsOffset = std::make_shared<size_t>();
    ByteRangeOffset = std::make_shared<size_t>();
}
