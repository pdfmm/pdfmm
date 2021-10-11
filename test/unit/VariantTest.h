/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef VARIANT_TEST_H
#define VARIANT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

 /** This test tests the class PdfVariant
  */
class VariantTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(VariantTest);
    CPPUNIT_TEST(testEmptyObject);
    CPPUNIT_TEST(testEmptyStream);
    CPPUNIT_TEST(testNameObject);
    CPPUNIT_TEST(testIsDirtyTrue);
    CPPUNIT_TEST(testIsDirtyFalse);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testEmptyObject();
    void testEmptyStream();
    void testNameObject();

    void testIsDirtyTrue();
    void testIsDirtyFalse();

private:
};

#endif // VARIANT_TEST_H
