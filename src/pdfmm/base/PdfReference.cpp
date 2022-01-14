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

void PdfReference::Write(PdfOutputDevice& device, PdfWriteFlags writeMode, const PdfEncrypt* encrypt) const
{
    (void)encrypt;
    if ((writeMode & PdfWriteFlags::NoInlineLiteral) == PdfWriteFlags::None)
        device.Put(' '); // Write space before the reference

    device.Write(cmn::Format("{} {} R", m_ObjectNo, m_GenerationNo));
}

string PdfReference::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}

void PdfReference::ToString(string& str) const
{
    str.clear();
    cmn::FormatTo(str, "{} {} R", m_ObjectNo, m_GenerationNo);
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
