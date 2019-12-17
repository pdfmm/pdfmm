/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfContainerDataType.h"

#include <string>
#include <cassert>
#include "PdfObject.h"
#include "PdfVecObjects.h"

using namespace PoDoFo;

PdfContainerDataType::PdfContainerDataType()
    : m_pOwner(nullptr), m_isImmutable(false), m_isDirty(false)
{
}

// NOTE: Don't copy owner. Copied objects must be always detached.
// Ownership will be set automatically elsewhere
PdfContainerDataType::PdfContainerDataType( const PdfContainerDataType&rhs )
    : PdfDataType( rhs ), m_pOwner(nullptr), m_isImmutable(false), m_isDirty(false)
{
}

void PdfContainerDataType::ResetDirty()
{
    m_isDirty = false;
    ResetDirtyInternal();
}

PdfObject & PdfContainerDataType::GetIndirectObject( const PdfReference &ref ) const
{
    if ( m_pOwner == nullptr)
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidHandle, "Object is a reference but does not have an owner!" );

    auto ret = m_pOwner->GetOwner()->GetObject( ref );
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
    m_isDirty = true;
}

PdfContainerDataType& PdfContainerDataType::operator=( const PdfContainerDataType& rhs )
{
    // NOTE: Don't copy owner. Objects being assigned will keep current ownership
    m_isDirty = true;
    PdfDataType::operator=( rhs );
    return *this;
}

PdfVecObjects * PdfContainerDataType::GetObjectOwner()
{
    return m_pOwner == nullptr ? nullptr : m_pOwner->GetOwner();
}

void PdfContainerDataType::AssertMutable() const
{
    if(IsImmutable()) 
    {
        PODOFO_RAISE_ERROR( EPdfError::ChangeOnImmutable );
    }
}
