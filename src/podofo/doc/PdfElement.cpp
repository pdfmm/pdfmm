/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfElement.h"

#include <doc/PdfDocument.h>
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"

#include "PdfStreamedDocument.h"

using namespace std;
using namespace PoDoFo;

PdfElement::PdfElement(PdfDocument& parent, const string_view& type)
{
    m_Object = parent.m_Objects.CreateDictionaryObject(type);
}

PdfElement::PdfElement(PdfObject& obj)
{
    if(!obj.IsDictionary())
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_Object = &obj;
}

PdfElement::PdfElement(EPdfDataType expectedDataType, PdfObject& obj)
{
    if (obj.GetDataType() != expectedDataType)
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_Object = &obj;
}

PdfElement::PdfElement(const PdfElement & element)
{
    m_Object = element.m_Object;
}

PdfElement::~PdfElement() { }

const char* PdfElement::TypeNameForIndex(unsigned index, const char** types, unsigned len) const
{
    return index < len ? types[index] : nullptr;
}

int PdfElement::TypeNameToIndex(const char* type, const char** types, unsigned len, int unknownValue) const
{
    if (type == nullptr)
        return unknownValue;

    for (unsigned i = 0; i < len; i++)
    {
        if (types[i] != nullptr && strcmp(type, types[i]) == 0)
            return i;
    }

    return unknownValue;
}

PdfObject* PdfElement::CreateObject(const string_view& type)
{
    return m_Object->GetDocument()->GetObjects().CreateDictionaryObject(type);
}

PdfDocument& PdfElement::GetDocument()
{
    return *m_Object->GetDocument();
}

const PdfDocument& PdfElement::GetDocument() const
{
    return *m_Object->GetDocument();
}
