/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef STRING_TEST_H
#define STRING_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#ifndef __clang__

/** This test tests the class PdfString
 */
class StringTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(StringTest);
    CPPUNIT_TEST(testLibUnistringSimple);
    CPPUNIT_TEST(testLibUnistringUtf8);
    CPPUNIT_TEST(testGetStringUtf8);
    CPPUNIT_TEST(testUtf16beContructor);
    CPPUNIT_TEST(testWCharConstructor);
    CPPUNIT_TEST(testEscapeBrackets);
    CPPUNIT_TEST(testWriteEscapeSequences);
    CPPUNIT_TEST(testEmptyString);
    CPPUNIT_TEST(testInitFromUtf8);
    CPPUNIT_TEST_SUITE_END();


public:
    void setUp();
    void tearDown();

    void testLibUnistringSimple();
    void testLibUnistringUtf8();
    void testGetStringUtf8();
    void testUtf16beContructor();
    void testWCharConstructor();
    void testEscapeBrackets();
    void testWriteEscapeSequences();
    void testEmptyString();
    void testInitFromUtf8();

private:
    void TestWriteEscapeSequences(const char* pszSource, const char* pszExpected);
    void TestLibUnistringInternal(const char* pszString, const long lLenUtf8, const long lLenUtf16);
};

#endif // __clang__

#endif // STRING_TEST_H
