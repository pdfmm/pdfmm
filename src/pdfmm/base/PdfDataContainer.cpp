/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfDataContainer.h"

#include "PdfDocument.h"
#include "PdfObject.h"
#include "PdfIndirectObjectList.h"

using namespace mm;

PdfDataContainer::PdfDataContainer()
    : m_Owner(nullptr)
{
}

void PdfDataContainer::SetOwner(PdfObject& owner)
{
    m_Owner = &owner;
    setChildrenParent();
}

void PdfDataContainer::ResetDirty()
{
    ResetDirtyInternal();
}

PdfObject& PdfDataContainer::GetIndirectObject(const PdfReference& ref) const
{
    if (m_Owner == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Object is a reference but does not have an owner");

    auto document = m_Owner->GetDocument();
    if (document == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Object owner is not part of any document");

    auto ret = document->GetObjects().GetObject(ref);
    if (ret == nullptr)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Can't find object {} {} R",
            ref.ObjectNumber(), ref.GenerationNumber());
    }

    return *ret;
}

void PdfDataContainer::SetDirty()
{
    if (m_Owner != nullptr)
        m_Owner->SetDirty();
}

bool PdfDataContainer::IsIndirectReferenceAllowed(const PdfObject& obj)
{
    PdfDocument* objDocument;
    if (obj.IsIndirect()
        && (objDocument = obj.GetDocument()) != nullptr
        && m_Owner != nullptr
        && objDocument == m_Owner->GetDocument())
    {
        return true;
    }

    return false;
}

PdfDocument* PdfDataContainer::GetObjectDocument()
{
    return m_Owner == nullptr ? nullptr : m_Owner->GetDocument();
}
