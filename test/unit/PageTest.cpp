/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include "TestUtils.h"

using namespace std;
using namespace mm;

TEST_CASE("testEmptyContentsStream")
{
    PdfMemDocument doc;
    PdfPage* page1 = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    PdfAnnotation* annot1 = page1->CreateAnnotation(PdfAnnotationType::Popup, PdfRect(300.0, 20.0, 250.0, 50.0));
    PdfString title("Author: Dominik Seichter");
    annot1->SetContents(title);
    annot1->SetOpen(true);

    string filename = TestUtils::getTempFilename();
    doc.Write(filename);

    // Read annotation again
    PdfMemDocument doc2;
    doc2.Load(filename);
    REQUIRE(doc2.GetPageTree().GetPageCount() == 1);
    auto& page2 = doc2.GetPageTree().GetPage(0);
    REQUIRE(page2.GetAnnotationCount() == 1);
    PdfAnnotation* annot2 = page2.GetAnnotation(0);
    REQUIRE(annot2 != nullptr);
    REQUIRE(annot2->GetContents() == title);

    auto& pageObj = page2.GetObject();
    REQUIRE(!pageObj.GetDictionary().HasKey("Contents"));

    TestUtils::deleteFile(filename);
}
