/**
 * Copyright (C) 20012 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */


#ifndef DATE_TEST_H
#define DATE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class DateTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DateTest);
    CPPUNIT_TEST(testCreateDateFromString);
    CPPUNIT_TEST(testDateValue);
    CPPUNIT_TEST(testAdditional);
    CPPUNIT_TEST(testParseDateValid);
    CPPUNIT_TEST(testParseDateInvalid);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testCreateDateFromString();
    void testDateValue();
    void testAdditional();
    void testParseDateInvalid();
    void testParseDateValid();
};

#endif // DATE_TEST_H
