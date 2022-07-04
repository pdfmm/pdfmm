/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfInputDevice.h"

using namespace std;
using namespace mm;

InputStreamDevice::InputStreamDevice()
    : InputStreamDevice(true) { }

InputStreamDevice::InputStreamDevice(bool init)
{
    if (init)
        SetAccess(DeviceAccess::Read);
}

int InputStreamDevice::Peek() const
{
    EnsureAccess(DeviceAccess::Read);
    char ch;
    if (peek(ch))
        return ch;
    else
        return -1;
}

bool InputStreamDevice::Peek(char& ch) const
{
    EnsureAccess(DeviceAccess::Read);
    return peek(ch);
}

size_t InputStreamDevice::readBuffer(char* buffer, size_t size, bool& eof)
{
    EnsureAccess(DeviceAccess::Read);
    return readBufferImpl(buffer, size, eof);
}

bool InputStreamDevice::readChar(char& ch)
{
    EnsureAccess(DeviceAccess::Read);
    return readCharImpl(ch);
}
