/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PARSER_TEST_H
#define PARSER_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

/** This test tests the class PdfParser
 *
 *  PdfParser was responsible for 14% of the PoDoFo CVEs reported up to April 2018
 *  so this class tests CVE fixes along with additional tests to test boundary conditions
 */
class ParserTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ParserTest);
    CPPUNIT_TEST(testMaxObjectCount);
    CPPUNIT_TEST(testReadDocumentStructure);
    CPPUNIT_TEST(testReadXRefContents);
    CPPUNIT_TEST(testReadXRefContents);
    CPPUNIT_TEST(testReadXRefSubsection);
    CPPUNIT_TEST(testReadXRefStreamContents);
    CPPUNIT_TEST(testReadObjects);
    CPPUNIT_TEST(testIsPdfFile);
    CPPUNIT_TEST(testRoundTripIndirectTrailerID);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    // commented out tests still need implemented

    void testMaxObjectCount();

    // CVE-2018-8002
    // testParseFile();

    void testReadDocumentStructure();
    //void testReadTrailer();
    //void testReadXRef();

    // CVE-2017-8053 tested
    void testReadXRefContents();

    // CVE-2015-8981, CVE-2017-5853, CVE-2018-5296 - tested
    // CVE-2017-6844, CVE-2017-5855 - symptoms are false postives due to ASAN allocator_may_return_null=1 not throwing bad_alloc
    void testReadXRefSubsection();

    // CVE-2017-8787, CVE-2018-5295 - tested
    void testReadXRefStreamContents();

    // CVE-2017-8378 - tested
    // CVE-2018-6352 - no fix yet, so no test yet
    void testReadObjects();

    //void testReadObjectFromStream();
    void testIsPdfFile();
    //void testReadNextTrailer();
    //void testCheckEOFMarker();

    void testRoundTripIndirectTrailerID();

private:
    std::string generateXRefEntries(size_t count);
    bool canOutOfMemoryKillUnitTests();
};

#endif // PARSER_TEST_H
