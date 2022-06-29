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

PdfOutputStream::~PdfOutputStream() { }

void PdfOutputStream::Write(char ch)
{
    writeBuffer(&ch, 1);
}

void PdfOutputStream::Write(const string_view& view)
{
    if (view.length() == 0)
        return;

    writeBuffer(view.data(), view.size());
}

void PdfOutputStream::Write(const char* buffer, size_t size)
{
    if (size == 0)
        return;

    writeBuffer(buffer, size);
}

void PdfOutputStream::Flush()
{
    flush();
}

void PdfOutputStream::flush()
{
    // Do nothing
}
