/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfObject.h"

#include <doc/PdfDocument.h>
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfFileStream.h"
#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfMemStream.h"

#include <sstream>
#include <fstream>

using namespace std;
using namespace PoDoFo;

PdfObject::PdfObject()
    : PdfObject(PdfDictionary(), false) { }

PdfObject::~PdfObject()
{
}

PdfObject::PdfObject(const PdfVariant& var)
    : PdfObject(var, false) { }

PdfObject::PdfObject(bool b)
    : PdfObject(b, false) { }

PdfObject::PdfObject(int64_t l)
    : PdfObject(l, false) { }

PdfObject::PdfObject(double d)
    : PdfObject(d, false) { }

PdfObject::PdfObject(const PdfString& str)
    : PdfObject(str, false) { }

PdfObject::PdfObject(const PdfName& name)
    : PdfObject(name, false) { }

PdfObject::PdfObject(const PdfReference& ref)
    : PdfObject(ref, false) { }

PdfObject::PdfObject(const PdfArray& arr)
    : PdfObject(arr, false) { }

PdfObject::PdfObject(const PdfDictionary& dict)
    : PdfObject(dict, false) { }

// NOTE: Don't copy parent document/container. Copied objects must be
// always detached. Ownership will be set automatically elsewhere.
// Also don't copy reference
PdfObject::PdfObject(const PdfObject& rhs)
    : PdfObject(rhs, false)
{
    copyFrom(rhs);
}

// NOTE: Dirty objects are those who are supposed to be serialized
// or deserialized.
PdfObject::PdfObject(const PdfVariant& var, bool isDirty)
    : m_Variant(var), m_IsDirty(isDirty)
{
    InitPdfObject();
}

void PdfObject::ForceCreateStream()
{
    DelayedLoadStream();
    forceCreateStream();
}

void PdfObject::SetDocument(PdfDocument& document)
{
    if (m_Document == &document)
    {
        // The inner document for variant data objects is guaranteed to be same
        return;
    }

    m_Document = &document;
    SetVariantOwner();
}

void PdfObject::DelayedLoad() const
{
    if (m_DelayedLoadDone)
        return;

    const_cast<PdfObject&>(*this).DelayedLoadImpl();
    m_DelayedLoadDone = true;
    const_cast<PdfObject&>(*this).SetVariantOwner();
}

void PdfObject::DelayedLoadImpl()
{
    // Default implementation of virtual void DelayedLoadImpl() throws, since delayed
    // loading should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

void PdfObject::SetVariantOwner()
{
    auto dataType = m_Variant.GetDataType();
    switch (dataType)
    {
        case EPdfDataType::Dictionary:
            static_cast<PdfContainerDataType&>(m_Variant.GetDictionary()).SetOwner(this);
            break;
        case EPdfDataType::Array:
            static_cast<PdfContainerDataType&>(m_Variant.GetArray()).SetOwner(this);
            break;
        default:
            break;
    }
}

void PdfObject::FreeStream()
{
    m_Stream = nullptr;
}

void PdfObject::InitPdfObject()
{
    m_Document = nullptr;
    m_Parent = nullptr;
    m_IsImmutable = false;
    m_DelayedLoadDone = true;
    m_DelayedLoadStreamDone = true;
    m_Stream = nullptr;
    SetVariantOwner();
}

void PdfObject::Write(PdfOutputDevice& device, PdfWriteMode writeMode,
    PdfEncrypt* encrypt) const
{
    DelayedLoad();
    DelayedLoadStream();

    if (m_IndirectReference.IsIndirect())
    {
        // CHECK-ME We want to make this in all the cases for PDF/A Compatibility
        //if( (writeMode & EPdfWriteMode::Clean) == EPdfWriteMode::Clean )
        {
            device.Print("%i %i obj\n", m_IndirectReference.ObjectNumber(), m_IndirectReference.GenerationNumber());
        }
        //else
        //{
        //    device.Print( "%i %i obj", m_reference.ObjectNumber(), m_reference.GenerationNumber() );
        //}
    }

    if (encrypt)
        encrypt->SetCurrentReference(m_IndirectReference);

    if (m_Stream != nullptr)
    {
        // Set length if it is a key
        PdfFileStream* pFileStream = dynamic_cast<PdfFileStream*>(m_Stream.get());
        if (pFileStream == nullptr)
        {
            // It's not a PdfFileStream. PdfFileStream handles length internally

            size_t length = m_Stream->GetLength();
            if (encrypt != nullptr)
                length = encrypt->CalculateStreamLength(length);

            // Add the key without triggering SetDirty
            const_cast<PdfObject&>(*this).m_Variant.GetDictionary()
                .addKey(PdfName::KeyLength, PdfObject(static_cast<int64_t>(length)), true);
        }
    }

    m_Variant.Write(device, writeMode, encrypt);
    device.Print("\n");

    if (m_Stream)
        m_Stream->Write(device, encrypt);

    if (m_IndirectReference.IsIndirect())
        device.Print("endobj\n");

    // After write we ca reset the dirty flag
    const_cast<PdfObject&>(*this).ResetDirty();
}

size_t PdfObject::GetObjectLength(PdfWriteMode writeMode)
{
    PdfOutputDevice device;

    this->Write(device, writeMode, nullptr);

    return device.GetLength();
}

PdfStream& PdfObject::GetOrCreateStream()
{
    DelayedLoadStream();
    return getOrCreateStream();
}

const PdfStream& PdfObject::GetStream() const
{
    DelayedLoadStream();
    if (m_Stream == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "The object doesn't have a stream");

    return *m_Stream.get();
}

PdfStream& PdfObject::GetStream()
{
    DelayedLoadStream();
    if (m_Stream == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "The object doesn't have a stream");

    return *m_Stream.get();
}

bool PdfObject::TryGetStream(PdfStream*& stream)
{
    DelayedLoadStream();
    if (m_Stream == nullptr)
    {
        stream = nullptr;
        return false;
    }
    else
    {
        stream = m_Stream.get();
        return true;
    }
}

bool PdfObject::TryGetStream(const PdfStream*& stream) const
{
    DelayedLoadStream();
    if (m_Stream == nullptr)
    {
        stream = nullptr;
        return false;
    }
    else
    {
        stream = m_Stream.get();
        return true;
    }
}

bool PdfObject::IsIndirect() const
{
    return m_IndirectReference.IsIndirect();
}

bool PdfObject::HasStream() const
{
    DelayedLoadStream();
    return m_Stream != nullptr;
}

PdfStream& PdfObject::getOrCreateStream()
{
    forceCreateStream();
    return *m_Stream;
}

void PdfObject::forceCreateStream()
{
    if (m_Stream != nullptr)
        return;

    if (m_Variant.GetDataType() != EPdfDataType::Dictionary)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Tried to get stream of non-dictionary object");

    if (m_Document == nullptr)
        m_Stream.reset(new PdfMemStream(*this));
    else
        m_Stream.reset(m_Document->GetObjects().CreateStream(*this));
}

PdfStream* PdfObject::getStream()
{
    return m_Stream.get();
}

void PdfObject::DelayedLoadStream() const
{
    DelayedLoad();
    delayedLoadStream();
}

void PdfObject::delayedLoadStream() const
{
    if (!m_DelayedLoadStreamDone)
    {
        const_cast<PdfObject&>(*this).DelayedLoadStreamImpl();
        m_DelayedLoadStreamDone = true;
    }
}

// TODO1: Add const PdfObject & operator=(const PdfVariant& rhs)
// TODO2: SetDirty only if the value to be added is different
//        For value (numbers) types this is trivial.
//        For dictionaries/lists maybe we can rely on auomatic dirty set
const PdfObject& PdfObject::operator=(const PdfObject& rhs)
{
    assign(rhs);
    SetDirty();
    return *this;
}

// NOTE: Don't copy parent document/container and indirect reference.
// Objects being assigned always keep current ownership
void PdfObject::copyFrom(const PdfObject& rhs)
{
    // NOTE: Don't call rhs.DelayedLoad() here. It's implicitly
    // called in PdfVariant assignment or copy constructor
    rhs.delayedLoadStream();
    SetVariantOwner();

    if (rhs.m_Stream != nullptr)
    {
        auto& stream = getOrCreateStream();
        stream = *rhs.m_Stream;
    }

    // Assume the delayed load of the stream is performed
    m_DelayedLoadStreamDone = true;
}

void PdfObject::EnableDelayedLoadingStream()
{
    m_DelayedLoadStreamDone = false;
}

void PdfObject::DelayedLoadStreamImpl()
{
    // Default implementation throws, since delayed loading of
    // steams should not be enabled except by types that support it.
    PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
}

void PdfObject::Assign(const PdfObject& rhs)
{
    if (&rhs == this)
        return;

    assign(rhs);
}

void PdfObject::assign(const PdfObject& rhs)
{
    rhs.DelayedLoad();
    m_Variant = rhs.m_Variant;
    copyFrom(rhs);
}

void PdfObject::ResetDirty()
{
    PODOFO_ASSERT(m_DelayedLoadDone);
    // Propagate new dirty state to subclasses
    switch (m_Variant.GetDataType())
    {
        // Arrays and Dictionaries
        // handle dirty status by themselves
        case EPdfDataType::Array:
            static_cast<PdfContainerDataType&>(m_Variant.GetArray()).ResetDirty();
            break;
        case EPdfDataType::Dictionary:
            static_cast<PdfContainerDataType&>(m_Variant.GetDictionary()).ResetDirty();
            break;
        case EPdfDataType::Bool:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
        case EPdfDataType::String:
        case EPdfDataType::Name:
        case EPdfDataType::RawData:
        case EPdfDataType::Reference:
        case EPdfDataType::Null:
        case EPdfDataType::Unknown:
        default:
            break;
    }

    resetDirty();
}

void PdfObject::SetDirty()
{
    if (IsIndirect())
    {
        // Set dirty only if is indirect object
        setDirty();
    }
    else if (m_Parent != nullptr)
    {
        // Reset parent if not indirect. Resetting will stop at
        // first indirect anchestor
        m_Parent->SetDirty();
    }
}

void PdfObject::setDirty()
{
    m_IsDirty = true;
}

void PdfObject::resetDirty()
{
    m_IsDirty = false;
}

void PdfObject::SetImmutable(bool isImmutable)
{
    DelayedLoad();
    m_IsImmutable = isImmutable;

    switch (m_Variant.GetDataType())
    {
        // Arrays and Dictionaries
        // handle dirty status by themselves
        case EPdfDataType::Array:
            m_Variant.GetArray().SetImmutable(isImmutable);
            break;
        case EPdfDataType::Dictionary:
            m_Variant.GetDictionary().SetImmutable(isImmutable);
            break;

        case EPdfDataType::Bool:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
        case EPdfDataType::String:
        case EPdfDataType::Name:
        case EPdfDataType::RawData:
        case EPdfDataType::Reference:
        case EPdfDataType::Null:
        case EPdfDataType::Unknown:
        default:
            // Do nothing
            break;
    }
}

PdfObject::operator const PdfVariant& () const
{
    DelayedLoad();
    return m_Variant;
}

const PdfVariant& PdfObject::GetVariant() const
{
    DelayedLoad();
    return m_Variant;
}

void PdfObject::Clear()
{
    DelayedLoad();
    m_Variant.Clear();
}

EPdfDataType PdfObject::GetDataType() const
{
    DelayedLoad();
    return m_Variant.GetDataType();
}

void PdfObject::ToString(string& data, PdfWriteMode writeMode) const
{
    DelayedLoad();
    m_Variant.ToString(data, writeMode);
}

bool PdfObject::GetBool() const
{
    DelayedLoad();
    return m_Variant.GetBool();
}

bool PdfObject::TryGetBool(bool& value) const
{
    DelayedLoad();
    return m_Variant.TryGetBool(value);
}

int64_t PdfObject::GetNumberLenient() const
{
    DelayedLoad();
    return m_Variant.GetNumberLenient();
}

bool PdfObject::TryGetNumberLenient(int64_t& value) const
{
    DelayedLoad();
    return m_Variant.TryGetNumberLenient(value);
}

int64_t PdfObject::GetNumber() const
{
    DelayedLoad();
    return m_Variant.GetNumber();
}

bool PdfObject::TryGetNumber(int64_t& value) const
{
    DelayedLoad();
    return m_Variant.TryGetNumber(value);
}

double PdfObject::GetReal() const
{
    DelayedLoad();
    return m_Variant.GetReal();
}

bool PdfObject::TryGetReal(double& value) const
{
    DelayedLoad();
    return m_Variant.TryGetReal(value);
}

double PdfObject::GetRealStrict() const
{
    DelayedLoad();
    return m_Variant.GetRealStrict();
}

bool PdfObject::TryGetRealStrict(double& value) const
{
    DelayedLoad();
    return m_Variant.TryGetRealStrict(value);
}

const PdfString& PdfObject::GetString() const
{
    DelayedLoad();
    return m_Variant.GetString();
}

bool PdfObject::TryGetString(PdfString& str) const
{
    DelayedLoad();
    return m_Variant.TryGetString(str);
}

const PdfName& PdfObject::GetName() const
{
    DelayedLoad();
    return m_Variant.GetName();
}

bool PdfObject::TryGetName(PdfName& name) const
{
    DelayedLoad();
    return m_Variant.TryGetName(name);
}

const PdfArray& PdfObject::GetArray() const
{
    DelayedLoad();
    return m_Variant.GetArray();
}

PdfArray& PdfObject::GetArray()
{
    DelayedLoad();
    return m_Variant.GetArray();
}

bool PdfObject::TryGetArray(const PdfArray*& arr) const
{
    DelayedLoad();
    return m_Variant.TryGetArray(arr);
}

bool PdfObject::TryGetArray(PdfArray*& arr)
{
    DelayedLoad();
    return m_Variant.TryGetArray(arr);
}

const PdfDictionary& PdfObject::GetDictionary() const
{
    DelayedLoad();
    return m_Variant.GetDictionary();
}

PdfDictionary& PdfObject::GetDictionary()
{
    DelayedLoad();
    return m_Variant.GetDictionary();
}

bool PdfObject::TryGetDictionary(const PdfDictionary*& dict) const
{
    DelayedLoad();
    return m_Variant.TryGetDictionary(dict);
}

bool PdfObject::TryGetDictionary(PdfDictionary*& dict)
{
    DelayedLoad();
    return m_Variant.TryGetDictionary(dict);
}

PdfReference PdfObject::GetReference() const
{
    DelayedLoad();
    return m_Variant.GetReference();
}

bool PdfObject::TryGetReference(PdfReference& ref) const
{
    DelayedLoad();
    return m_Variant.TryGetReference(ref);
}

void PdfObject::SetBool(bool b)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetBool(b);
    SetDirty();
}

void PdfObject::SetNumber(int64_t l)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetNumber(l);
    SetDirty();
}

void PdfObject::SetReal(double d)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetReal(d);
    SetDirty();
}

void PdfObject::SetName(const PdfName& name)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetName(name);
    SetDirty();
}

void PdfObject::SetString(const PdfString& str)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetString(str);
    SetDirty();
}

void PdfObject::SetReference(const PdfReference& ref)
{
    AssertMutable();
    DelayedLoad();
    m_Variant.SetReference(ref);
    SetDirty();
}

const char* PdfObject::GetDataTypeString() const
{
    DelayedLoad();
    return m_Variant.GetDataTypeString();
}

bool PdfObject::IsBool() const
{
    return GetDataType() == EPdfDataType::Bool;
}

bool PdfObject::IsNumber() const
{
    return GetDataType() == EPdfDataType::Number;
}

bool PdfObject::IsRealStrict() const
{
    return GetDataType() == EPdfDataType::Real;
}

bool PdfObject::IsNumberOrReal() const
{
    EPdfDataType dataType = GetDataType();
    return dataType == EPdfDataType::Number || dataType == EPdfDataType::Real;
}

bool PdfObject::IsString() const
{
    return GetDataType() == EPdfDataType::String;
}

bool PdfObject::IsName() const
{
    return GetDataType() == EPdfDataType::Name;
}

bool PdfObject::IsArray() const
{
    return GetDataType() == EPdfDataType::Array;
}

bool PdfObject::IsDictionary() const
{
    return GetDataType() == EPdfDataType::Dictionary;
}

bool PdfObject::IsRawData() const
{
    return GetDataType() == EPdfDataType::RawData;
}

bool PdfObject::IsNull() const
{
    return GetDataType() == EPdfDataType::Null;
}

bool PdfObject::IsReference() const
{
    return GetDataType() == EPdfDataType::Reference;
}

void PdfObject::AssertMutable() const
{
    if (m_IsImmutable)
        PODOFO_RAISE_ERROR(EPdfError::ChangeOnImmutable);
}

bool PdfObject::operator<(const PdfObject& rhs) const
{
    if (m_Document != rhs.m_Document)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Can't compare objects with different parent document");

    return m_IndirectReference < rhs.m_IndirectReference;
}

bool PdfObject::operator==(const PdfObject& rhs) const
{
    if (this == &rhs)
        return true;

    if (m_IndirectReference.IsIndirect())
    {
        // If lhs is indirect, just check document and reference
        return m_Document == rhs.m_Document &&
            m_IndirectReference == rhs.m_IndirectReference;
    }
    else
    {
        // Otherwise check variant
        DelayedLoad();
        rhs.DelayedLoad();
        return m_Variant == rhs.m_Variant;
    }
}

bool PdfObject::operator!=(const PdfObject& rhs) const
{
    if (this != &rhs)
        return true;

    if (m_IndirectReference.IsIndirect())
    {
        // If lhs is indirect, just check document and reference
        return m_Document != rhs.m_Document ||
            m_IndirectReference != rhs.m_IndirectReference;
    }
    else
    {
        // Otherwise check variant
        DelayedLoad();
        rhs.DelayedLoad();
        return m_Variant != rhs.m_Variant;
    }
}

bool PdfObject::operator==(const PdfVariant& rhs) const
{
    DelayedLoad();
    return m_Variant == rhs;
}

bool PdfObject::operator!=(const PdfVariant& rhs) const
{
    DelayedLoad();
    return m_Variant != rhs;
}
