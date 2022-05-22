/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfDataProvider.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

PdfDataProvider::PdfDataProvider() { }

PdfDataProvider::~PdfDataProvider() { }

string PdfDataProvider::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}

void PdfDataProvider::ToString(string& str) const
{
    str.clear();
    PdfStringOutputDevice device(str);
    charbuff buffer;
    Write(device, PdfWriteFlags::None, nullptr, buffer);
}
