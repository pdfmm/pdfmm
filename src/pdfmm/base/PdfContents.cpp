/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfContents.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfOutputDevice.h"

#include "PdfDocument.h"
#include "PdfPage.h"

using namespace mm;

PdfContents::PdfContents(PdfPage &parent, PdfObject &obj)
    : m_parent(&parent), m_object(&obj)
{
}

PdfContents::PdfContents(PdfPage &parent) 
    : m_parent(&parent), m_object(parent.GetObject().GetDocument()->GetObjects().CreateArrayObject())
{
    reset();
}

void PdfContents::Reset(PdfObject* obj)
{
    if (obj == nullptr)
    {
        m_object = m_parent->GetObject().GetDocument()->GetObjects().CreateArrayObject();
    }
    else
    {
        PdfDataType type = obj->GetDataType();
        if (!(type == PdfDataType::Array || type == PdfDataType::Dictionary))
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "The object is neither a Dictionary or an Array");

        m_object = obj;
    }

    reset();
}

void PdfContents::reset()
{
    m_parent->GetObject().GetDictionary().AddKeyIndirect("Contents", m_object);
}

PdfObjectStream & PdfContents::GetStreamForAppending(PdfStreamAppendFlags flags)
{
    PdfArray *arr;
    if (m_object->IsArray())
    {
        arr = &m_object->GetArray();
    }
    else if (m_object->IsDictionary())
    {
        // Create a /Contents array and put the current stream into it
        auto newObjArray = m_parent->GetObject().GetDocument()->GetObjects().CreateObject(PdfArray());
        m_parent->GetObject().GetDictionary().AddKeyIndirect("Contents", newObjArray);
        arr = &newObjArray->GetArray();
        arr->AddIndirect(m_object);
        m_object = newObjArray;
    }
    else
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }

    if ((flags & PdfStreamAppendFlags::NoSaveRestorePrior) == PdfStreamAppendFlags::None)
    {
        // Record all content and readd into a new stream that
        // substitue all the previous streams
        charbuff buffer;
        for (unsigned i = 0; i < arr->GetSize(); i++)
        {
            auto stream = arr->FindAt(i).GetStream();
            if (stream != nullptr)
                stream->ExtractTo(buffer);
        }

        if (buffer.size() != 0)
        {
            PdfObject* newobj = m_object->GetDocument()->GetObjects().CreateDictionaryObject();
            auto &stream = newobj->GetOrCreateStream();
            stream.BeginAppend();
            stream.Append("q\n");
            stream.AppendBuffer(buffer);
            // TODO: Avoid adding unuseful \n prior Q
            stream.Append("\nQ");
            stream.EndAppend();
            arr->Clear();
            arr->Add(newobj->GetIndirectReference());
        }
    }

    // Create a new stream, add it to the array, return it
    PdfObject * newStm = m_object->GetDocument()->GetObjects().CreateDictionaryObject();
    if ((flags & PdfStreamAppendFlags::Prepend) == PdfStreamAppendFlags::Prepend)
        arr->insert(arr->begin(), newStm->GetIndirectReference());
    else
        arr->Add(newStm->GetIndirectReference());
    return newStm->GetOrCreateStream();
}
