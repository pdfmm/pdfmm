/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CANVAS_INPUT_DEVICE_H
#define PDF_CANVAS_INPUT_DEVICE_H

#include "PdfInputDevice.h"

#include <list>

namespace PoDoFo {

class PdfCanvas;
class PdfObject;

/**
 * There are Pdfs spanning delimiters or begin/end tags into
 * streams. Let's create a device correctly spanning I/O
 * reads into these
 */
class PdfCanvasInputDevice : public PdfInputDevice
{
public:
    PdfCanvasInputDevice(PdfCanvas& canvas);
public:
    bool TryGetChar(char& ch) override;
    size_t Tell() override;
    int Look() override;
    size_t Read(char* buffer, size_t size) override;
    bool Eof() const override { return m_eof; }
    bool IsSeekable() const override { return false; }
private:
    bool tryGetNextDevice(PdfInputDevice*& device);
    void popNextDevice();
private:
    bool m_eof;
    std::list<PdfObject*> m_lstContents;
    std::unique_ptr<PdfInputDevice> m_device;
};

}

#endif // PDF_CANVAS_INPUT_DEVICE_H
