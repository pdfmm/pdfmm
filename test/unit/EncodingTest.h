/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef ENCODING_TEST_H
#define ENCODING_TEST_H

#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfEncoding;
};

/** This test tests the various class PdfEncoding classes
 */
class EncodingTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(EncodingTest);
    CPPUNIT_TEST(testDifferences);
    CPPUNIT_TEST(testDifferencesEncoding);
    CPPUNIT_TEST(testDifferencesObject);
    CPPUNIT_TEST(testUnicodeNames);
    CPPUNIT_TEST(testGetCharCode);
    CPPUNIT_TEST(testToUnicodeParse);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testDifferences();
    void testDifferencesObject();
    void testDifferencesEncoding();
    void testUnicodeNames();
    void testGetCharCode();
    void testToUnicodeParse();

private:

    bool outofRangeHelper(PoDoFo::PdfEncoding* pEncoding, std::string& rMsg, const char* pszName);
};

#endif // ENCODING_TEST_H
