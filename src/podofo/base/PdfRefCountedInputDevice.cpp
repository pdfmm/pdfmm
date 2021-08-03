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
    : m_pDevice(nullptr)
{

}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(const string_view& filename)
    : m_pDevice(nullptr)
{
    m_pDevice = new TRefCountedInputDevice();
    m_pDevice->m_lRefCount = 1;

    try
    {
        m_pDevice->m_pDevice = new PdfInputDevice(filename);
    }
    catch (PdfError& rError)
    {
        delete m_pDevice;
        throw rError;
    }
}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(const char* pBuffer, size_t lLen)
    : m_pDevice(nullptr)
{
    m_pDevice = new TRefCountedInputDevice();
    m_pDevice->m_lRefCount = 1;


    try
    {
        m_pDevice->m_pDevice = new PdfInputDevice(pBuffer, lLen);
    }
    catch (PdfError& rError)
    {
        delete m_pDevice;
        throw rError;
    }
}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(PdfInputDevice* pDevice)
    : m_pDevice(nullptr)
{
    m_pDevice = new TRefCountedInputDevice();
    m_pDevice->m_lRefCount = 1;
    m_pDevice->m_pDevice = pDevice;
}

PdfRefCountedInputDevice::PdfRefCountedInputDevice(const PdfRefCountedInputDevice& rhs)
    : m_pDevice(nullptr)
{
    this->operator=(rhs);
}

PdfRefCountedInputDevice::~PdfRefCountedInputDevice()
{
    Detach();
}

void PdfRefCountedInputDevice::Detach()
{
    if (m_pDevice && !--m_pDevice->m_lRefCount)
    {
        // last owner of the file!
        m_pDevice->m_pDevice->Close();
        delete m_pDevice->m_pDevice;
        delete m_pDevice;
        m_pDevice = nullptr;
    }

}

const PdfRefCountedInputDevice& PdfRefCountedInputDevice::operator=(const PdfRefCountedInputDevice& rhs)
{
    Detach();

    m_pDevice = rhs.m_pDevice;
    if (m_pDevice)
        m_pDevice->m_lRefCount++;

    return *this;
}

PdfInputDevice* PdfRefCountedInputDevice::Device() const
{
    return m_pDevice ? m_pDevice->m_pDevice : nullptr;
}
