/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfOutputStream.h"

using namespace std;
using namespace mm;

OutputStream::~OutputStream() { }

void OutputStream::Write(char ch)
{
    checkWrite();
    writeBuffer(&ch, 1);
}

void OutputStream::Write(const string_view& view)
{
    if (view.length() == 0)
        return;

    checkWrite();
    writeBuffer(view.data(), view.size());
}

void OutputStream::Write(const char* buffer, size_t size)
{
    if (size == 0)
        return;

    checkWrite();
    writeBuffer(buffer, size);
}

void OutputStream::Flush()
{
    flush();
}

void OutputStream::flush()
{
    // Do nothing
}

void OutputStream::checkWrite() const
{
    // Do nothing
}
