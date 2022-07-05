/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfOutputDevice.h"

using namespace mm;

OutputStreamDevice::OutputStreamDevice()
    : OutputStreamDevice(true) { }

OutputStreamDevice::OutputStreamDevice(bool init)
{
    if (init)
        SetAccess(DeviceAccess::Write);
}

void OutputStreamDevice::checkWrite() const
{
    EnsureAccess(DeviceAccess::Write);
}
