/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfCanvasInputDevice.h"
#include "PdfCanvas.h"

using namespace std;
using namespace mm;

PdfCanvasInputDevice::PdfCanvasInputDevice(const PdfCanvas& canvas)
{
    auto contents = canvas.GetContentsObject();
    if (contents != nullptr)
    {
        if (contents->IsArray())
        {
            auto& contentsArr = contents->GetArray();
            for (unsigned i = 0; i < contentsArr.GetSize(); i++)
            {
                auto& streamObj = contentsArr.FindAt(i);
                m_contents.push_back(&streamObj);
            }
        }
        else if (contents->IsDictionary())
        {
            // NOTE: Pages are allowed to be empty
            if (contents->HasStream())
                m_contents.push_back(contents);
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Page /Contents not stream or array of streams");
        }
    }

    if (m_contents.size() == 0)
    {
        m_eof = true;
    }
    else
    {
        if (tryPopNextDevice())
            m_eof = m_device->Eof();
        else
            m_eof = false;
    }
}

bool PdfCanvasInputDevice::TryGetChar(char& ch)
{
    if (m_eof)
    {
        ch = 0;
        return false;
    }

    PdfInputDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            m_eof = true;
            return false;
        }

        if (device->TryGetChar(ch))
            return true;
    }
}

int PdfCanvasInputDevice::Look()
{
    if (m_eof)
        return EOF;

    PdfInputDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            m_eof = true;
            return EOF;
        }

        int ret = device->Look();
        if (ret != EOF)
            return ret;
    }
}

size_t PdfCanvasInputDevice::Read(char* buffer, size_t size)
{
    if (size == 0 || m_eof)
        return 0;

    size_t readCount = 0;
    PdfInputDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            m_eof = true;
            return readCount;
        }

        // Span reads into multple input devices
        size_t readLocal = device->Read(buffer + readCount, size);
        size -= readLocal;
        readCount += readLocal;

        if (size == 0)
            return readCount;
    }
}

size_t PdfCanvasInputDevice::Tell()
{
    throw runtime_error("Unsupported");
}

bool PdfCanvasInputDevice::tryGetNextDevice(PdfInputDevice*& device)
{
    PDFMM_ASSERT(m_device != nullptr);
    if (device == nullptr)
    {
        device = m_device.get();
        return true;
    }

    if (m_contents.size() == 0)
    {
        device = nullptr;
        return false;
    }

    if (!tryPopNextDevice())
    {
        device = nullptr;
        return false;
    }

    device = m_device.get();
    return true;
}

bool PdfCanvasInputDevice::tryPopNextDevice()
{
    auto contents = m_contents.front()->GetStream();
    m_contents.pop_front();
    if (contents == nullptr)
    {
        m_device = nullptr;
        return false;
    }
    else
    {
        m_device = std::make_unique<PdfMemoryInputDevice>(*contents);
        return true;
    }
}
