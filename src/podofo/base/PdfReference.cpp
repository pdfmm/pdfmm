/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfReference.h"

#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

#include <sstream>

using namespace PoDoFo;

void PdfReference::Write(PdfOutputDevice& pDevice, PdfWriteMode eWriteMode, const PdfEncrypt* ) const
{
    if( (eWriteMode & PdfWriteMode::Compact) == PdfWriteMode::Compact )
    {
        // Write space before the reference
        pDevice.Print( " %i %hi R", m_nObjectNo, m_nGenerationNo );
    }
    else
    {
        pDevice.Print( "%i %hi R", m_nObjectNo, m_nGenerationNo );
    }
}

const std::string PdfReference::ToString() const
{
    std::ostringstream out;
    out << m_nObjectNo << " " << m_nGenerationNo << " R";
    return out.str();
}

const PdfReference& PdfReference::operator=(const PdfReference& rhs)
{
    m_nObjectNo = rhs.m_nObjectNo;
    m_nGenerationNo = rhs.m_nGenerationNo;
    return *this;
}

bool PdfReference::operator<(const PdfReference& rhs) const
{
    return m_nObjectNo == rhs.m_nObjectNo ? m_nGenerationNo < rhs.m_nGenerationNo : m_nObjectNo < rhs.m_nObjectNo;
}

bool PdfReference::operator==(const PdfReference& rhs) const
{
    return m_nObjectNo == rhs.m_nObjectNo && m_nGenerationNo == rhs.m_nGenerationNo;
}

bool PdfReference::operator!=(const PdfReference& rhs) const
{
    return !this->operator==(rhs);
}

bool PdfReference::IsIndirect() const
{
    return m_nObjectNo != 0 || m_nGenerationNo != 0;
}
