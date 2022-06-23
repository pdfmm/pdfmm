/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfOutputStream.h"

#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

void PdfOutputStream::Write(const string_view& view)
{
    Write(view.data(), view.length());
}

PdfOutputStream::~PdfOutputStream() { }

void PdfOutputStream::Write(const char* buffer, size_t len)
{
    if (len == 0)
        return;

    WriteImpl(buffer, len);
}

PdfMemoryOutputStream::PdfMemoryOutputStream(size_t initialCapacity)
{
    m_Buffer.reserve(initialCapacity);
}

void PdfMemoryOutputStream::WriteImpl(const char* data, size_t len)
{
    m_Buffer.append(data, len);
}

void PdfMemoryOutputStream::Close()
{
    // Do nothing
}

charbuff PdfMemoryOutputStream::TakeBuffer()
{
    return std::move(m_Buffer);
}

PdfDeviceOutputStream::PdfDeviceOutputStream(PdfOutputDevice& device)
    : m_Device(&device) { }

void PdfDeviceOutputStream::WriteImpl(const char* data, size_t len)
{
    m_Device->Write(string_view(data, len));
}

void PdfDeviceOutputStream::Close()
{
    // Do nothing
}
