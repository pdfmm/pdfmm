/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfData.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

PdfData::PdfData() { }

PdfData::PdfData(chars&& data, const shared_ptr<size_t>& writeBeacon)
    : m_data(std::move(data)), m_writeBeacon(writeBeacon)
{
}

PdfData::PdfData(const cspan<char>& data, const shared_ptr<size_t>& writeBeacon)
    : m_data(chars(data)), m_writeBeacon(writeBeacon)
{
}

PdfData& PdfData::operator=(const cspan<char>& data)
{
    m_data = data;
    return *this;
}

void PdfData::Write(PdfOutputDevice& device, PdfWriteMode, const PdfEncrypt* encrypt) const
{
    (void)encrypt;
    if (m_writeBeacon != nullptr)
        *m_writeBeacon = device.Tell();

    device.Write(m_data);
}
