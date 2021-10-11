/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PageTest.h"
#include "TestUtils.h"

#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(PageTest);

void PageTest::setUp()
{
}

void PageTest::tearDown()
{
}

void PageTest::testEmptyContents()
{
    PdfVecObjects vecObjects;
    PdfObject object(PdfReference(1, 0), "Page");
    vecObjects.push_back(&object);

    const std::deque<PdfObject*> parents;
    PdfPage page(&object, parents);
    CPPUNIT_ASSERT(NULL != page.GetContents());

}

void PageTest::testEmptyContentsStream()
{
    PdfMemDocument doc;
    PdfPage* pPage = doc.CreatePage(PdfPage::CreateStandardPageSize(ePdfPageSize_A4));
    PdfAnnotation* pAnnot = pPage->CreateAnnotation(ePdfAnnotation_Popup, PdfRect(300.0, 20.0, 250.0, 50.0));
    PdfString      sTitle("Author: Dominik Seichter");
    pAnnot->SetContents(sTitle);
    pAnnot->SetOpen(true);

    std::string sFilename = TestUtils::getTempFilename();
    doc.Write(sFilename.c_str());

    // Read annotation again
    PdfMemDocument doc2(sFilename.c_str());
    CPPUNIT_ASSERT_EQUAL(1, doc2.GetPageCount());
    PdfPage* pPage2 = doc2.GetPage(0);
    CPPUNIT_ASSERT(NULL != pPage2);
    CPPUNIT_ASSERT_EQUAL(1, pPage2->GetNumAnnots());
    PdfAnnotation* pAnnot2 = pPage2->GetAnnotation(0);
    CPPUNIT_ASSERT(NULL != pAnnot2);
    CPPUNIT_ASSERT(sTitle == pAnnot2->GetContents());

    PdfObject* pPageObject = pPage2->GetObject();
    CPPUNIT_ASSERT(!pPageObject->GetDictionary().HasKey("Contents"));

    TestUtils::deleteFile(sFilename.c_str());
}

