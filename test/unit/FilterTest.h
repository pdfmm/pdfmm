/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef FILTER_TEST_H
#define FILTER_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

/** This test tests the various PdfFilter classes
 */
class FilterTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(FilterTest);
    CPPUNIT_TEST(testFilters);
    CPPUNIT_TEST(testCCITT);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testFilters();

    void testCCITT();

private:
    void TestFilter(PoDoFo::EPdfFilter eFilter, const char* pTestBuffer, const long lTestLength);
};

#endif // FILTER_TEST_H
