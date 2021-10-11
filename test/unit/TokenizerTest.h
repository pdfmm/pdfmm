/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef TOKENIZER_TEST_H
#define TOKENIZER_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

/** This test tests the class PdfTokenizer
 *
 *  Currently the following methods are tested
 *  - void PdfTokenizer::GetNextVariant( PdfVariant& rVariant, PdfEncrypt* pEncrypt );
 *  - bool PdfTokenizer::GetNextToken( const char *& pszToken, EPdfTokenType* peType = NULL);
 *  - void PdfTokenizer::IsNextToken( const char* pszToken );
 */
class TokenizerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TokenizerTest);
    CPPUNIT_TEST(testArrays);
    CPPUNIT_TEST(testBool);
    CPPUNIT_TEST(testHexString);
    CPPUNIT_TEST(testName);
    CPPUNIT_TEST(testNull);
    CPPUNIT_TEST(testNumbers);
    CPPUNIT_TEST(testReference);
    CPPUNIT_TEST(testString);
    CPPUNIT_TEST(testTokens);
    CPPUNIT_TEST(testComments);
    CPPUNIT_TEST(testDictionary);
    CPPUNIT_TEST(testLocale);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testArrays();
    void testBool();
    void testHexString();
    void testName();
    void testNull();
    void testNumbers();
    void testReference();
    void testString();
    void testDictionary();

    void testTokens();
    void testComments();

    void testLocale();

private:
    void Test(const char* pszString, PoDoFo::EPdfDataType eDataType, const char* pszExpected = NULL);

    /** Test parsing a stream.
     *
     *  \param pszBuffer a string buffer that will be parsed
     *  \param pszTokens a NULL terminated list of all tokens in the
     *                   order PdfTokenizer should read them from pszBuffer
     */
    void TestStream(const char* pszBuffer, const char* pszTokens[]);

    /** Test parsing a stream. As above but this time using PdfTokenizer::IsNextToken()
     *
     *  \param pszBuffer a string buffer that will be parsed
     *  \param pszTokens a NULL terminated list of all tokens in the
     *                   order PdfTokenizer should read them from pszBuffer
     */
    void TestStreamIsNextToken(const char* pszBuffer, const char* pszTokens[]);
};

#endif // TOKENIZER_TEST_H
