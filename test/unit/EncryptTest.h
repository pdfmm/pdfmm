/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef ENCRYPT_TEST_H
#define ENCRYPT_TEST_H

#include <podofo.h>
#include <cppunit/extensions/HelperMacros.h>

namespace PoDoFo {
class PdfEncrypt;
}

/** This test tests the class PdfString
 */
class EncryptTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(EncryptTest);
    CPPUNIT_TEST(testDefault);
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
    CPPUNIT_TEST(testRC4);
    CPPUNIT_TEST(testRC4v2_40);
    CPPUNIT_TEST(testRC4v2_56);
    CPPUNIT_TEST(testRC4v2_80);
    CPPUNIT_TEST(testRC4v2_96);
    CPPUNIT_TEST(testRC4v2_128);
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
    CPPUNIT_TEST(testAESV2);
#ifdef PODOFO_HAVE_LIBIDN
    CPPUNIT_TEST(testAESV3);
#endif // PODOFO_HAVE_LIBIDN
    CPPUNIT_TEST(testLoadEncrypedFilePdfParser);
    CPPUNIT_TEST(testLoadEncrypedFilePdfMemDocument);
    CPPUNIT_TEST(testEnableAlgorithms);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testDefault();
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
    void testRC4();
    void testRC4v2_40();
    void testRC4v2_56();
    void testRC4v2_80();
    void testRC4v2_96();
    void testRC4v2_128();
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
    void testAESV2();
#ifdef PODOFO_HAVE_LIBIDN
    void testAESV3();
#endif // PODOFO_HAVE_LIBIDN

    void testLoadEncrypedFilePdfParser();
    void testLoadEncrypedFilePdfMemDocument();

    void testEnableAlgorithms();

private:
    void TestAuthenticate(PoDoFo::PdfEncrypt* pEncrypt, int keyLength, int rValue);
    void TestEncrypt(PoDoFo::PdfEncrypt* pEncrypt);

    /**
     * Create an encrypted PDF.
     *
     * @param pszFilename save the encrypted PDF here.
     */
    void CreateEncryptedPdf(const char* pszFilename);

private:
    char* m_pEncBuffer;
    PoDoFo::pdf_long m_lLen;
    int   m_protection;

};

#endif // ENCRYPT_TEST_H
