/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
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

bool InputStreamDevice::Peek(char& ch) const
{
    EnsureAccess(DeviceAccess::Read);
    return peek(ch);
}

void InputStreamDevice::checkRead() const
{
    EnsureAccess(DeviceAccess::Read);
}
