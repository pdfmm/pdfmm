/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2011 by Petr Pytelka
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfSignature.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfData.h"

#include "PdfDocument.h"
#include "PdfXObject.h"
#include "PdfPage.h"

using namespace std;
using namespace mm;

PdfSignature::PdfSignature(PdfPage& page, const PdfRect& rect)
    : PdfField(PdfFieldType::Signature, page, rect), m_ValueObj(nullptr)
{

    Init(page.GetDocument().GetOrCreateAcroForm());
}

PdfSignature::PdfSignature(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfField(PdfFieldType::Signature, doc, widget, insertInAcroform), m_ValueObj(nullptr)
{
    Init(doc.GetOrCreateAcroForm());
}

PdfSignature::PdfSignature(PdfObject& obj, PdfAnnotation* widget)
    : PdfField(PdfFieldType::Signature, obj, widget), m_ValueObj(nullptr)
{
    // do not call Init() here
    m_ValueObj = this->GetObject().GetDictionary().FindKey("V");
}

void PdfSignature::SetAppearanceStream(PdfXObjectForm& obj, PdfAnnotationAppearance appearance, const PdfName& state)
{
    GetWidgetAnnotation()->SetAppearanceStream(obj, appearance, state);
    (void)this->GetOrCreateAppearanceCharacteristics();
}

void PdfSignature::Init(PdfAcroForm& acroForm)
{
    // TABLE 8.68 Signature flags: SignaturesExist (1) | AppendOnly (2)
    // This will open signature panel when inspecting PDF with acrobat,
    // even if the signature is unsigned
    acroForm.GetObject().GetDictionary().AddKey("SigFlags", PdfObject((int64_t)3));
}

void PdfSignature::SetSignerName(const PdfString& text)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().AddKey("Name", text);
}

void PdfSignature::SetSignatureReason(const PdfString& text)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().AddKey("Reason", text);
}

void PdfSignature::SetSignatureDate(const PdfDate& sigDate)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfString dateStr = sigDate.ToString();
    m_ValueObj->GetDictionary().AddKey("M", dateStr);
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
    m_ValueObj->GetDictionary().AddKey(PdfName::KeyContents, PdfObject(contentsData));

    // Prepare byte range data
    PdfData byteRangeData(beacons.ByteRangeBeacon, beacons.ByteRangeOffset);
    m_ValueObj->GetDictionary().AddKey("ByteRange", PdfObject(byteRangeData));
}

void PdfSignature::SetSignatureLocation(const PdfString& text)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().AddKey("Location", text);
}

void PdfSignature::SetSignatureCreator(const PdfName& creator)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    if (m_ValueObj->GetDictionary().HasKey("Prop_Build"))
    {
        PdfObject* propBuild = m_ValueObj->GetDictionary().GetKey("Prop_Build");
        if (propBuild->GetDictionary().HasKey("App"))
        {
            PdfObject* app = propBuild->GetDictionary().GetKey("App");
            app->GetDictionary().RemoveKey("Name");
            propBuild->GetDictionary().RemoveKey("App");
        }

        m_ValueObj->GetDictionary().RemoveKey("Prop_Build");
    }

    m_ValueObj->GetDictionary().AddKey("Prop_Build", PdfDictionary());
    PdfObject* propBuild = m_ValueObj->GetDictionary().GetKey("Prop_Build");
    propBuild->GetDictionary().AddKey("App", PdfDictionary());
    PdfObject* app = propBuild->GetDictionary().GetKey("App");
    app->GetDictionary().AddKey("Name", creator);
}

void PdfSignature::AddCertificationReference(PdfCertPermission perm)
{
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    m_ValueObj->GetDictionary().RemoveKey("Reference");

    auto sigRef = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("SigRef");
    sigRef->GetDictionary().AddKey("TransformMethod", PdfName("DocMDP"));

    auto transParams = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("TransformParams");
    transParams->GetDictionary().AddKey("V", PdfName("1.2"));
    transParams->GetDictionary().AddKey("P", PdfObject((int64_t)perm));
    sigRef->GetDictionary().AddKey("TransformParams", *transParams);

    auto& catalog = GetObject().GetDocument()->GetCatalog();
    PdfObject permObject;
    permObject.GetDictionary().AddKey("DocMDP", this->GetObject().GetDictionary().GetKey("V")->GetReference());
    catalog.GetDictionary().AddKey("Perms", permObject);

    PdfArray refers;
    refers.push_back(*sigRef);

    m_ValueObj->GetDictionary().AddKey("Reference", PdfVariant(refers));
}

const PdfObject* PdfSignature::GetSignatureReason() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("Reason");
}

const PdfObject* PdfSignature::GetSignatureLocation() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("Location");
}

const PdfObject* PdfSignature::GetSignatureDate() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("M");
}

const PdfObject* PdfSignature::GetSignerName() const
{
    if (m_ValueObj == nullptr)
        return nullptr;

    return m_ValueObj->GetDictionary().GetKey("Name");
}

PdfObject* PdfSignature::GetSignatureObject() const
{
    return m_ValueObj;
}

void PdfSignature::EnsureSignatureObject()
{
    if (m_ValueObj != nullptr)
        return;

    m_ValueObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("Sig");
    if (m_ValueObj == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    GetObject().GetDictionary().AddKey("V", m_ValueObj->GetIndirectReference());
}

PdfSignatureBeacons::PdfSignatureBeacons()
{
    ContentsOffset = std::make_shared<size_t>();
    ByteRangeOffset = std::make_shared<size_t>();
}
