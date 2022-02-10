/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace mm;

static void CompareStreamContent(PdfObjectStream& stream, const string_view& expected);

TEST_CASE("testAppend")
{
    string_view example = "BT (Hallo) Tj ET";

    PdfMemDocument doc;
    PdfPage* page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    auto& contents = page->GetOrCreateContents();
    auto& stream = contents.GetStreamForAppending();
    stream.Set(example);

    CompareStreamContent(stream, example);

    PdfPainter painter;
    painter.SetCanvas(page);
    painter.SetColor(PdfColor(1.0, 1.0, 1.0));
    painter.FinishDrawing();

    PdfCanvasInputDevice device(*page);
    string out;
    char buffer[4096];
    while (!device.Eof())
    {
        size_t read = device.Read(buffer, 4096);
        out.append(buffer, read);
    }

    REQUIRE(out == "q\nBT (Hallo) Tj ET\nQ\nq\n1.000 1.000 1.000 rg\nQ\n");
}

void CompareStreamContent(PdfObjectStream& stream, const string_view& expected)
{
    size_t length;
    unique_ptr<char[]> buffer;
    stream.GetFilteredCopy(buffer, length);

    REQUIRE(memcmp(buffer.get(), expected.data(), expected.size()) == 0);
}
