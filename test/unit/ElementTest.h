/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef ELEMENT_TEST_H
#define ELEMENT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

class ElementTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ElementTest);
    CPPUNIT_TEST(testTypeToIndexAnnotation);
    CPPUNIT_TEST(testTypeToIndexAction);
    CPPUNIT_TEST(testTypeToIndexAnnotationUnknown);
    CPPUNIT_TEST(testTypeToIndexActionUnknown);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testTypeToIndexAnnotation();
    void testTypeToIndexAction();
    void testTypeToIndexAnnotationUnknown();
    void testTypeToIndexActionUnknown();

private:
};

#endif // ELEMENT_TEST_H
