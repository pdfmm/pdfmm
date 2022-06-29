/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfInputStream.h"

#include "PdfOutputStream.h"

using namespace std;
using namespace mm;

PdfInputStream::~PdfInputStream() { }

bool PdfInputStream::Read(char& ch, bool& eof)
{
    return readChar(ch, eof);
}

size_t PdfInputStream::Read(char* buffer, size_t size, bool& eof)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    return readBuffer(buffer, size, eof);
}

void PdfInputStream::CopyTo(PdfOutputStream& stream)
{
    constexpr size_t BUFFER_SIZE = 4096;
    size_t len = 0;
    char buffer[BUFFER_SIZE];

    bool eof;
    do
    {
        len = Read(buffer, BUFFER_SIZE, eof);
        stream.Write(buffer, len);
    } while (!eof);

    stream.Flush();
}

size_t PdfInputStream::ReadBuffer(PdfInputStream& stream, char* buffer, size_t size, bool& eof)
{
    return stream.readBuffer(buffer, size, eof);
}

bool PdfInputStream::ReadChar(PdfInputStream& stream, char& ch, bool& eof)
{
    return stream.readChar(ch, eof);
}

bool PdfInputStream::readChar(char& ch, bool& eof)
{
    ch = '\0';
    return readBuffer(&ch, 1, eof) == 1;
}
