/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfRefCountedInputDevice.h"  

#include "PdfInputDevice.h"
#include "PdfDefinesPrivate.h"

using namespace std;
using namespace PoDoFo;

PdfRefCountedInputDevice::PdfRefCountedInputDevice()
    : m_Device(nullptr)
{

}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(const string_view& filename)
    : m_Device(nullptr)
{
    m_Device = new TRefCountedInputDevice();
    m_Device->RefCount = 1;

    try
    {
        m_Device->Device = new PdfInputDevice(filename);
    }
    catch (PdfError& rError)
    {
        delete m_Device;
        throw rError;
    }
}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(const char* buffer, size_t len)
    : m_Device(nullptr)
{
    m_Device = new TRefCountedInputDevice();
    m_Device->RefCount = 1;


    try
    {
        m_Device->Device = new PdfInputDevice(buffer, len);
    }
    catch (PdfError& rError)
    {
        delete m_Device;
        throw rError;
    }
}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(PdfInputDevice* device)
    : m_Device(nullptr)
{
    m_Device = new TRefCountedInputDevice();
    m_Device->RefCount = 1;
    m_Device->Device = device;
}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(const PdfRefCountedInputDevice& rhs)
    : m_Device(nullptr)
{
    this->operator=(rhs);
}

PdfRefCountedInputDevice::~PdfRefCountedInputDevice()
{
    Detach();
}

void PdfRefCountedInputDevice::Detach()
{
    if (m_Device && !--m_Device->RefCount)
    {
        // last owner of the file!
        m_Device->Device->Close();
        delete m_Device->Device;
        delete m_Device;
        m_Device = nullptr;
    }

}

const PdfRefCountedInputDevice& PdfRefCountedInputDevice::operator=(const PdfRefCountedInputDevice& rhs)
{
    Detach();

    m_Device = rhs.m_Device;
    if (m_Device)
        m_Device->RefCount++;

    return *this;
}

PdfInputDevice* PdfRefCountedInputDevice::Device() const
{
    return m_Device ? m_Device->Device : nullptr;
}
