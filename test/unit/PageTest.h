/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PAGE_TEST_H
#define PAGE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfPage;
};

/** This test tests the class PdfPagesTree
 */
class PageTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(PageTest);
    CPPUNIT_TEST(testEmptyContents);
    CPPUNIT_TEST(testEmptyContentsStream);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testEmptyContents();
    void testEmptyContentsStream();
};

#endif // PAGE_TEST_H


