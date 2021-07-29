/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfVariant.h"

#include "PdfArray.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfOutputDevice.h"
#include "PdfParserObject.h"

#include <sstream>

using namespace PoDoFo;
using namespace std;

PdfVariant PdfVariant::NullValue;

PdfVariant::PdfVariant(EPdfDataType type)
    : m_Data{ }, m_DataType(type) { }

PdfVariant::PdfVariant()
    : PdfVariant(EPdfDataType::Null) { }

PdfVariant::PdfVariant(bool b)
    : PdfVariant(EPdfDataType::Bool)
{
    m_Data.Bool = b;
}

PdfVariant::PdfVariant(int64_t l)
    : PdfVariant(EPdfDataType::Number)
{
    m_Data.Number = l;
}

PdfVariant::PdfVariant(double d)
    : PdfVariant(EPdfDataType::Real)
{
    m_Data.Real = d;
}

PdfVariant::PdfVariant(const PdfString& str)
    : PdfVariant(EPdfDataType::String)
{
    m_Data.Data = new PdfString(str);
}

PdfVariant::PdfVariant(const PdfName& name)
    : PdfVariant(EPdfDataType::Name)
{
    m_Data.Data = new PdfName(name);
}

PdfVariant::PdfVariant(const PdfReference& ref)
    : PdfVariant(EPdfDataType::Reference)
{
    m_Data.Reference = ref;
}

PdfVariant::PdfVariant(const PdfArray& arr)
    : PdfVariant(EPdfDataType::Array)
{
    m_Data.Data = new PdfArray(arr);
}

PdfVariant::PdfVariant(const PdfDictionary& dict)
    : PdfVariant(EPdfDataType::Dictionary)
{
    m_Data.Data = new PdfDictionary(dict);
}

PdfVariant::PdfVariant(const PdfData& data)
    : PdfVariant(EPdfDataType::RawData)
{
    m_Data.Data = new PdfData(data);
}

PdfVariant::PdfVariant(const PdfVariant& rhs)
    : PdfVariant(EPdfDataType::Unknown)
{
    this->operator=(rhs);
}

PdfVariant::~PdfVariant()
{
    Clear();
}

void PdfVariant::Clear()
{
    switch (m_DataType)
    {
        case EPdfDataType::Array:
        case EPdfDataType::Dictionary:
        case EPdfDataType::Name:
        case EPdfDataType::String:
        case EPdfDataType::RawData:
        {
            delete m_Data.Data;
            break;
        }

        case EPdfDataType::Reference:
        case EPdfDataType::Bool:
        case EPdfDataType::Null:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
        case EPdfDataType::Unknown:
        default:
            break;

    }

    m_Data = { };
}

void PdfVariant::Write(PdfOutputDevice& device, PdfWriteMode writeMode,
    const PdfEncrypt* encrypt) const
{
    switch (m_DataType)
    {
        case EPdfDataType::Bool:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
                device.Write(" ", 1); // Write space before true or false

            if (m_Data.Bool)
                device.Write("true", 4);
            else
                device.Write("false", 5);
            break;
        }
        case EPdfDataType::Number:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
                device.Write(" ", 1); // Write space before numbers

            device.Print("%" PDF_FORMAT_INT64, m_Data.Number);
            break;
        }
        case EPdfDataType::Real:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
                device.Write(" ", 1); // Write space before numbers

            // Use ostringstream, so that locale does not matter
            // NOTE: Don't use printf() formatting! It may write the number
            // way that is incompatible in PDF
            ostringstream oss;
            PdfLocaleImbue(oss);
            oss << std::fixed << m_Data.Real;
            string copy = oss.str();
            size_t len = copy.size();

            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact &&
                copy.find('.') != string::npos)
            {
                const char* str = copy.c_str();
                while (str[len - 1] == '0')
                    len--;

                if (str[len - 1] == '.')
                    len--;

                if (len == 0)
                {
                    device.Write("0", 1);
                    break;
                }
            }

            device.Write(copy.c_str(), len);
            break;
        }
        case EPdfDataType::Reference:
            m_Data.Reference.Write(device, writeMode, encrypt);
            break;
        case EPdfDataType::String:
        case EPdfDataType::Name:
        case EPdfDataType::Array:
        case EPdfDataType::Dictionary:
        case EPdfDataType::RawData:
            if (m_Data.Data == nullptr)
                PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

            m_Data.Data->Write(device, writeMode, encrypt);
            break;
        case EPdfDataType::Null:
        {
            if ((writeMode & PdfWriteMode::Compact) == PdfWriteMode::Compact)
            {
                device.Write(" ", 1); // Write space before null
            }

            device.Print("null");
            break;
        }
        case EPdfDataType::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
            break;
        }
    };
}

void PdfVariant::ToString(string& data, PdfWriteMode writeMode) const
{
    ostringstream out;
    // We don't need to this stream with the safe PDF locale because
    // PdfOutputDevice will do so for us.
    PdfOutputDevice device(out);

    this->Write(device, writeMode, nullptr);

    data = out.str();
}

const PdfVariant& PdfVariant::operator=(const PdfVariant& rhs)
{
    Clear();

    m_DataType = rhs.m_DataType;

    switch (m_DataType)
    {
        case EPdfDataType::Array:
        {
            m_Data.Data = new PdfArray(*static_cast<PdfArray*>(rhs.m_Data.Data));
            break;
        }
        case EPdfDataType::Dictionary:
        {
            m_Data.Data = new PdfDictionary(*static_cast<PdfDictionary*>(rhs.m_Data.Data));
            break;
        }
        case EPdfDataType::Name:
        {
            m_Data.Data = new PdfName(*static_cast<PdfName*>(rhs.m_Data.Data));
            break;
        }
        case EPdfDataType::String:
        {
            m_Data.Data = new PdfString(*static_cast<PdfString*>(rhs.m_Data.Data));
            break;
        }

        case EPdfDataType::RawData:
        {
            m_Data.Data = new PdfData(*static_cast<PdfData*>(rhs.m_Data.Data));
            break;
        }
        case EPdfDataType::Reference:
        case EPdfDataType::Bool:
        case EPdfDataType::Null:
        case EPdfDataType::Number:
        case EPdfDataType::Real:
            m_Data = rhs.m_Data;
            break;

        case EPdfDataType::Unknown:
        default:
            break;
    };

    return *this;
}

const char* PdfVariant::GetDataTypeString() const
{
    switch (m_DataType)
    {
        case EPdfDataType::Bool:
            return "Bool";
        case EPdfDataType::Number:
            return "Number";
        case EPdfDataType::Real:
            return "Real";
        case EPdfDataType::String:
            return "String";
        case EPdfDataType::Name:
            return "Name";
        case EPdfDataType::Array:
            return "Array";
        case EPdfDataType::Dictionary:
            return "Dictionary";
        case EPdfDataType::Null:
            return "Null";
        case EPdfDataType::Reference:
            return "Reference";
        case EPdfDataType::RawData:
            return "RawData";
        case EPdfDataType::Unknown:
            return "Unknown";
        default:
            return "INVALID_TYPE_ENUM";
    }
}

bool PdfVariant::operator==(const PdfVariant& rhs) const
{
    if (this == &rhs)
        return true;

    switch (m_DataType)
    {
        case EPdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Data.Bool == value;
            else
                return false;
        }
        case EPdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Data.Number == value;
            else
                return false;
        }
        case EPdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Data.Real == value;
            else
                return false;
        }
        case EPdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Data.Reference == value;
            else
                return false;
        }
        case EPdfDataType::String:
        {
            const PdfString* value;
            if (rhs.TryGetString(value))
                return *(PdfString*)m_Data.Data == *value;
            else
                return false;
        }
        case EPdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.TryGetName(value))
                return *(PdfName*)m_Data.Data == *value;
            else
                return false;
        }
        case EPdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *(PdfArray*)m_Data.Data == *value;
            else
                return false;
        }
        case EPdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *(PdfDictionary*)m_Data.Data == *value;
            else
                return false;
        }
        case EPdfDataType::RawData:
            PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "Equality not yet implemented for RawData");
        case EPdfDataType::Null:
            return m_DataType == EPdfDataType::Null;
        case EPdfDataType::Unknown:
            return false;
        default:
            PODOFO_RAISE_ERROR(EPdfError::NotImplemented);
    }
}

bool PdfVariant::operator!=(const PdfVariant& rhs) const
{
    if (this != &rhs)
        return true;

    switch (m_DataType)
    {
        case EPdfDataType::Bool:
        {
            bool value;
            if (rhs.TryGetBool(value))
                return m_Data.Bool != value;
            else
                return true;
        }
        case EPdfDataType::Number:
        {
            int64_t value;
            if (rhs.TryGetNumber(value))
                return m_Data.Number != value;
            else
                return true;
        }
        case EPdfDataType::Real:
        {
            // NOTE: Real type equality semantics is strict
            double value;
            if (rhs.TryGetRealStrict(value))
                return m_Data.Real != value;
            else
                return true;
        }
        case EPdfDataType::Reference:
        {
            PdfReference value;
            if (rhs.TryGetReference(value))
                return m_Data.Reference != value;
            else
                return true;
        }
        case EPdfDataType::String:
        {
            const PdfString* value;
            if (rhs.TryGetString(value))
                return *(PdfString*)m_Data.Data != *value;
            else
                return true;
        }
        case EPdfDataType::Name:
        {
            const PdfName* value;
            if (rhs.TryGetName(value))
                return *(PdfName*)m_Data.Data != *value;
            else
                return true;
        }
        case EPdfDataType::Array:
        {
            const PdfArray* value;
            if (rhs.TryGetArray(value))
                return *(PdfArray*)m_Data.Data != *value;
            else
                return true;
        }
        case EPdfDataType::Dictionary:
        {
            const PdfDictionary* value;
            if (rhs.TryGetDictionary(value))
                return *(PdfDictionary*)m_Data.Data != *value;
            else
                return true;
        }
        case EPdfDataType::RawData:
            PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "Disequality not yet implemented for RawData");
        case EPdfDataType::Null:
            return m_DataType != EPdfDataType::Null;
        case EPdfDataType::Unknown:
            return true;
        default:
            PODOFO_RAISE_ERROR(EPdfError::NotImplemented);
    }
}

bool PdfVariant::GetBool() const
{
    bool ret;
    if (!TryGetBool(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetBool(bool& value) const
{
    if (m_DataType != EPdfDataType::Bool)
    {
        value = false;
        return false;
    }

    value = m_Data.Bool;
    return true;
}

int64_t PdfVariant::GetNumberLenient() const
{
    int64_t ret;
    if (!TryGetNumberLenient(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetNumberLenient(int64_t& value) const
{
    if (!(m_DataType == EPdfDataType::Number
        || m_DataType == EPdfDataType::Real))
    {
        value = 0;
        return false;
    }

    if (m_DataType == EPdfDataType::Real)
        value = static_cast<int64_t>(std::round(m_Data.Real));
    else
        value = m_Data.Number;

    return true;
}

int64_t PdfVariant::GetNumber() const
{
    int64_t ret;
    if (!TryGetNumber(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return m_Data.Number;
}

bool PdfVariant::TryGetNumber(int64_t& value) const
{
    if (m_DataType != EPdfDataType::Number)
    {
        value = 0;
        return false;
    }

    value = m_Data.Number;
    return true;
}

double PdfVariant::GetReal() const
{
    double ret;
    if (!TryGetReal(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReal(double& value) const
{
    if (!(m_DataType == EPdfDataType::Real
        || m_DataType == EPdfDataType::Number))
    {
        value = 0;
        return false;
    }

    if (m_DataType == EPdfDataType::Number)
        value = static_cast<double>(m_Data.Number);
    else
        value = m_Data.Real;

    return true;
}

double PdfVariant::GetRealStrict() const
{
    double ret;
    if (!TryGetRealStrict(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return m_Data.Real;
}

bool PdfVariant::TryGetRealStrict(double& value) const
{
    if (m_DataType != EPdfDataType::Real)
    {
        value = 0;
        return false;
    }

    value = m_Data.Real;
    return true;
}

const PdfString& PdfVariant::GetString() const
{
    const PdfString* ret;
    if (!TryGetString(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetString(const PdfString*& str) const
{
    if (m_DataType != EPdfDataType::String)
    {
        str = nullptr;
        return false;
    }

    str = (PdfString*)m_Data.Data;
    return true;
}

const PdfName& PdfVariant::GetName() const
{
    const PdfName* ret;
    if (!TryGetName(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetName(const PdfName*& name) const
{
    if (m_DataType != EPdfDataType::Name)
    {
        name = nullptr;
        return false;
    }

    name = (PdfName*)m_Data.Data;
    return true;
}

PdfReference PdfVariant::GetReference() const
{
    PdfReference ret;
    if (!TryGetReference(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return ret;
}

bool PdfVariant::TryGetReference(PdfReference& ref) const
{
    if (m_DataType != EPdfDataType::Reference)
    {
        ref = PdfReference();
        return false;
    }

    ref = m_Data.Reference;
    return true;
}

const PdfData& PdfVariant::GetRawData() const
{
    const PdfData* ret;
    if (!TryGetRawData(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfData& PdfVariant::GetRawData()
{
    PdfData* ret;
    if (!TryGetRawData(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetRawData(const PdfData*& data) const
{
    if (m_DataType != EPdfDataType::RawData)
    {
        data = nullptr;
        return false;
    }

    data = (PdfData*)m_Data.Data;
    return true;
}

bool PdfVariant::TryGetRawData(PdfData*& data)
{
    if (m_DataType != EPdfDataType::RawData)
    {
        data = nullptr;
        return false;
    }

    data = (PdfData*)m_Data.Data;
    return true;
}

const PdfArray& PdfVariant::GetArray() const
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfArray& PdfVariant::GetArray()
{
    PdfArray* ret;
    if (!tryGetArray(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetArray(const PdfArray*& arr) const
{
    return tryGetArray(const_cast<PdfArray*&>(arr));
}

bool PdfVariant::TryGetArray(PdfArray*& arr)
{
    return tryGetArray(arr);
}

const PdfDictionary& PdfVariant::GetDictionary() const
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

PdfDictionary& PdfVariant::GetDictionary()
{
    PdfDictionary* ret;
    if (!tryGetDictionary(ret))
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    return *ret;
}

bool PdfVariant::TryGetDictionary(const PdfDictionary*& dict) const
{
    return tryGetDictionary(const_cast<PdfDictionary*&>(dict));
}

bool PdfVariant::TryGetDictionary(PdfDictionary*& dict)
{
    return tryGetDictionary(dict);
}

bool PdfVariant::tryGetDictionary(PdfDictionary*& dict) const
{
    if (m_DataType != EPdfDataType::Dictionary)
    {
        dict = nullptr;
        return false;
    }

    dict = (PdfDictionary*)m_Data.Data;
    return true;
}

bool PdfVariant::tryGetArray(PdfArray*& arr) const
{
    if (m_DataType != EPdfDataType::Array)
    {
        arr = nullptr;
        return false;
    }

    arr = (PdfArray*)m_Data.Data;
    return true;
}

void PdfVariant::SetBool(bool b)
{
    if (m_DataType != EPdfDataType::Bool)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_Data.Bool = b;
}

void PdfVariant::SetNumber(int64_t l)
{
    if (!(m_DataType == EPdfDataType::Number
        || m_DataType == EPdfDataType::Real))
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if (m_DataType == EPdfDataType::Real)
        m_Data.Real = static_cast<double>(l);
    else
        m_Data.Number = l;
}

void PdfVariant::SetReal(double d)
{
    if (!(m_DataType == EPdfDataType::Real
        || m_DataType == EPdfDataType::Number))
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if (m_DataType == EPdfDataType::Number)
        m_Data.Number = static_cast<int64_t>(std::round(d));
    else
        m_Data.Real = d;
}

void PdfVariant::SetName(const PdfName &name)
{
    if (m_DataType != EPdfDataType::Name)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfName*)m_Data.Data) = name;
}

void PdfVariant::SetString(const PdfString &str)
{
    if (m_DataType != EPdfDataType::String)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    *((PdfString*)m_Data.Data) = str;
}

void PdfVariant::SetReference(const PdfReference &ref)
{
    if (m_DataType != EPdfDataType::Reference)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_Data.Reference = ref;
}

bool PdfVariant::IsBool() const
{
    return m_DataType == EPdfDataType::Bool;
}

bool PdfVariant::IsNumber() const
{
    return m_DataType == EPdfDataType::Number;
}

bool PdfVariant::IsRealStrict() const
{
    return m_DataType == EPdfDataType::Real;
}

bool PdfVariant::IsNumberOrReal() const
{
    return m_DataType == EPdfDataType::Number || m_DataType == EPdfDataType::Real;
}

bool PdfVariant::IsString() const
{
    return m_DataType == EPdfDataType::String;
}

bool PdfVariant::IsName() const
{
    return m_DataType == EPdfDataType::Name;
}

bool PdfVariant::IsArray() const
{
    return m_DataType == EPdfDataType::Array;
}

bool PdfVariant::IsDictionary() const
{
    return m_DataType == EPdfDataType::Dictionary;
}

bool PdfVariant::IsRawData() const
{
    return m_DataType == EPdfDataType::RawData;
}

bool PdfVariant::IsNull() const
{
    return m_DataType == EPdfDataType::Null;
}

bool PdfVariant::IsReference() const
{
    return m_DataType == EPdfDataType::Reference;
}
