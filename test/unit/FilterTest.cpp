/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>

using namespace std;
using namespace mm;

static void TestFilter(PdfFilterType eFilter, const char* testBuffer, size_t testLength);

static string_view s_testBuffer1 = "Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.";

const char s_testBuffer2[] = {
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x01,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x03,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x02,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x01, 0x64, 0x65, static_cast<char>(0xFE), 0x6B, static_cast<char>(0x80), 0x45, 0x32, static_cast<char>(0x88), 0x12, static_cast<char>(0x71), static_cast<char>(0xEA), 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

TEST_CASE("testFilters")
{
    for (unsigned i = 0; i <= (unsigned)PdfFilterType::Crypt; i++)
    {
        TestFilter(static_cast<PdfFilterType>(i), s_testBuffer1.data(), s_testBuffer1.length());
        TestFilter(static_cast<PdfFilterType>(i), s_testBuffer2, std::size(s_testBuffer2));
    }
}

TEST_CASE("testCCITT")
{
    unique_ptr<PdfFilter> filter = PdfFilterFactory::Create(PdfFilterType::CCITTFaxDecode);
    if (filter == nullptr)
        INFO("!!! ePdfFilter_CCITTFaxDecode not implemented skipping test!");
}

void TestFilter(PdfFilterType filterType, const char* testBuffer, size_t testLength)
{
    size_t encodedLength;
    size_t decodedLength;
    unique_ptr<char[]> encoded;
    unique_ptr<char[]> decoded;

    unique_ptr<PdfFilter> filter = PdfFilterFactory::Create(filterType);
    if (filter == nullptr)
    {
        INFO(cmn::Format("!!! Filter {} not implemented.\n", filterType));
        return;
    }

    INFO(cmn::Format("Testing Algorithm {}:", filterType));
    INFO("\t-> Testing Encoding");
    try
    {
        filter->Encode(testBuffer, testLength, encoded, encodedLength);
    }
    catch (PdfError& e)
    {
        if (e == PdfErrorCode::UnsupportedFilter)
        {
            INFO(cmn::Format("\t-> Encoding not supported for filter {}.", (unsigned)filterType));
            return;
        }
        else
        {
            e.AddToCallstack(__FILE__, __LINE__);
            throw e;
        }
    }

    INFO("\t-> Testing Decoding");
    try
    {
        filter->Decode(encoded.get(), encodedLength, decoded, decodedLength);
    }
    catch (PdfError& e)
    {
        if (e == PdfErrorCode::UnsupportedFilter)
        {
            INFO(cmn::Format("\t-> Decoding not supported for filter {}", filterType));
            return;
        }
        else
        {
            e.AddToCallstack(__FILE__, __LINE__);
            throw e;
        }
    }

    INFO(cmn::Format("\t-> Original Data Length: {}", testLength));
    INFO(cmn::Format("\t-> Encoded  Data Length: {}", encodedLength));
    INFO(cmn::Format("\t-> Decoded  Data Length: {}", decodedLength));

    REQUIRE(testLength == decodedLength);
    REQUIRE(memcmp(testBuffer, decoded.get(), testLength) == 0);

    INFO("\t-> Test succeeded!");
}


