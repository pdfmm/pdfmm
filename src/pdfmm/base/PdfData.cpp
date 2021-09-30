/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfData.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

PdfData::PdfData()
    : m_data(std::make_shared<string>())
{
}

PdfData::PdfData(const string_view& data, const shared_ptr<size_t>& writeBeacon)
    : m_data(std::make_shared<string>(data)), m_writeBeacon(writeBeacon)
{
}

PdfData::PdfData(string&& data, const shared_ptr<size_t>& writeBeacon)
    : m_data(std::make_shared<string>(std::move(data))), m_writeBeacon(writeBeacon)
{
}

PdfData::PdfData(const PdfData& rhs)
{
    this->operator=(rhs);
}

void PdfData::Write(PdfOutputDevice& device, PdfWriteMode, const PdfEncrypt* encrypt) const
{
    (void)encrypt;
    if (m_writeBeacon != nullptr)
        *m_writeBeacon = device.Tell();

    device.Write(*m_data);
}

const PdfData& PdfData::operator=(const PdfData& rhs)
{
    m_data = rhs.m_data;
    m_writeBeacon = rhs.m_writeBeacon;
    return *this;
}
