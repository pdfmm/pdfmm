/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CANVAS_INPUT_DEVICE_H
#define PDF_CANVAS_INPUT_DEVICE_H

#include "PdfInputDevice.h"

#include <list>

namespace mm {

class PdfCanvas;
class PdfObject;

/**
 * There are Pdfs spanning delimiters or begin/end tags into
 * contents streams. Let's create a device correctly spanning
 * I/O reads into these
 */
class PDFMM_API PdfCanvasInputDevice : public PdfInputDevice
{
public:
    PdfCanvasInputDevice(const PdfCanvas& canvas);
public:
    size_t GetPosition() override;
    int Peek() override;
    bool Eof() const override { return m_eof; }
private:
    bool tryGetNextDevice(PdfInputDevice*& device);
    bool tryPopNextDevice();
    void setEOF();
protected:
    size_t readBufferImpl(char* buffer, size_t size) override;
    bool readCharImpl(char& ch) override;
private:
    bool m_eof;
    std::list<const PdfObject*> m_contents;
    charbuff m_buffer;
    std::unique_ptr<PdfInputDevice> m_device;
    bool m_deviceSwitchOccurred;
};

}

#endif // PDF_CANVAS_INPUT_DEVICE_H
