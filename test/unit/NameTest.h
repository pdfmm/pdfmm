/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef NAME_TEST_H
#define NAME_TEST_H

#include <cppunit/extensions/HelperMacros.h>

/** This test tests the class PdfName
 */
class NameTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(NameTest);
    CPPUNIT_TEST(testParseAndWrite);
    CPPUNIT_TEST(testNameEncoding);
    CPPUNIT_TEST(testEncodedNames);
    CPPUNIT_TEST(testEquality);
    CPPUNIT_TEST(testWrite);
    CPPUNIT_TEST(testFromEscaped);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testParseAndWrite();
    void testNameEncoding();
    void testEncodedNames();
    void testEquality();
    void testWrite();
    void testFromEscaped();

private:

    void TestName(const char* pszString, const char* pszExpectedEncoded);
    void TestEncodedName(const char* pszString, const char* pszExpected);

    /** Tests if both names are equal
     */
    void TestNameEquality(const char* pszName1, const char* pszName2);

    /** Test if pszName interpreted as PdfName and written
     *  to a PdfOutputDevice equals pszResult
     */
    void TestWrite(const char* pszName, const char* pszResult);

    void TestFromEscape(const char* pszName1, const char* pszName2);
};

#endif // NAME_TEST_H
