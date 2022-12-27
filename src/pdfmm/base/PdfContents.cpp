/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfContents.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfStreamDevice.h"

#include "PdfDocument.h"
#include "PdfPage.h"

using namespace mm;

PdfContents::PdfContents(PdfPage &parent, PdfObject &obj)
    : m_parent(&parent), m_object(&obj)
{
}

PdfContents::PdfContents(PdfPage &parent) 
    : m_parent(&parent), m_object(parent.GetDocument().GetObjects().CreateArrayObject())
{
    reset();
}

void PdfContents::Reset(PdfObject* obj)
{
    if (obj == nullptr)
    {
        m_object = m_parent->GetDocument().GetObjects().CreateArrayObject();
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
    m_parent->GetObject().GetDictionary().AddKeyIndirect("Contents", *m_object);
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
        auto newObjArray = m_parent->GetDocument().GetObjects().CreateArrayObject();
        m_parent->GetObject().GetDictionary().AddKeyIndirect("Contents", *newObjArray);
        arr = &newObjArray->GetArray();
        arr->AddIndirect(*m_object);
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
            PdfObjectStream* stream;
            auto streamObj = arr->FindAt(i);
            if (streamObj != nullptr && (stream = streamObj->GetStream()) != nullptr)
            {
                BufferStreamDevice device(buffer);
                stream->CopyTo(device);
            }
        }

        if (buffer.size() != 0)
        {
            arr->Clear();
            auto newobj = m_parent->GetDocument().GetObjects().CreateDictionaryObject();
            arr->AddIndirect(*newobj);
            auto &stream = newobj->GetOrCreateStream();
            auto output = stream.GetOutputStream();
            output.Write("q\n");
            output.Write(buffer);
            // TODO: Avoid adding unuseful \n prior Q
            output.Write("\nQ");
        }
    }

    // Create a new stream, add it to the array, return it
    PdfObject * newStm = m_parent->GetDocument().GetObjects().CreateDictionaryObject();
    if ((flags & PdfStreamAppendFlags::Prepend) == PdfStreamAppendFlags::Prepend)
        arr->insert(arr->begin(), newStm->GetIndirectReference());
    else
        arr->Add(newStm->GetIndirectReference());
    return newStm->GetOrCreateStream();
}
