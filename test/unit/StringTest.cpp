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

static void TestWriteEscapeSequences(const string_view& str, const string_view& expected);

TEST_CASE("testStringUtf8")
{
    string_view str = "Hallo PoDoFo!";
    REQUIRE(PdfString(str) == str);

    string_view stringJapUtf8 = "「PoDoFo」は今から日本語も話せます。";
    REQUIRE(PdfString(stringJapUtf8) == stringJapUtf8);
}

TEST_CASE("testGetStringUtf8")
{
    const string src1 = "Hello World!";
    const string src2 = src1;
    const string src3 = "「Po\tDoFo」は今から日本語も話せます。";

    // Normal ascii string should be converted to UTF8
    PdfString str1(src1.c_str());
    string res1 = str1.GetString();

    INFO("testing const char* ASCII -> UTF8");
    REQUIRE(src1 == res1);

    // Normal string string should be converted to UTF8
    PdfString str2(src2);
    string res2 = str2.GetString();

    INFO("testing string ASCII -> UTF8");
    REQUIRE(src2 == res2);

    // UTF8 data in string cannot be converted as we do not know it is UTF8
    PdfString str3(src3);
    string res3 = str3.GetString();

    INFO("testing string UTF8 -> UTF8");
    REQUIRE(res3 != src3);
}

TEST_CASE("testEscapeBrackets")
{
    // Test balanced brackets ansi
    const char* ascii = "Hello (balanced) World";
    const char* asciiExpect = "(Hello \\(balanced\\) World)";

    PdfString pdfStrAscii(ascii);
    PdfVariant varAscii(pdfStrAscii);
    string strAscii;
    varAscii.ToString(strAscii);

    REQUIRE(!pdfStrAscii.IsUnicode());
    REQUIRE(strAscii == asciiExpect);

    // Test un-balanced brackets ansi
    const char* ascii2 = "Hello ((unbalanced World";
    const char* asciiExpect2 = "(Hello \\(\\(unbalanced World)";

    PdfString pdfStrAscii2(ascii2);
    PdfVariant varAscii2(pdfStrAscii2);
    string strAscii2;
    varAscii2.ToString(strAscii2);

    REQUIRE(strAscii2 == asciiExpect2);

    // Test balanced brackets unicode
    const char* unicode = "Hello (balanced) World";

    // Force unicode string
    PdfString pdfStrUnic(unicode);
    PdfVariant varUnic(pdfStrUnic);
    string strUnic;
    varUnic.ToString(strUnic);

    REQUIRE(strUnic == unicode);

    string_view utf16Expect =
        "<28FEFF00480065006C006C006F0020005C28005C280075006E00620061006C0061006E00630065006400200057006F0072006C006429>";

    // Test reading the unicode string back in
    PdfVariant varRead;
    PdfTokenizer tokenizer;
    PdfMemoryInputDevice input(utf16Expect);
    (void)tokenizer.ReadNextVariant(input, varRead);
    REQUIRE(varRead.GetDataType() == PdfDataType::String);
    REQUIRE(varRead.GetString() == unicode);
    REQUIRE(varRead.GetString() == pdfStrUnic);
}

TEST_CASE("testWriteEscapeSequences")
{
    TestWriteEscapeSequences("(1Hello\\nWorld)", "(1Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\nWorld)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\012World)", "(Hello\\nWorld)");
    TestWriteEscapeSequences("(Hello\\012World)", "(Hello\\nWorld)");

    TestWriteEscapeSequences("(2Hello\\rWorld)", "(2Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\rWorld)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\015World)", "(Hello\\rWorld)");
    TestWriteEscapeSequences("(Hello\\015World)", "(Hello\\rWorld)");

    TestWriteEscapeSequences("(3Hello\\tWorld)", "(3Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\tWorld)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\011World)", "(Hello\\tWorld)");
    TestWriteEscapeSequences("(Hello\\011World)", "(Hello\\tWorld)");

    TestWriteEscapeSequences("(4Hello\\fWorld)", "(4Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\fWorld)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\014World)", "(Hello\\fWorld)");
    TestWriteEscapeSequences("(Hello\\014World)", "(Hello\\fWorld)");

    TestWriteEscapeSequences("(5Hello\\(World)", "(5Hello\\(World)");
    TestWriteEscapeSequences("(Hello\\050World)", "(Hello\\(World)");

    TestWriteEscapeSequences("(6Hello\\)World)", "(6Hello\\)World)");
    TestWriteEscapeSequences("(Hello\\051World)", "(Hello\\)World)");

    TestWriteEscapeSequences("(7Hello\\\\World)", "(7Hello\\\\World)");
    TestWriteEscapeSequences("(Hello\\\134World)", "(Hello\\\\World)");

    // Special case, \ at end of line
    TestWriteEscapeSequences("(8Hello\\\nWorld)", "(8HelloWorld)");


    TestWriteEscapeSequences("(9Hello\003World)", "(9Hello\003World)");
}

TEST_CASE("testEmptyString")
{
    const char* empty = "";
    string strEmpty;
    string strEmpty2(empty);

    PdfString str1;
    PdfString str2(strEmpty);
    PdfString str3(strEmpty2);
    PdfString str4(empty);

    REQUIRE(str1.GetString().length() == 0u);
    REQUIRE(str1.GetString() == strEmpty);
    REQUIRE(str1.GetString() == strEmpty2);

    REQUIRE(str2.GetString().length() == 0u);
    REQUIRE(str2.GetString() == strEmpty);
    REQUIRE(str2.GetString() == strEmpty2);

    REQUIRE(str3.GetString().length() == 0u);
    REQUIRE(str3.GetString() == strEmpty);
    REQUIRE(str3.GetString() == strEmpty2);

    REQUIRE(str4.GetString().length() == 0u);
    REQUIRE(str4.GetString() == strEmpty);
    REQUIRE(str4.GetString() == strEmpty2);
}

TEST_CASE("testInitFromUtf8")
{
    const char* utf8 = "This string contains UTF-8 Characters: ÄÖÜ.";
    const PdfString str(utf8);

    REQUIRE(str.IsUnicode());
    REQUIRE(str.GetString().length() == char_traits<char>::length(utf8));
    REQUIRE(str.GetString() == string(utf8));
}

void TestWriteEscapeSequences(const string_view& str, const string_view& expected)
{
    PdfVariant variant;
    string ret;

    INFO(cmn::Format("Testing with value: {}", str));
    PdfPostScriptTokenizer tokenizer;
    PdfMemoryInputDevice device(str);

    tokenizer.TryReadNextVariant(device, variant);
    REQUIRE(variant.GetDataType() == PdfDataType::String);

    variant.ToString(ret);
    INFO(cmn::Format("   -> Convert To String: {}", ret));

    REQUIRE(expected == ret);
}
