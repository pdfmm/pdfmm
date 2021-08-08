/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfDataContainer.h"

#include "PdfDocument.h"
#include "PdfObject.h"
#include "PdfIndirectObjectList.h"

using namespace mm;

PdfDataContainer::PdfDataContainer()
    : m_Owner(nullptr), m_IsImmutable(false)
{
}

// NOTE: Don't copy owner. Copied objects must be always detached.
// Ownership will be set automatically elsewhere
PdfDataContainer::PdfDataContainer(const PdfDataContainer& rhs)
    : PdfDataProvider(rhs), m_Owner(nullptr), m_IsImmutable(false)
{
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
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Can't find reference with objnum: " + std::to_string(ref.ObjectNumber()) + ", gennum: " + std::to_string(ref.GenerationNumber()));

    return *ret;
}

void PdfDataContainer::SetOwner(PdfObject* owner)
{
    PDFMM_ASSERT(owner != nullptr);
    m_Owner = owner;
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

PdfDataContainer& PdfDataContainer::operator=(const PdfDataContainer& rhs)
{
    // NOTE: Don't copy owner. Objects being assigned will keep current ownership
    PdfDataProvider::operator=(rhs);
    return *this;
}

PdfDocument* PdfDataContainer::GetObjectDocument()
{
    return m_Owner == nullptr ? nullptr : m_Owner->GetDocument();
}

void PdfDataContainer::AssertMutable() const
{
    if (IsImmutable())
        PDFMM_RAISE_ERROR(PdfErrorCode::ChangeOnImmutable);
}
