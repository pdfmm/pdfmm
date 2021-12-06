/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfReference.h"

#include "PdfOutputDevice.h"

#include <sstream>

using namespace std;
using namespace mm;

PdfReference::PdfReference()
    : m_ObjectNo(0), m_GenerationNo(0)
{
}

PdfReference::PdfReference(const uint32_t objectNo, const uint16_t generationNo)
    : m_ObjectNo(objectNo), m_GenerationNo(generationNo)
{
}

void PdfReference::Write(PdfOutputDevice& device, PdfWriteMode writeMode, const PdfEncrypt* encrypt) const
{
    (void)encrypt;
    if ((writeMode & PdfWriteMode::NoInlineLiteral) == PdfWriteMode::None)
        device.Put(' '); // Write space before the reference

    device.Write(PDFMM_FORMAT("{} {} R", m_ObjectNo, m_GenerationNo));
}

const string PdfReference::ToString() const
{
    ostringstream out;
    out << m_ObjectNo << " " << m_GenerationNo << " R";
    return out.str();
}

bool PdfReference::operator<(const PdfReference& rhs) const
{
    return m_ObjectNo == rhs.m_ObjectNo ? m_GenerationNo < rhs.m_GenerationNo : m_ObjectNo < rhs.m_ObjectNo;
}

bool PdfReference::operator==(const PdfReference& rhs) const
{
    return m_ObjectNo == rhs.m_ObjectNo && m_GenerationNo == rhs.m_GenerationNo;
}

bool PdfReference::operator!=(const PdfReference& rhs) const
{
    return m_ObjectNo != rhs.m_ObjectNo || m_GenerationNo != rhs.m_GenerationNo;
}

bool PdfReference::IsIndirect() const
{
    return m_ObjectNo != 0 || m_GenerationNo != 0;
}
