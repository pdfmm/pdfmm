/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

#include "TestUtils.h"

using namespace std;
using namespace mm;

static void testAuthenticate(PdfEncrypt& encrypt, unsigned keyLength, unsigned rValue);
static void testEncrypt(PdfEncrypt& encrypt);
static void createEncryptedPdf(const string_view& filename);

charbuff s_encBuffer;
PdfPermissions s_protection;

struct Init
{
    Init()
    {
        const char* buffer1 = "Somekind of drawing \001 buffer that possibly \003 could contain PDF drawing commands";
        const char* buffer2 = " possibly could contain PDF drawing\003  commands";

        size_t len = strlen(buffer1) + 2 * strlen(buffer2);
        s_encBuffer.resize(len);

        memcpy(s_encBuffer.data(), buffer1, strlen(buffer1) * sizeof(char));
        memcpy(s_encBuffer.data() + strlen(buffer1), buffer2, strlen(buffer2));
        memcpy(s_encBuffer.data() + strlen(buffer1) + strlen(buffer2), buffer2, strlen(buffer2));

        s_protection = PdfPermissions::Print |
            PdfPermissions::Edit |
            PdfPermissions::Copy |
            PdfPermissions::EditNotes |
            PdfPermissions::FillAndSign |
            PdfPermissions::Accessible |
            PdfPermissions::DocAssembly |
            PdfPermissions::HighPrint;
    }
} s_init;

TEST_CASE("testDefault")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo");
    testAuthenticate(*encrypt, 40, 2);
    testEncrypt(*encrypt);
}

#ifndef PODOFO_HAVE_OPENSSL_NO_RC4

TEST_CASE("testRC4")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::RC4V1,
        PdfKeyLength::L40);

    testAuthenticate(*encrypt, 40, 3);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_40")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L40);

    testAuthenticate(*encrypt, 40, 3);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_56")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L56);

    testAuthenticate(*encrypt, 56, 3);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_80")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L80);

    testAuthenticate(*encrypt, 80, 3);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_96")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L96);

    testAuthenticate(*encrypt, 96, 3);
    testEncrypt(*encrypt);
}

TEST_CASE("testRC4v2_128")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::RC4V2,
        PdfKeyLength::L128);

    testAuthenticate(*encrypt, 128, 3);
    testEncrypt(*encrypt);
}
#endif // PODOFO_HAVE_OPENSSL_NO_RC4

TEST_CASE("testAESV2")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::AESV2,
        PdfKeyLength::L128);

    testAuthenticate(*encrypt, 128, 4);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

#ifdef PODOFO_HAVE_LIBIDN

TEST_CASE("testAESV3")
{
    auto encrypt = PdfEncrypt::CreatePdfEncrypt("user", "podofo", s_protection,
        PdfEncryptAlgorithm::AESV3,
        PdfKeyLength::L256);

    TestAuthenticate(encrypt, 256, 5);
    // AES decryption is not yet implemented.
    // Therefore we have to disable this test.
    //TestEncrypt(encrypt);
}

#endif // PODOFO_HAVE_LIBIDN

void testAuthenticate(PdfEncrypt& encrypt, unsigned keyLength, unsigned rValue)
{
    (void)keyLength;
    (void)rValue;
    PdfString documentId = PdfString::FromHexData("BF37541A9083A51619AD5924ECF156DF");

    encrypt.GenerateEncryptionKey(documentId);

    INFO("authenticate using user password");
    REQUIRE(encrypt.Authenticate(std::string("user"), documentId));
    INFO("authenticate using wrong user password");
    REQUIRE(encrypt.Authenticate(std::string("wrongpassword"), documentId));
    INFO("authenticate using owner password");
    REQUIRE(encrypt.Authenticate(std::string("podofo"), documentId));
    INFO("authenticate using wrong owner password");
    REQUIRE(encrypt.Authenticate(std::string("wrongpassword"), documentId));
}

void testEncrypt(PdfEncrypt& encrypt)
{
    encrypt.SetCurrentReference(PdfReference(7, 0));

    string encrypted;
    // Encrypt buffer
    try
    {
        encrypt.Encrypt(s_encBuffer, encrypted);
    }
    catch (PdfError& e)
    {
        FAIL(e.ErrorMessage(e.GetError()));
    }

    string decrypted;
    // Decrypt buffer
    try
    {
        encrypt.Decrypt(encrypted, decrypted);
    }
    catch (PdfError& e)
    {
        FAIL(e.ErrorMessage(e.GetError()));
    }

    INFO("compare encrypted and decrypted buffers");
    REQUIRE(memcmp(s_encBuffer.data(), decrypted.data(), s_encBuffer.size()) == 0);
}

TEST_CASE("testLoadEncrypedFilePdfParser")
{
    string tempFile = TestUtils::getTempFilename();

    try
    {
        createEncryptedPdf(tempFile);

        auto device = std::make_shared<PdfFileInputDevice>(tempFile);
        // Try loading with PdfParser
        PdfIndirectObjectList objects;
        PdfParser parser(objects);

        try
        {
            parser.Parse(*device, true);

            // Must throw an exception
            FAIL("Encrypted file not recognized!");
        }
        catch (PdfError& e)
        {
            if (e.GetError() != PdfErrorCode::InvalidPassword)
                FAIL("Invalid encryption exception thrown!");
        }

        parser.SetPassword("user");
    }
    catch (PdfError& e)
    {
        e.PrintErrorMsg();

        INFO(cmn::Format("Removing temp file: {}", tempFile));
        TestUtils::deleteFile(tempFile);
        throw e;
    }

    INFO(cmn::Format("Removing temp file: {}", tempFile));
    TestUtils::deleteFile(tempFile);
}

TEST_CASE("testLoadEncrypedFilePdfMemDocument")
{
    string tempFile = TestUtils::getTempFilename();

    try
    {
        createEncryptedPdf(tempFile);

        // Try loading with PdfParser
        PdfMemDocument document;
        try
        {
            document.Load(tempFile);

            // Must throw an exception
            FAIL("Encrypted file not recognized!");
        }
        catch (...)
        {

        }

        document.Load(tempFile, "user");
    }
    catch (PdfError& e)
    {
        e.PrintErrorMsg();

        INFO(cmn::Format("Removing temp file: {}", tempFile));
        TestUtils::deleteFile(tempFile);

        throw e;
    }

    INFO(cmn::Format("Removing temp file: {}", tempFile));
    TestUtils::deleteFile(tempFile);
}

void createEncryptedPdf(const string_view& filename)
{
    PdfMemDocument doc;
    PdfPage* page = doc.GetPageTree().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
    PdfPainter painter;
    painter.SetCanvas(page);

    PdfFont* font = doc.GetFontManager().GetFont("Arial");
    if (font == nullptr)
        FAIL("Coult not find Arial font");

    painter.SetFont(font);
    painter.GetTextState().SetFontSize(16);
    painter.DrawText("Hello World", 100, 100);
    painter.FinishDrawing();

    doc.SetEncrypted("user", "owner");
    doc.Save(filename);

    INFO(cmn::Format("Wrote: {} (R={})", filename, doc.GetEncrypt()->GetRevision()));
}

TEST_CASE("testEnableAlgorithms")
{
    auto enabledAlgorithms = PdfEncrypt::GetEnabledEncryptionAlgorithms();

    // By default every algorithms should be enabled
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V1));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V2));
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV2));
#ifdef PODOFO_HAVE_LIBIDN
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV3));
#endif // PODOFO_HAVE_LIBIDN

    PdfEncryptAlgorithm testAlgorithms = PdfEncryptAlgorithm::AESV2;
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
    testAlgorithms |= PdfEncryptAlgorithm::RC4V1 | PdfEncryptAlgorithm::RC4V2;
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
#ifdef PODOFO_HAVE_LIBIDN
    testAlgorithms |= PdfEncryptAlgorithm::AESV3;
#endif // PODOFO_HAVE_LIBIDN
    REQUIRE(testAlgorithms == PdfEncrypt::GetEnabledEncryptionAlgorithms());

    // Disable AES
#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
    PdfEncrypt::SetEnabledEncryptionAlgorithms(PdfEncryptAlgorithm::RC4V1 |
        PdfEncryptAlgorithm::RC4V2);

    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V1));
    REQUIRE(PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::RC4V2));
#endif // PODOFO_HAVE_OPENSSL_NO_RC4
    REQUIRE(!PdfEncrypt::IsEncryptionEnabled(PdfEncryptAlgorithm::AESV2));

#ifndef PODOFO_HAVE_OPENSSL_NO_RC4
    REQUIRE((PdfEncryptAlgorithm::RC4V1 | PdfEncryptAlgorithm::RC4V2) ==
        PdfEncrypt::GetEnabledEncryptionAlgorithms());
#endif // PODOFO_HAVE_OPENSSL_NO_RC4

    PdfObject object;
    object.GetDictionary().AddKey("Filter", PdfName("Standard"));
    object.GetDictionary().AddKey("V", static_cast<int64_t>(4L));
    object.GetDictionary().AddKey("R", static_cast<int64_t>(4L));
    object.GetDictionary().AddKey("P", static_cast<int64_t>(1L));
    object.GetDictionary().AddKey("O", PdfString(""));
    object.GetDictionary().AddKey("U", PdfString(""));

    try
    {
        (void)PdfEncrypt::CreatePdfEncrypt(object);
        REQUIRE(false);
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::UnsupportedFilter);
    }

    // Restore default
    PdfEncrypt::SetEnabledEncryptionAlgorithms(enabledAlgorithms);
}
