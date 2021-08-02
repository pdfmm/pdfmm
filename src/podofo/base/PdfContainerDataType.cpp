/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfContainerDataType.h"

#include <doc/PdfDocument.h>
#include "PdfObject.h"
#include "PdfVecObjects.h"

using namespace PoDoFo;

PdfContainerDataType::PdfContainerDataType()
    : m_pOwner(nullptr), m_isImmutable(false)
{
}

// NOTE: Don't copy owner. Copied objects must be always detached.
// Ownership will be set automatically elsewhere
PdfContainerDataType::PdfContainerDataType( const PdfContainerDataType&rhs )
    : PdfDataType( rhs ), m_pOwner(nullptr), m_isImmutable(false)
{
}

void PdfContainerDataType::ResetDirty()
{
    ResetDirtyInternal();
}

PdfObject & PdfContainerDataType::GetIndirectObject( const PdfReference &ref ) const
{
    if ( m_pOwner == nullptr)
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidHandle, "Object is a reference but does not have an owner" );

    auto document = m_pOwner->GetDocument();
    if (document == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Object owner is not part of any document");

    auto ret = document->GetObjects().GetObject( ref );
    if (ret == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Can't find reference with objnum: " + std::to_string(ref.ObjectNumber()) + ", gennum: " + std::to_string(ref.GenerationNumber()));

    return *ret;
}

void PdfContainerDataType::SetOwner( PdfObject* pOwner )
{
    PODOFO_ASSERT( pOwner != nullptr);
    m_pOwner = pOwner;
}

void PdfContainerDataType::SetDirty()
{
    if (m_pOwner != nullptr)
        m_pOwner->SetDirty();
}

bool PdfContainerDataType::IsIndirectReferenceAllowed(const PdfObject& obj)
{
    PdfDocument* objDocument;
    if (obj.IsIndirect()
        && (objDocument = obj.GetDocument()) != nullptr
        && m_pOwner != nullptr
        && objDocument == m_pOwner->GetDocument())
    {
        return true;
    }

    return false;
}

PdfContainerDataType& PdfContainerDataType::operator=( const PdfContainerDataType& rhs )
{
    // NOTE: Don't copy owner. Objects being assigned will keep current ownership
    PdfDataType::operator=( rhs );
    return *this;
}

PdfDocument * PdfContainerDataType::GetObjectDocument()
{
    return m_pOwner == nullptr ? nullptr : m_pOwner->GetDocument();
}

void PdfContainerDataType::AssertMutable() const
{
    if(IsImmutable())
        PODOFO_RAISE_ERROR( EPdfError::ChangeOnImmutable );
}
