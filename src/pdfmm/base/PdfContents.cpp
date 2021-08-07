/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfContents.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfOutputDevice.h"

#include "PdfDocument.h"
#include "PdfPage.h"

#include <iostream>

using namespace mm;

PdfContents::PdfContents(PdfPage &parent, PdfObject &obj)
    : m_parent(&parent), m_object(&obj)
{
}

PdfContents::PdfContents(PdfPage &parent) 
    : m_parent(&parent), m_object(parent.GetObject().GetDocument()->GetObjects().CreateObject(PdfArray()))
{
    parent.GetObject().GetDictionary().AddKey("Contents", m_object->GetIndirectReference());
}

PdfObject & PdfContents::GetContents() const
{
    return *m_object;
}

PdfStream & PdfContents::GetStreamForAppending(EPdfStreamAppendFlags flags)
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
        m_parent->GetObject().GetDictionary().AddKey("Contents", newObjArray->GetIndirectReference());
        arr = &newObjArray->GetArray();
        arr->push_back(m_object->GetIndirectReference());
        m_object = newObjArray;
    }
    else
    {
        PDFMM_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    if ((flags & EPdfStreamAppendFlags::NoSaveRestorePrior) == EPdfStreamAppendFlags::None)
    {
        // Record all content and readd into a new stream that
        // substitue all the previous streams
        PdfMemoryOutputStream memstream;
        for (unsigned i = 0; i < arr->GetSize(); i++)
        {
            const PdfStream* stream;
            if (arr->FindAt(i).TryGetStream(stream))
                stream->GetFilteredCopy(memstream);
        }

        if (memstream.GetLength() != 0)
        {
            PdfObject* newobj = m_object->GetDocument()->GetObjects().CreateDictionaryObject();
            auto &stream = newobj->GetOrCreateStream();
            stream.BeginAppend();
            stream.Append("q\n");
            stream.Append(memstream.GetBuffer(), memstream.GetLength());
            // TODO: Avoid adding unuseful \n prior Q
            stream.Append("\nQ");
            stream.EndAppend();
            arr->clear();
            arr->push_back(newobj->GetIndirectReference());
        }
    }

    // Create a new stream, add it to the array, return it
    PdfObject * newStm = m_object->GetDocument()->GetObjects().CreateDictionaryObject();
    if ((flags & EPdfStreamAppendFlags::Prepend) == EPdfStreamAppendFlags::Prepend)
        arr->insert(arr->begin(), newStm->GetIndirectReference());
    else
        arr->push_back(newStm->GetIndirectReference());
    return newStm->GetOrCreateStream();
};
