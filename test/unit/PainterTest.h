/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PAINTER_TEST_H
#define PAINTER_TEST_H

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfPage;
class PdfStream;
};

/** This test tests the class PdfPainter
 */
class PainterTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(PainterTest);
    CPPUNIT_TEST(testAppend);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    /**
     * Test if contents are appended correctly
     * to pages with existing contents.
     */
    void testAppend();

private:
    /**
     * Compare the filtered contents of a PdfStream object
     * with a string and assert if the contents do not match!
     *
     * @param pStream PdfStream object
     * @param pszContent expected contents
     */
    void  CompareStreamContent(PoDoFo::PdfStream* pStream, const char* pszExpected);
};

#endif // PAINTER_TEST_H
