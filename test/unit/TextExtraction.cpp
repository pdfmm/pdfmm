/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include "TestUtils.h"

using namespace std;
using namespace mm;

TEST_CASE("TextExtraction1")
{
    PdfMemDocument doc;
    doc.Load(TestUtils::GetTestInputFilePath("TextExtraction1.pdf"));
    auto& page = doc.GetPages().GetPage(0);
    vector<PdfTextEntry> entries;
    page.ExtractTextTo(entries);

    REQUIRE(entries[0].Text == "MATLAB (an abbreviation of \"matrix laboratory\") is a proprietary multi-paradigm programming");
    ASSERT_EQUAL(entries[0].X, 29.000000232);
    ASSERT_EQUAL(entries[0].Y, 694.943905559551);
    REQUIRE(entries[1].Text == "language and numerical computing environment developed by MathWorks. MATLAB allows matrix");
    ASSERT_EQUAL(entries[1].X, 29.000000232);
    ASSERT_EQUAL(entries[1].Y, 684.920205479362);
    REQUIRE(entries[11].Text == "interfaces, and interfacing with programs written in other languages.");
    ASSERT_EQUAL(entries[11].X, 29.000000232);
    ASSERT_EQUAL(entries[11].Y, 664.872605318981);
}
