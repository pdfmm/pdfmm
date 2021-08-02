/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfDestination.h"

#include "base/PdfDictionary.h"

#include "PdfAction.h"
#include "PdfMemDocument.h"
#include "PdfNamesTree.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"

using namespace PoDoFo;

PdfDestination::PdfDestination(PdfDocument& doc)
{
    m_pObject = doc.GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(PdfObject& obj)
{
    Init(obj, *obj.GetDocument());
}

PdfDestination::PdfDestination(const PdfPage& page, PdfDestinationFit eFit)
{
    PdfName type = PdfName("Fit");

    if (eFit == PdfDestinationFit::Fit)
        type = PdfName("Fit");
    else if (eFit == PdfDestinationFit::FitB)
        type = PdfName("FitB");
    else
    {
        // Peter Petrov 6 June 2008
        // silent mode
        //PODOFO_RAISE_ERROR( EPdfError::InvalidKey );
    }

    m_array.push_back(page.GetObject().GetIndirectReference());
    m_array.push_back(type);
    m_pObject = page.GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfPage& page, const PdfRect& rRect)
{
    PdfVariant var;

    rRect.ToVariant(var);

    m_array.push_back(page.GetObject().GetIndirectReference());
    m_array.push_back(PdfName("FitR"));
    m_array.insert(m_array.end(), var.GetArray().begin(), var.GetArray().end());
    m_pObject = page.GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfPage* pPage, double dLeft, double dTop, double dZoom)
{
    m_array.push_back(pPage->GetObject().GetIndirectReference());
    m_array.push_back(PdfName("XYZ"));
    m_array.push_back(dLeft);
    m_array.push_back(dTop);
    m_array.push_back(dZoom);
    m_pObject = pPage->GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfPage* pPage, PdfDestinationFit eFit, double dValue)
{
    PdfName type;

    if (eFit == PdfDestinationFit::FitH)
        type = PdfName("FitH");
    else if (eFit == PdfDestinationFit::FitV)
        type = PdfName("FitV");
    else if (eFit == PdfDestinationFit::FitBH)
        type = PdfName("FitBH");
    else if (eFit == PdfDestinationFit::FitBV)
        type = PdfName("FitBV");
    else
        PODOFO_RAISE_ERROR(EPdfError::InvalidKey);

    m_array.push_back(pPage->GetObject().GetIndirectReference());
    m_array.push_back(type);
    m_array.push_back(dValue);
    m_pObject = pPage->GetObject().GetDocument()->GetObjects().CreateObject(m_array);
}

PdfDestination::PdfDestination(const PdfDestination& rhs)
{
    this->operator=(rhs);
}

const PdfDestination& PdfDestination::operator=(const PdfDestination& rhs)
{
    m_array = rhs.m_array;
    m_pObject = rhs.m_pObject;

    return *this;
}

void PdfDestination::Init(PdfObject& obj, PdfDocument& document)
{
    bool bValueExpected = false;
    PdfObject* pValue = nullptr;

    if (obj.GetDataType() == EPdfDataType::Array)
    {
        m_array = obj.GetArray();
        m_pObject = &obj;
    }
    else if (obj.GetDataType() == EPdfDataType::String)
    {
        PdfNamesTree* pNames = document.GetNamesTree(ePdfDontCreateObject);
        if (!pNames)
        {
            PODOFO_RAISE_ERROR(EPdfError::NoObject);
        }

        pValue = pNames->GetValue("Dests", obj.GetString());
        bValueExpected = true;
    }
    else if (obj.GetDataType() == EPdfDataType::Name)
    {
        PdfMemDocument* pMemDoc = dynamic_cast<PdfMemDocument*>(&document);
        if (!pMemDoc)
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle,
                "For reading from a document, only use PdfMemDocument.");
        }

        PdfObject* pDests = pMemDoc->GetCatalog().GetDictionary().FindKey("Dests");
        if (!pDests)
        {
            // The error code has been chosen for its distinguishability.
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidKey,
                "No PDF-1.1-compatible destination dictionary found.");
        }
        pValue = pDests->GetDictionary().FindKey(obj.GetName());
        bValueExpected = true;
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Error, "Unsupported object given to"
            " PdfDestination::Init of type %s", obj.GetDataTypeString());
        m_array = PdfArray(); // needed to prevent crash on method calls
        // needed for GetObject() use w/o checking its return value for nullptr
        m_pObject = document.GetObjects().CreateObject(m_array);
    }
    if (bValueExpected)
    {
        if (!pValue)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidName);
        }

        if (pValue->IsArray())
            m_array = pValue->GetArray();
        else if (pValue->IsDictionary())
            m_array = pValue->GetDictionary().MustFindKey("D").GetArray();
        m_pObject = pValue;
    }
}

void PdfDestination::AddToDictionary(PdfDictionary& dictionary) const
{
    // Do not add empty destinations
    if (!m_array.size())
        return;

    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if (dictionary.HasKey(PdfName("A")))
        PODOFO_RAISE_ERROR(EPdfError::ActionAlreadyPresent);

    dictionary.AddKey("Dest", *m_pObject);
}

PdfPage* PdfDestination::GetPage(PdfDocument* pDoc)
{
    if (!m_array.size())
        return nullptr;

    // first entry in the array is the page - so just make a new page from it!
    return &pDoc->GetPageTree().GetPage(m_array[0].GetReference());
}

PdfPage* PdfDestination::GetPage(PdfVecObjects* pVecObjects)
{
    return this->GetPage(&pVecObjects->GetDocument());
}

PdfDestinationType PdfDestination::GetType() const
{
    if (!m_array.size())
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
        PODOFO_RAISE_ERROR(EPdfError::WrongDestinationType);
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
        PODOFO_RAISE_ERROR(EPdfError::WrongDestinationType);
    }

    return m_array[2].GetReal();
}

PdfRect PdfDestination::GetRect() const
{
    if (GetType() != PdfDestinationType::FitR)
        PODOFO_RAISE_ERROR(EPdfError::WrongDestinationType);

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
            PODOFO_RAISE_ERROR(EPdfError::WrongDestinationType);
        }
    };
}

double PdfDestination::GetZoom() const
{
    if (GetType() != PdfDestinationType::XYZ)
        PODOFO_RAISE_ERROR(EPdfError::WrongDestinationType);

    return m_array[4].GetReal();
}
