/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef COLOR_TEST_H
#define COLOR_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class ColorTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ColorTest);
    CPPUNIT_TEST(testDefaultConstructor);
    CPPUNIT_TEST(testGreyConstructor);
    CPPUNIT_TEST(testGreyConstructorInvalid);
    CPPUNIT_TEST(testRGBConstructor);
    CPPUNIT_TEST(testRGBConstructorInvalid);
    CPPUNIT_TEST(testCMYKConstructor);
    CPPUNIT_TEST(testCMYKConstructorInvalid);
    CPPUNIT_TEST(testCopyConstructor);
    CPPUNIT_TEST(testAssignmentOperator);
    CPPUNIT_TEST(testEqualsOperator);
    CPPUNIT_TEST(testHexNames);
    CPPUNIT_TEST(testNamesGeneral);
    CPPUNIT_TEST(testNamesOneByOne);

    CPPUNIT_TEST(testColorGreyConstructor);
    CPPUNIT_TEST(testColorRGBConstructor);
    CPPUNIT_TEST(testColorCMYKConstructor);

    CPPUNIT_TEST(testColorSeparationAllConstructor);
    CPPUNIT_TEST(testColorSeparationNoneConstructor);
    CPPUNIT_TEST(testColorSeparationConstructor);
    CPPUNIT_TEST(testColorCieLabConstructor);

    CPPUNIT_TEST(testRGBtoCMYKConversions);

    CPPUNIT_TEST_SUITE_END();

public:
    virtual void setUp();
    virtual void tearDown();

protected:
    void testDefaultConstructor();
    void testGreyConstructor();
    void testGreyConstructorInvalid();
    void testRGBConstructor();
    void testRGBConstructorInvalid();
    void testCMYKConstructor();
    void testCMYKConstructorInvalid();
    void testCopyConstructor();
    void testAssignmentOperator();
    void testEqualsOperator();
    void testHexNames();
    void testNamesGeneral();
    void testNamesOneByOne();

    void testColorGreyConstructor();
    void testColorRGBConstructor();
    void testColorCMYKConstructor();

    void testColorSeparationAllConstructor();
    void testColorSeparationNoneConstructor();
    void testColorSeparationConstructor();
    void testColorCieLabConstructor();

    void testRGBtoCMYKConversions();

    void testAssignNull();
};

#endif // COLOR_TEST_H
