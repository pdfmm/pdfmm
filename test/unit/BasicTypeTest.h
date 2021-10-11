/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef BASICTYPE_TEST_H
#define BASICTYPE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/** This class tests the basic integer and other types PoDoFo uses
 *  to make sure they satisfy its requirements for behaviour, size, etc.
 *
 *  We now detect what types to use using CMake, so it's important to
 *  test that detection and make sure it's doing the right thing.
 */
class BasicTypeTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(BasicTypeTest);
    CPPUNIT_TEST(testXrefOffsetTypeSize);
    CPPUNIT_TEST(testDefaultMaximumNumberOfObjects);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testXrefOffsetTypeSize();
    void testDefaultMaximumNumberOfObjects();

private:
};

#endif // BASICTYPE_TEST_H
