/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfDestination.h"

#include "PdfDictionary.h"
#include "PdfAction.h"
#include "PdfMemDocument.h"
#include "PdfNameTree.h"
#include "PdfPage.h"
#include "PdfPageTree.h"

using namespace mm;

PdfDestination::PdfDestination(PdfDocument& doc)
{
    m_Object = doc.GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(PdfObject& obj)
{
    Init(obj, *obj.GetDocument());
}

PdfDestination::PdfDestination(const PdfPage& page, PdfDestinationFit fit)
{
    PdfName type = PdfName("Fit");

    if (fit == PdfDestinationFit::Fit)
        type = PdfName("Fit");
    else if (fit == PdfDestinationFit::FitB)
        type = PdfName("FitB");
    else
    {
        // Peter Petrov 6 June 2008
        // silent mode
        //PDFMM_RAISE_ERROR( EPdfError::InvalidKey );
    }

    m_array.push_back(page.GetObject().GetIndirectReference());
    m_array.push_back(type);
    m_Object = page.GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfPage& page, const PdfRect& rect)
{
    PdfArray arr;
    rect.ToArray(arr);

    m_array.push_back(page.GetObject().GetIndirectReference());
    m_array.push_back(PdfName("FitR"));
    m_array.insert(m_array.end(), arr.begin(), arr.end());
    m_Object = page.GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfPage& page, double left, double top, double zoom)
{
    m_array.push_back(page.GetObject().GetIndirectReference());
    m_array.push_back(PdfName("XYZ"));
    m_array.push_back(left);
    m_array.push_back(top);
    m_array.push_back(zoom);
    m_Object = page.GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfPage& page, PdfDestinationFit fit, double value)
{
    PdfName type;

    if (fit == PdfDestinationFit::FitH)
        type = PdfName("FitH");
    else if (fit == PdfDestinationFit::FitV)
        type = PdfName("FitV");
    else if (fit == PdfDestinationFit::FitBH)
        type = PdfName("FitBH");
    else if (fit == PdfDestinationFit::FitBV)
        type = PdfName("FitBV");
    else
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidKey);

    m_array.push_back(page.GetObject().GetIndirectReference());
    m_array.push_back(type);
    m_array.push_back(value);
    m_Object = page.GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfDestination& rhs)
{
    this->operator=(rhs);
}

const PdfDestination& PdfDestination::operator=(const PdfDestination& rhs)
{
    m_array = rhs.m_array;
    m_Object = rhs.m_Object;

    return *this;
}

void PdfDestination::Init(PdfObject& obj, PdfDocument& doc)
{
    bool valueExpected = false;
    PdfObject* value = nullptr;

    if (obj.GetDataType() == PdfDataType::Array)
    {
        m_array = obj.GetArray();
        m_Object = &obj;
    }
    else if (obj.GetDataType() == PdfDataType::String)
    {
        auto names = doc.GetNameTree();
        if (names == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

        value = names->GetValue("Dests", obj.GetString());
        valueExpected = true;
    }
    else if (obj.GetDataType() == PdfDataType::Name)
    {
        auto memDoc = dynamic_cast<PdfMemDocument*>(&doc);
        if (memDoc == nullptr)
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle,
                "For reading from a document, only use PdfMemDocument");
        }

        auto dests = memDoc->GetCatalog().GetDictionary().FindKey("Dests");
        if (dests == nullptr)
        {
            // The error code has been chosen for its distinguishability.
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidKey,
                "No PDF-1.1-compatible destination dictionary found");
        }
        value = dests->GetDictionary().FindKey(obj.GetName());
        valueExpected = true;
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Error, "Unsupported object given to "
            "PdfDestination::Init of type {}", obj.GetDataTypeString());
        m_array = PdfArray(); // needed to prevent crash on method calls
        // needed for GetObject() use w/o checking its return value for nullptr
        m_Object = doc.GetObjects().CreateObject(m_array);
    }

    if (valueExpected)
    {
        if (value == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidName);

        if (value->IsArray())
            m_array = value->GetArray();
        else if (value->IsDictionary())
            m_array = value->GetDictionary().MustFindKey("D").GetArray();

        m_Object = value;
    }
}

void PdfDestination::AddToDictionary(PdfDictionary& dictionary) const
{
    // Do not add empty destinations
    if (m_array.size() == 0)
        return;

    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if (dictionary.HasKey("A"))
        PDFMM_RAISE_ERROR(PdfErrorCode::ActionAlreadyPresent);

    dictionary.AddKey("Dest", *m_Object);
}

PdfPage* PdfDestination::GetPage()
{
    if (m_array.size() == 0)
        return nullptr;

    // first entry in the array is the page - so just make a new page from it!
    return &m_Object->GetDocument()->GetPageTree().GetPage(m_array[0].GetReference());
}

PdfDestinationType PdfDestination::GetType() const
{
    if (m_array.size() == 0)
        return PdfDestinationType::Unknown;

    PdfName tp = m_array[1].GetName();

    if (tp == "XYZ")
        return PdfDestinationType::XYZ;
    else if (tp == "Fit")
        return PdfDestinationType::Fit;
    else if (tp == "FitH")
        return PdfDestinationType::FitH;
    else if (tp == "FitV")
        return PdfDestinationType::FitV;
    else if (tp == "FitR")
        return PdfDestinationType::FitR;
    else if (tp == "FitB")
        return PdfDestinationType::FitB;
    else if (tp == "FitBH")
        return PdfDestinationType::FitBH;
    else if (tp == "FitBV")
        return PdfDestinationType::FitBV;
    else
        return PdfDestinationType::Unknown;
}

double PdfDestination::GetDValue() const
{
    PdfDestinationType tp = GetType();

    if (tp != PdfDestinationType::FitH
        && tp != PdfDestinationType::FitV
        && tp != PdfDestinationType::FitBH)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::WrongDestinationType);
    }

    return m_array[2].GetReal();
}

double PdfDestination::GetLeft() const
{
    PdfDestinationType tp = GetType();

    if (tp != PdfDestinationType::FitV
        && tp != PdfDestinationType::XYZ
        && tp != PdfDestinationType::FitR)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::WrongDestinationType);
    }

    return m_array[2].GetReal();
}

PdfRect PdfDestination::GetRect() const
{
    if (GetType() != PdfDestinationType::FitR)
        PDFMM_RAISE_ERROR(PdfErrorCode::WrongDestinationType);

    return PdfRect(m_array[2].GetReal(), m_array[3].GetReal(),
        m_array[4].GetReal(), m_array[5].GetReal());
}

double PdfDestination::GetTop() const
{
    PdfDestinationType tp = GetType();

    switch (tp)
    {
        case PdfDestinationType::XYZ:
            return m_array[3].GetReal();
        case PdfDestinationType::FitH:
        case PdfDestinationType::FitBH:
            return m_array[2].GetReal();
        case PdfDestinationType::FitR:
            return m_array[5].GetReal();
        case PdfDestinationType::Fit:
        case PdfDestinationType::FitV:
        case PdfDestinationType::FitB:
        case PdfDestinationType::FitBV:
        case PdfDestinationType::Unknown:
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::WrongDestinationType);
        }
    };
}

double PdfDestination::GetZoom() const
{
    if (GetType() != PdfDestinationType::XYZ)
        PDFMM_RAISE_ERROR(PdfErrorCode::WrongDestinationType);

    return m_array[4].GetReal();
}
