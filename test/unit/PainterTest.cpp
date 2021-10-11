/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PainterTest.h"
#include "TestUtils.h"

#include <podofo.h>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(PainterTest);

void PainterTest::setUp()
{

}

void PainterTest::tearDown()
{
}

void PainterTest::CompareStreamContent(PdfStream* pStream, const char* pszExpected)
{
    char* pBuffer;
    pdf_long lLen;
    pStream->GetFilteredCopy(&pBuffer, &lLen);

    std::string str(pBuffer, lLen);
    CPPUNIT_ASSERT_EQUAL(std::string(pszExpected), str);

    free(pBuffer);
}

void PainterTest::testAppend()
{
    const char* pszExample1 = "BT (Hallo) Tj ET";
    const char* pszColor = " 1.000 1.000 1.000 rg\n";

    PdfMemDocument doc;
    PdfPage* pPage = doc.CreatePage(PdfPage::CreateStandardPageSize(ePdfPageSize_A4));
    pPage->GetContents()->GetStream()->Set(pszExample1);

    this->CompareStreamContent(pPage->GetContents()->GetStream(), pszExample1);

    PdfPainter painter;
    painter.SetCanvas(pPage);
    painter.SetColor(PdfColor(1.0, 1.0, 1.0));
    painter.FinishDrawing();

    std::string newContent = pszExample1;
    newContent += pszColor;

    this->CompareStreamContent(pPage->GetContents()->GetStream(), newContent.c_str());
}
