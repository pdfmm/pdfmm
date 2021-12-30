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
    string_view color = " 1.000 1.000 1.000 rg\n";

    PdfMemDocument doc;
    PdfPage* page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    auto& contents = page->GetOrCreateContents();
    contents.GetStreamForAppending().Set(example);

    CompareStreamContent(contents.GetObject().MustGetStream(), example);

    PdfPainter painter;
    painter.SetCanvas(page);
    painter.SetColor(PdfColor(1.0, 1.0, 1.0));
    painter.FinishDrawing();

    string newContent = (string)example;
    newContent += color;

    CompareStreamContent(contents.GetObject().MustGetStream(), newContent);
}

void CompareStreamContent(PdfObjectStream& stream, const string_view& expected)
{
    size_t length;
    unique_ptr<char[]> buffer;
    stream.GetFilteredCopy(buffer, length);

    REQUIRE(memcpy(buffer.get(), expected.data(), expected.size()) == 0);
}
