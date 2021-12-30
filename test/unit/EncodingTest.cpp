/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <PdfTest.h>
#include "TestExtension.h"

#include <ostream>

using namespace std;
using namespace mm;

static void outofRangeHelper(PdfEncoding& encoding);

inline ostream& operator<<(ostream& o, const PdfVariant& s)
{
    string str;
    s.ToString(str);
    return o << str;
}

TEST_CASE("testDifferences")
{
    PdfDifferenceList difference;

    // Newly created encoding should be empty
    REQUIRE(difference.GetCount() == 0);

    // Adding 0 should work
    difference.AddDifference(0, 0, "A");
    REQUIRE(difference.GetCount() == 1);

    // Adding 255 should work
    difference.AddDifference(255, 0, "B");
    REQUIRE(difference.GetCount() == 2);

    // Convert to array
    PdfArray data;
    PdfArray expected;
    expected.Add(static_cast<int64_t>(0));
    expected.Add(PdfName("A"));
    expected.Add(static_cast<int64_t>(255));
    expected.Add(PdfName("B"));

    difference.ToArray(data);

    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);


    // Test replace
    expected.Clear();
    expected.Add(static_cast<int64_t>(0));
    expected.Add(PdfName("A"));
    expected.Add(static_cast<int64_t>(255));
    expected.Add(PdfName("X"));

    difference.AddDifference(255, 0, "X");

    difference.ToArray(data);

    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);


    // Test more complicated array
    expected.Clear();
    expected.Add(static_cast<int64_t>(0));
    expected.Add(PdfName("A"));
    expected.Add(PdfName("B"));
    expected.Add(PdfName("C"));
    expected.Add(static_cast<int64_t>(4));
    expected.Add(PdfName("D"));
    expected.Add(PdfName("E"));
    expected.Add(static_cast<int64_t>(9));
    expected.Add(PdfName("F"));
    expected.Add(static_cast<int64_t>(255));
    expected.Add(PdfName("X"));

    difference.AddDifference(1, 0, "B");
    difference.AddDifference(2, 0, "C");
    difference.AddDifference(4, 0, "D");
    difference.AddDifference(5, 0, "E");
    difference.AddDifference(9, 0, "F");

    difference.ToArray(data);

    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);

    // Test if contains works correctly
    PdfName name;
    char32_t value;
    REQUIRE(difference.Contains(0, name, value));
    REQUIRE(name == "A");
    REQUIRE(static_cast<int>(value) == 0x0041);

    REQUIRE(difference.Contains(9, name, value));
    REQUIRE(name == "F");
    REQUIRE(static_cast<int>(value) == 0x0046);

    REQUIRE(difference.Contains(255, name, value));
    REQUIRE(name  == "X");
    REQUIRE(static_cast<int>(value) == 0x0058);

    REQUIRE(difference.Contains(100, name, value));
}

TEST_CASE("testDifferencesObject")
{
    PdfDifferenceList difference;
    difference.AddDifference(1, 0, "B");
    difference.AddDifference(2, 0, "C");
    difference.AddDifference(4, 0, "D");
    difference.AddDifference(5, 0, "E");
    difference.AddDifference(9, 0, "F");

    PdfDifferenceEncoding encoding(difference, PdfEncodingMapFactory::MacRomanEncodingInstance());


    // Check for encoding key
    PdfMemDocument doc;
    PdfName name;
    PdfObject* obj;
    REQUIRE(encoding.TryGetExportObject(doc.GetObjects(), name, obj));
    REQUIRE(obj != nullptr);
    REQUIRE(obj->GetDictionary().HasKey("Encoding"));

    PdfObject* pKey = obj->GetDictionary().GetKey("Encoding");
    REQUIRE(pKey->IsReference());

    PdfObject* pEncoding = doc.GetObjects().GetObject(pKey->GetReference());

    // Test BaseEncoding
    PdfObject* pBase = pEncoding->GetDictionary().GetKey("BaseEncoding");
    REQUIRE(pBase->GetName() == "MacRomanEncoding");

    // Test differences
    PdfObject* diff = pEncoding->GetDictionary().GetKey("Differences");
    PdfArray expected;

    expected.Add(static_cast<int64_t>(1));
    expected.Add(PdfName("B"));
    expected.Add(PdfName("C"));
    expected.Add(static_cast<int64_t>(4));
    expected.Add(PdfName("D"));
    expected.Add(PdfName("E"));
    expected.Add(static_cast<int64_t>(9));
    expected.Add(PdfName("F"));

    const PdfArray& data = diff->GetArray();
    REQUIRE(expected.GetSize() == data.GetSize());
    for (unsigned int i = 0; i < data.GetSize(); i++)
        REQUIRE(expected[i] == data[i]);
}

TEST_CASE("testDifferencesEncoding")
{
    PdfMemDocument doc;

    // Create a differences encoding where A and B are exchanged
    PdfDifferenceList difference;
    difference.AddDifference(0x0041, 0, "B");
    difference.AddDifference(0x0042, 0, "A");
    difference.AddDifference(0x0043, 0, "D");

    PdfEncoding encoding (std::make_shared<PdfDifferenceEncoding>(difference, PdfEncodingMapFactory::WinAnsiEncodingInstance()));

    auto encoded = PdfString(encoding.ConvertToEncoded("BAABC"));
    //REQUIRE(encoded == ""); // TODO
    auto unicode = encoding.ConvertToUtf8(encoded);
    REQUIRE(encoded == unicode);
}

TEST_CASE("testUnicodeNames")
{
    // List of items which are defined twice and cause
    // other ids to be returned than those which where send in
    const char* duplicates[] = {
        "Delta",
        "fraction",
        "hyphen",
        "macron",
        "mu",
        "Omega",
        "periodcentered",
        "scedilla",
        "Scedilla",
        "space",
        "tcommaaccent",
        "Tcommaaccent",
        "exclamsmall",
        "dollaroldstyle",
        "zerooldstyle",
        "oneoldstyle",
        "twooldstyle",
        "threeoldstyle",
        "fouroldstyle",
        "fiveoldstyle",
        "sixoldstyle",
        "sevenoldstyle",
        "eightoldstyle",
        "nineoldstyle",
        "ampersandsmall",
        "questionsmall",
        nullptr
    };

    int count = 0;
    for (int i = 0; i <= 0xFFFF; i++)
    {
        PdfName name = PdfDifferenceEncoding::UnicodeIDToName(static_cast<char32_t>(i));

        char32_t id = PdfDifferenceEncoding::NameToUnicodeID(name);

        bool found = false;
        const char** duplicate = duplicates;
        while (*duplicate != nullptr)
        {
            if (name == *duplicate)
            {
                found = true;
                break;
            }

            duplicate++;
        }

        if (!found)
        {
            // Does not work because of too many duplicates...
            //CPPUNIT_ASSERT_EQUAL_MESSAGE( name.GetName(), id, static_cast<pdf_utf16be>(i) );
            if (static_cast<char32_t>(i) == id)
                count++;
        }
    }

    INFO("Compared codes count");
    REQUIRE(count == 65422);
}

TEST_CASE("testGetCharCode")
{
    auto winAnsiEncoding = PdfEncodingFactory::CreateWinAnsiEncoding();
    INFO("WinAnsiEncoding");
    outofRangeHelper(winAnsiEncoding);

    auto macRomanEncoding = PdfEncodingFactory::CreateMacRomanEncoding();
    INFO("MacRomanEncoding");
    outofRangeHelper(macRomanEncoding);

    PdfDifferenceList difference;
    difference.AddDifference(0x0041, 0, "B");
    difference.AddDifference(0x0042, 0, "A");
    PdfEncoding differenceEncoding(std::make_shared<PdfDifferenceEncoding>(difference, PdfEncodingMapFactory::WinAnsiEncodingInstance()));
    outofRangeHelper(differenceEncoding);
}

TEST_CASE("testToUnicodeParse")
{
    const char* toUnicode =
        "3 beginbfrange\n"
        "<0001> <0004> <1001>\n"
        "<0005> <000A> [<000A> <0009> <0008> <0007> <0006> <0005>]\n"
        "<000B> <000F> <100B>\n"
        "endbfrange\n";
    charbuff encodedStr("\x0\x1\x0\x2\x0\x3\x0\x4\x0\x5\x0\x6\x0\x7\x0\x8\x0\x9\x0\xA\x0\xB\x0\xC\x0\xD\x0\xE\x0\xF\x0\x0"sv);
    string expected(""sv);

    PdfMemDocument doc;
    auto toUnicodeObj = doc.GetObjects().CreateDictionaryObject();
    toUnicodeObj->GetOrCreateStream().Set(toUnicode, strlen(toUnicode));

    PdfEncoding encoding(std::make_shared<PdfIdentityEncoding>(2), PdfCMapEncoding::Create(*toUnicodeObj));

    auto utf8str = encoding.ConvertToUtf8(PdfString(encodedStr));
    //REQUIRE(utf8str == expected); // TODO

    const char* toUnicodeInvalidTests[] =
    {
        // missing object numbers
        "beginbfrange\n",
        "beginbfchar\n",

        // invalid hex digits
        "2 beginbfrange <WXYZ> endbfrange\n",
        "2 beginbfrange <-123> endbfrange\n",
        "2 beginbfrange <<00>> endbfrange\n",

        // missing hex digits
        "2 beginbfrange <> endbfrange\n",

        // empty array
        "2 beginbfrange [] endbfrange\n",

        nullptr
    };

    for (size_t i = 0; toUnicodeInvalidTests[i] != nullptr; i++)
    {
        try
        {
            PdfIndirectObjectList invalidList;
            PdfObject* strmInvalidObject;

            strmInvalidObject = invalidList.CreateObject(PdfVariant(PdfDictionary()));
            strmInvalidObject->GetStream()->Set(toUnicodeInvalidTests[i], strlen(toUnicodeInvalidTests[i]));

            PdfEncoding encodingTestInvalid(std::make_shared<PdfIdentityEncoding>(2));

            auto unicodeStringTestInvalid = encodingTestInvalid.ConvertToUtf8(PdfString(encodedStr));

            // exception not thrown - should never get here
            // TODO not all invalid input throws an exception (e.g. no hex digits in <WXYZ>)
            //CPPUNIT_ASSERT( false );
        }
        catch (PdfError& error)
        {
            // parsing every invalid test string should throw an exception
        }
        catch (exception& ex)
        {
            FAIL("Unexpected exception type");
        }
    }
}

void outofRangeHelper(PdfEncoding& encoding)
{
    REQUIRE(encoding.GetCodePoint(encoding.GetFirstChar()) != U'\0');
    REQUIRE(encoding.GetCodePoint(encoding.GetFirstChar().Code - 1) == U'\0');
    REQUIRE(encoding.GetCodePoint(encoding.GetLastChar()) == U'\0');
    REQUIRE(encoding.GetCodePoint(encoding.GetLastChar().Code + 1) == U'\0');
}
