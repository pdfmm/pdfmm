/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

/*
    Notes:

    1) out of memory tests don't run if Address Santizer (ASAN) is enabled because
       ASAN terminates the unit test process the first time it attempts to allocate
       too much memory (so running the tests with and without ASAN is recommended)

    2) PoDoFo log warnings about inconsistencies or values out of range are expected
       because the tests are supplying invalid values to check PoDoFo behaves correctly
       in those situations
*/

#include <limits>

#include <PdfTest.h>

using namespace std;
using namespace mm;

static string generateXRefEntries(size_t count);
static bool canOutOfMemoryKillUnitTests();
static void testReadXRefSubsection();

// this value is from Table C.1 in Appendix C.2 Architectural Limits in PDF 32000-1:2008
// on 32-bit systems sizeof(PdfParser::TXRefEntry)=16 => max size of m_offsets=16*8,388,607 = 134 MB
// on 64-bit systems sizeof(PdfParser::TXRefEntry)=24 => max size of m_offsets=16*8,388,607 = 201 MB
constexpr unsigned maxNumberOfIndirectObjects = 8388607;

namespace mm
{
    class PdfParserTestWrapper : public PdfParser
    {
    public:
        PdfParserTestWrapper(PdfIndirectObjectList& objectList, const string_view& buff)
            : PdfParser(objectList), m_device(new PdfMemoryInputDevice(buff))
        { }

        void ReadXRefContents(size_t offset, bool positionAtEnd)
        {
            // call protected method
            PdfParser::ReadXRefContents(*m_device, offset, positionAtEnd);
        }

        void ReadXRefSubsection(int64_t firstObject, int64_t objectCount)
        {
            // call protected method
            PdfParser::ReadXRefSubsection(*m_device, firstObject, objectCount);
        }

        void ReadXRefStreamContents(size_t offset, bool readOnlyTrailer)
        {
            // call protected method
            PdfParser::ReadXRefStreamContents(*m_device, offset, readOnlyTrailer);
        }

        void ReadObjects()
        {
            // call protected method
            PdfParser::ReadObjects(*m_device);
        }

        bool IsPdfFile()
        {
            // call protected method
            return PdfParser::IsPdfFile(*m_device);
        }

        const shared_ptr<PdfInputDevice>& GetDevice() { return m_device; }

    private:
        shared_ptr<PdfInputDevice> m_device;
    };
}

TEST_CASE("TestMaxObjectCount")
{
    size_t defaultObjectCount = PdfParser::GetMaxObjectCount();

    REQUIRE(defaultObjectCount == maxNumberOfIndirectObjects);

    // test methods that use PdfParser::s_nMaxObjects or GetMaxObjectCount
    // with a range of different maximums
    PdfParser::SetMaxObjectCount(numeric_limits<unsigned>::max());
    testReadXRefSubsection();

    PdfParser::SetMaxObjectCount(maxNumberOfIndirectObjects);
    testReadXRefSubsection();

    PdfParser::SetMaxObjectCount(numeric_limits<unsigned short>::max());
    testReadXRefSubsection();

    PdfParser::SetMaxObjectCount(numeric_limits<unsigned>::max());
    testReadXRefSubsection();
}

TEST_CASE("TestReadXRefContents")
{
    try
    {
        // generate an xref section
        // xref
        // 0 3
        // 0000000000 65535 f 
        // 0000000018 00000 n 
        // 0000000077 00000 n
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF
        ostringstream oss;
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(3);
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefContents(0, false);
        // expected to succeed
    }
    catch (PdfError&)
    {
        INFO("should not throw PdfError");
        REQUIRE(false);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    try
    {
        // generate an xref section with missing xref entries
        // xref
        // 0 3
        // 0000000000 65535 f 
        // 0000000018 00000 n 
        // 
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF        
        ostringstream oss;
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(2); // 2 entries supplied, but expecting 3 entries
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefContents(0, false);
        // expected to succeed
    }
    catch (PdfError&)
    {
        FAIL("should not throw PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    try
    {
        // TODO malformed entries are not detected
        // generate an xref section with badly formed xref entries
        // xref
        // 0 3        
        // 000000000 65535
        // 00000000065535 x
        // 0000000
        // 0000000018 00000 n
        // 0000000077 00000 n        
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF
        ostringstream oss;
        oss << "xref\r\n0 5\r\n";
        oss << "000000000 65535\r\n";
        oss << "00000000065535 x\r\n";
        oss << "0000000\r\n";
        oss << generateXRefEntries(2);
        oss << "trailer << /Root 1 0 R /Size 5 >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefContents(0, false);
        // succeeds reading badly formed xref entries  - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // CVE-2017-8053 ReadXRefContents and ReadXRefStreamContents are mutually recursive   
    // and can cause stack overflow

    try
    {
        // generate an xref section and one XRef stream that references itself
        // via the /Prev entry (but use a slightly lower offset by linking to
        // to whitespace discarded by the tokenizer just before the xref section)
        // xref
        // 0 1
        // 000000000 65535
        // 2 0 obj << /Type XRef /Prev offsetXrefStmObj2 >> stream ... endstream
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // offsetXrefStmObj2
        // %%EOF
        ostringstream oss;

        // object stream contents - length excludes trailing whitespace
        string streamContents =
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");

        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);

        // XRef stream at offsetXrefStm1, but any /Prev entries pointing to any offet between
        // offsetXrefStm1Whitespace and offsetXrefStm1 point to the same /Prev section
        // because the PDF processing model says tokenizer must discard whitespace and comments
        size_t offsetXrefStm1Whitespace = oss.str().length();
        oss << "    \r\n";
        oss << "% comments and leading white space are ignored - see PdfTokenizer::GetNextToken\r\n";
        size_t offsetXrefStm1 = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << streamContentsLength << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 3 ";
        oss << "/Prev " << offsetXrefStm1Whitespace << " ";     // xref /Prev offset points back to start of this stream object
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        oss << streamContents;
        oss << "endstream\r\n";
        oss << "endobj\r\n";

        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXrefStm1 << "\r\n";
        oss << "%EOF";

        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefContents(offsetXrefStm1, false);
        // succeeds in current code - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    try
    {
        // generate an xref section and two XRef streams that reference each other
        // via the /Prev entry
        // xref
        // 0 1
        // 000000000 65535
        // 2 0 obj << /Type XRef /Prev offsetXrefStmObj3 >> stream ...  endstream
        // 3 0 obj << /Type XRef /Prev offsetXrefStmObj2 >> stream ...  endstream
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // offsetXrefStmObj2
        // %%EOF
        ostringstream oss;

        // object stream contents - length excludes trailing whitespace
        string streamContents =
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");

        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);

        // xrefstm at offsetXrefStm1
        size_t offsetXrefStm1 = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << streamContentsLength << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 3 ";
        oss << "/Prev 185 ";     // xref stream 1 sets xref stream 2 as previous in chain
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        oss << streamContents;
        oss << "endstream\r\n";
        oss << "endobj\r\n";

        // xrefstm at offsetXrefStm2
        size_t offsetXrefStm2 = oss.str().length();
        REQUIRE(offsetXrefStm2 == 185); // hard-coded in /Prev entry in XrefStm1 above
        oss << "3 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << streamContentsLength << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 3 ";
        oss << "/Prev " << offsetXrefStm1 << " ";     // xref stream 2 sets xref stream 1 as previous in chain
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        oss << streamContents;
        oss << "endstream\r\n";
        oss << "endobj\r\n";

        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXrefStm2 << "\r\n";
        oss << "%EOF";

        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefContents(offsetXrefStm2, false);
        // succeeds in current code - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    try
    {
        // generate an xref section and lots of XRef streams without loops but reference 
        // the previous stream via the /Prev entry
        // xref
        // 0 1
        // 000000000 65535
        // 2 0 obj << /Type XRef >> stream ...  endstream
        // 3 0 obj << /Type XRef /Prev offsetStreamObj(2) >> stream ...  endstream
        // 4 0 obj << /Type XRef /Prev offsetStreamObj(3) >> stream ...  endstream
        // ...
        // N 0 obj << /Type XRef /Prev offsetStreamObj(N-1) >> stream ...  endstream
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // offsetStreamObj(N)
        // %%EOF
        ostringstream oss;
        size_t prevOffset = 0;
        size_t currentOffset = 0;

        // object stream contents - length excludes trailing whitespace
        string streamContents =
            "01 0E8A 0\r\n"
            "02 0002 00\r\n";
        size_t streamContentsLength = streamContents.size() - strlen("\r\n");

        // xref section at offset 0
        //size_t offsetXref = 0;
        oss << "xref\r\n0 1\r\n";
        oss << generateXRefEntries(1);

        // this caused stack overflow on macOS 64-bit with around 3000 streams
        // and on Windows 32-bit with around 1000 streams

        const int maxXrefStreams = 10000;
        for (int i = 0; i < maxXrefStreams; i++)
        {
            int objNo = i + 2;

            // xrefstm at currentOffset linked back to stream at prevOffset
            prevOffset = currentOffset;
            currentOffset = oss.str().length();
            oss << objNo << " 0 obj ";
            oss << "<< /Type /XRef ";
            oss << "/Length " << streamContentsLength << " ";
            oss << "/Index [2 2] ";
            oss << "/Size 3 ";
            if (prevOffset > 0)
                oss << "/Prev " << prevOffset << " ";
            oss << "/W [1 2 1] ";
            oss << "/Filter /ASCIIHexDecode ";
            oss << ">>\r\n";
            oss << "stream\r\n";
            oss << streamContents;
            oss << "endstream\r\n";
            oss << "endobj\r\n";
        }

        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << currentOffset << "\r\n";
        oss << "%EOF";

        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefContents(currentOffset, false);
        // succeeds in current code - should it?
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }
}

void testReadXRefSubsection()
{
    int64_t firstObject = 0;
    int64_t objectCount = 0;

    // TODO does ReadXRefSubsection with objectCount = 0 make sense ???

    // CVE-2017-5855 m_offsets.resize() NULL ptr read
    // CVE-2017-6844 m_offsets.resize() buffer overwrite 
    // false positives due to AFL setting allocator_may_return_null=1 which causes
    // ASAN to return NULL instead of throwing bad_alloc for out-of-memory conditions
    // https://github.com/mirrorer/afl/blob/master/docs/env_variables.txt#L248
    // https://github.com/google/sanitizers/issues/295#issuecomment-234273218 
    // the test for CVE-2018-5296 below checks that PoDoFo restricts allocations

    // CVE-2018-5296 m_offsets.resize() malloc failure when large size specified
    // check PoDoFo throws PdfError and not anything derived from exception
    // check PoDoFo can't allocate unrestricted amounts of memory

    if (PdfParser::GetMaxObjectCount() <= maxNumberOfIndirectObjects)
    {
        try
        {
            string strInput = generateXRefEntries(PdfParser::GetMaxObjectCount());
            PdfIndirectObjectList objects;
            PdfParserTestWrapper parser(objects, strInput);
            firstObject = 0;
            objectCount = PdfParser::GetMaxObjectCount();
            parser.ReadXRefSubsection(firstObject, objectCount);
            // expected to succeed
        }
        catch (PdfError&)
        {
            FAIL("should not throw PdfError");
        }
        catch (exception&)
        {
            FAIL("Unexpected exception type");
        }
    }
    else
    {
        // test has been called from testMaxObjectCount with PdfParser::SetMaxObjectCount()
        // set to a large value (large allocs are tested in address space tests below)
    }

    // don't run the following test if PdfParser::GetMaxObjectCount()+1 will overflow
    // in the numXRefEntries calculation below (otherwise we get an ASAN error)
    if (PdfParser::GetMaxObjectCount() < numeric_limits<unsigned>::max())
    {
        // don't generate xrefs for high values of GetMaxObjectCount() e.g. don't try to generate 2**63 xrefs
        unsigned numXRefEntries = std::min(maxNumberOfIndirectObjects + 1, PdfParser::GetMaxObjectCount() + 1);

        try
        {
            string strInput = generateXRefEntries(numXRefEntries);
            PdfIndirectObjectList objects;
            PdfParserTestWrapper parser(objects, strInput);
            firstObject = 0;
            objectCount = PdfParser::GetMaxObjectCount() + 1;
            parser.ReadXRefSubsection(firstObject, objectCount);
            FAIL("PdfError not thrown");
        }
        catch (PdfError& error)
        {
            // too many indirect objects in Trailer /Size key throws PdfErrorCode::ValueOutOfRange
            // but too many indirect objects in xref table throws PdfErrorCode::InvalidXRef
            REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
        }
        catch (exception&)
        {
            FAIL("Wrong exception type");
        }
    }

    // CVE-2018-5296 try to allocate more than address space size 
    // should throw a bad_length exception in STL which is rethrown as a PdfError
    try
    {
        // this attempts to allocate numeric_limits<size_t>::max()/2 * sizeof(TXRefEntry)
        // on 32-bit systems this allocates 2**31 * sizeof(TXRefEntry) = 2**31 * 16 (larger than 32-bit address space)
        // on LP64 (macOS,*nix) systems this allocates 2**63 * sizeof(TXRefEntry) = 2**63 * 24 (larger than 64-bit address space)
        // on LLP64 (Win64) systems this allocates 2**31 * sizeof(TXRefEntry) = 2**31 * 16 (smaller than 64-bit address space)
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<size_t>::max() / 2 - 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        // if objectCount > PdfParser::GetMaxObjectCount() then we'll see PdfErrorCode::InvalidXRef
        // otherwise we'll see PdfErrorCode::ValueOutOfRange or PdfErrorCode::OutOfMemory (see testMaxObjectCount)
        REQUIRE((error.GetError() == PdfErrorCode::InvalidXRef
            || error.GetError() == PdfErrorCode::ValueOutOfRange
            || error.GetError() == PdfErrorCode::OutOfMemory));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2018-5296 try to allocate 95% of VM address space size (which should always fail)
    if (!canOutOfMemoryKillUnitTests())
    {
        size_t maxObjects = numeric_limits<size_t>::max() / sizeof(PdfXRefEntry) / 100 * 95;

        try
        {
            string strInput = " ";
            PdfIndirectObjectList objects;
            PdfParserTestWrapper parser(objects, strInput);
            firstObject = 1;
            objectCount = maxObjects;
            parser.ReadXRefSubsection(firstObject, objectCount);
            FAIL("PdfError not thrown");
        }
        catch (PdfError& error)
        {
            if (maxObjects >= (size_t)PdfParser::GetMaxObjectCount())
                REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
            else
                REQUIRE(error.GetError() == PdfErrorCode::OutOfMemory);
        }
        catch (exception&)
        {
            FAIL("Wrong exception type");
        }
    }

    // CVE-2015-8981 happens because this->GetNextNumber() can return negative numbers 
    // in range (LONG_MIN to LONG_MAX) so the xref section below causes a buffer underflow
    // because m_offsets[-5].bParsed is set to true when first entry is read
    // NOTE: vector operator[] is not bounds checked

    // xref
    // -5 5
    // 0000000000 65535 f 
    // 0000000018 00000 n 
    // 0000000077 00000 n 
    // 0000000178 00000 n 
    // 0000000457 00000 n 
    // trailer
    // <<  /Root 1 0 R
    //    /Size 5
    //>>
    // startxref
    // 565
    // %%EOF

    try
    {
        string strInput = "0000000000 65535 f\r\n";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = -5LL;
        objectCount = 5;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE((error.GetError() == PdfErrorCode::ValueOutOfRange || error.GetError() == PdfErrorCode::NoXRef));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2015-8981 can also happen due to integer overflow in firstObject+objectCount
    // in the example below 2147483647=0x7FFF, so 0x7FFF + 0x7FFF = 0XFFFE = -2 on a 32-bit system
    // which means m_offsets.size()=5 because m_offsets.resize() is never called and 
    // m_offsets[2147483647].bParsed is set to true when first entry is read
    // NOTE: vector operator[] is not bounds checked

    // 2147483647 2147483647 
    // 0000000000 65535 f 
    // 0000000018 00000 n 
    // 0000000077 00000 n 
    // 0000000178 00000 n 
    // 0000000457 00000 n 
    // trailer
    // <<  /Root 1 0 R
    //    /Size 5
    //>>
    // startxref
    // 565
    // %%EOF

    try
    {
        string strInput = "0000000000 65535 f\r\n";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<unsigned>::max();
        objectCount = numeric_limits<unsigned>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "0000000000 65535 f\r\n";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<int64_t>::max();
        objectCount = numeric_limits<int64_t>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // test for integer overflows in ReadXRefSubsection (CVE-2017-5853) which caused
    // wrong buffer size to be calculated and then triggered buffer overflow (CVE-2017-6844)   
    // the overflow checks in ReadXRefSubsection depend on the value returned by GetMaxObjectCount
    // if the value changes these checks need looked at again
    REQUIRE(PdfParser::GetMaxObjectCount() <= numeric_limits<unsigned>::max());

    // test CVE-2017-5853 signed integer overflow in firstObject + objectCount
    // CVE-2017-5853 1.1 - firstObject < 0
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = -1LL;
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.2 - firstObject = min value of long
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<unsigned>::min();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.3 - firstObject = min value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<int64_t>::min();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.4 - firstObject = min value of size_t is zero (size_t is unsigned)
    // and zero is a valid value for firstObject

    // CVE-2017-5853 1.5 - firstObject = max value of long
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<unsigned>::max();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.6 - firstObject = max value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<int64_t>::max();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.7 - firstObject = max value of size_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = numeric_limits<size_t>::max();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        // weird: different errors returned depending on architecture 
        REQUIRE((error.GetError() == PdfErrorCode::ValueOutOfRange || sizeof(size_t) == 4));
        REQUIRE((error.GetError() == PdfErrorCode::InvalidXRef || sizeof(size_t) == 8));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 1.8 - firstObject = PdfParser::GetMaxObjectCount()
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(PdfParser::GetMaxObjectCount() > 0);
        firstObject = PdfParser::GetMaxObjectCount();
        objectCount = 1;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.1 - objectCount < 0
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = -1LL;
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.2 - objectCount = min value of long
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<unsigned>::min();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.3 - objectCount = min value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<int64_t>::min();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::ValueOutOfRange);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.4 - objectCount = min value of size_t is zero (size_t is unsigned)
    // and zero is a valid value for firstObject
    // TODO

    // CVE-2017-5853 2.5 - objectCount = max value of long
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<unsigned>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.6 - objectCount = max value of int64_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<int64_t>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.7 - objectCount = max value of size_t
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = numeric_limits<size_t>::max();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        // weird: different errors returned depending on architecture 
        REQUIRE((error.GetError() == PdfErrorCode::ValueOutOfRange || sizeof(size_t) == 4));
        REQUIRE((error.GetError() == PdfErrorCode::InvalidXRef || sizeof(size_t) == 8));
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.8 - objectCount = PdfParser::GetMaxObjectCount()
    try
    {
        string strInput = " ";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        firstObject = 1;
        objectCount = PdfParser::GetMaxObjectCount();
        parser.ReadXRefSubsection(firstObject, objectCount);
        FAIL("PdfError not thrown");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRef);
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    // CVE-2017-5853 2.9 - finally - loop through a set of interesting bit patterns
    static uint64_t s_values[] =
    {
        //(1ull << 64) - 1,
        //(1ull << 64),
        //(1ull << 64) + 1,
        (1ull << 63) - 1,
        (1ull << 63),
        (1ull << 63) + 1,
        (1ull << 62) - 1,
        (1ull << 62),
        (1ull << 62) + 1,

        (1ull << 49) - 1,
        (1ull << 49),
        (1ull << 49) + 1,
        (1ull << 48) - 1,
        (1ull << 48),
        (1ull << 48) + 1,
        (1ull << 47) - 1,
        (1ull << 47),
        (1ull << 47) + 1,

        (1ull << 33) - 1,
        (1ull << 33),
        (1ull << 33) + 1,
        (1ull << 32) - 1,
        (1ull << 32),
        (1ull << 32) + 1,
        (1ull << 31) - 1,
        (1ull << 31),
        (1ull << 31) + 1,

        (1ull << 25) - 1,
        (1ull << 33),
        (1ull << 33) + 1,
        (1ull << 24) - 1,
        (1ull << 24),
        (1ull << 24) + 1,
        (1ull << 31) - 1,
        (1ull << 31),
        (1ull << 31) + 1,

        (1ull << 17) - 1,
        (1ull << 17),
        (1ull << 17) + 1,
        (1ull << 16) - 1,
        (1ull << 16),
        (1ull << 16) + 1,
        (1ull << 15) - 1,
        (1ull << 15),
        (1ull << 15) + 1,

        (uint64_t)-1,
        0,
        1
    };
    const size_t numValues = sizeof(s_values) / sizeof(s_values[0]);

    for (int i = 0; i < static_cast<int>(numValues); i++)
    {
        for (size_t j = 0; j < numValues; j++)
        {
            try
            {
                string strInput = " ";
                PdfIndirectObjectList objects;
                PdfParserTestWrapper parser(objects, strInput);
                firstObject = s_values[i];
                objectCount = s_values[j];

                if (canOutOfMemoryKillUnitTests() && (firstObject > maxNumberOfIndirectObjects || objectCount > maxNumberOfIndirectObjects))
                {
                    // can't call this in test environments where an out-of-memory condition terminates
                    // unit test process before all tests have run (e.g. AddressSanitizer)
                }
                else
                {
                    parser.ReadXRefSubsection(firstObject, objectCount);
                    // some combinations of firstObject/objectCount from s_values are legal - so we expect to reach here sometimes
                }
            }
            catch (PdfError& error)
            {
                // other combinations of firstObject/objectCount from s_values are illegal 
                // if we reach here it should be an invalid xref value of some type
                REQUIRE((error.GetError() == PdfErrorCode::InvalidXRef || error.GetError() == PdfErrorCode::ValueOutOfRange
                    || error.GetError() == PdfErrorCode::NoXRef
                    || error.GetError() == PdfErrorCode::OutOfMemory));
            }
            catch (exception&)
            {
                // and should never reach here
                FAIL("Wrong exception type");
            }
        }
    }
}

TEST_CASE("testReadXRefStreamContents")
{
    // test valid stream
    try
    {
        // generate an XRef stream with valid /W values
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream with 5 entries
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer        
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadXRefStreamContents(offsetXRefObject, false);
        // should succeed
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // CVE-2018-5295: integer overflow caused by checking sum of /W entry values /W [ 1 2 9223372036854775807 ]
    // see https://bugzilla.redhat.com/show_bug.cgi?id=1531897 (/W values used were extracted from PoC file)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 1 2 9223372036854775807 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE((error.GetError() == PdfErrorCode::NoXRef
            || error.GetError() == PdfErrorCode::InvalidXRefStream));
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // CVE-2017-8787: heap based overflow caused by unchecked /W entry values /W [ 1 -4 2 ]
    // see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=861738 for value of /W array
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 1 -4 2 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // /W entry values /W [ 4095 1 1 ] for data in form 02 0002 00 (doesn't match size of entry)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 4095 1 1 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE((offsetEndstream - offsetStream - strlen("\r\n")) == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRefStream);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // /W entry values /W [ 4 4 4 ] for data in form 02 0002 00 (doesn't match size of entry)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 5 ";
        oss << "/W [ 4 4 4 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidXRefType);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // /W entry values /W [ 1 4 4 ] (size=9) for data 01 0E8A 0\r\n02 0002 00\r\n (size=8 bytes)
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        // XRef stream
        size_t lengthXRefObject = 21;
        size_t offsetXRefObject = 0;
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 2 ";
        oss << "/W [ 1 4 4 ] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // check /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        // parse the dictionary then try reading the XRef stream using the invalid /W entries
        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with 5 entries but /Size 2 specified
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 2 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(2);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with 5 entries but /Size 10 specified
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2] ";
        oss << "/Size 10 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(2);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index [0 0] array
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [0 0] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index [-1 -1] array
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [-1 -1] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index array with no entries
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [ ] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        // should this succeed ???
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index array with 3 entries
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [2 2 2] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "01 0E8A 0\r\n";
        oss << "02 0002 00\r\n";
        oss << "02 0002 01\r\n";
        oss << "02 0002 02\r\n";
        oss << "02 0002 03\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }

    // XRef stream with /Index array with 22 entries
    try
    {
        ostringstream oss;
        size_t offsetStream;
        size_t offsetEndstream;

        size_t lengthXRefObject = 57;
        size_t offsetXRefObject = oss.str().length();
        oss << "2 0 obj ";
        oss << "<< /Type /XRef ";
        oss << "/Length " << lengthXRefObject << " ";
        oss << "/Index [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22] ";
        oss << "/Size 5 ";
        oss << "/W [1 2 1] ";
        oss << "/Filter /ASCIIHexDecode ";
        oss << ">>\r\n";
        oss << "stream\r\n";
        offsetStream = oss.str().length();
        oss << "00 0000 0\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        oss << "00 0000 00\r\n";
        offsetEndstream = oss.str().length();
        oss << "endstream\r\n";
        oss << "endobj\r\n";
        REQUIRE(offsetEndstream - offsetStream - strlen("\r\n") == lengthXRefObject); // hard-coded in /Length entry in XRef stream above

        // trailer
        oss << "trailer << /Root 1 0 R /Size 3 >>\r\n";
        oss << "startxref " << offsetXRefObject << "\r\n";
        oss << "%EOF";

        auto inputStr = oss.str();
        PdfXRefEntries offsets;
        auto device = std::make_shared<PdfMemoryInputDevice>(inputStr);
        PdfMemDocument doc;
        doc.LoadFromDevice(device);
        PdfXRefStreamParserObject xrefStreamParser(doc, *device, offsets);

        offsets.Enlarge(5);
        xrefStreamParser.Parse();
        xrefStreamParser.ReadXRefTable();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::NoXRef);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }
}

TEST_CASE("testReadObjects")
{
    // CVE-2017-8378 - m_offsets out-of-bounds access when referenced encryption dictionary object doesn't exist
    try
    {
        // generate an xref section
        // xref
        // 0 3
        // 0000000000 65535 f 
        // 0000000018 00000 n 
        // 0000000077 00000 n
        // trailer << /Root 1 0 R /Size 3 >>
        // startxref
        // 0
        // %%EOF
        ostringstream oss;
        oss << "%PDF–1.0\r\n";
        oss << "xref\r\n0 3\r\n";
        oss << generateXRefEntries(3);
        oss << "trailer << /Root 1 0 R /Size 3 /Encrypt 2 0 R >>\r\n";
        oss << "startxref 0\r\n";
        oss << "%EOF";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, oss.str());
        parser.ReadObjects();
        FAIL("Should throw exception");
    }
    catch (PdfError& error)
    {
        REQUIRE(error.GetError() == PdfErrorCode::InvalidEncryptionDict);
    }
    catch (exception&)
    {
        FAIL("Unexpected exception type");
    }
}

TEST_CASE("testIsPdfFile")
{
    try
    {
        string strInput = "%PDF-1.0";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-1.1";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-1.7";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-1.9";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(!parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%PDF-2.0";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "%!PS-Adobe-2.0";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(!parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }

    try
    {
        string strInput = "GIF89a";
        PdfIndirectObjectList objects;
        PdfParserTestWrapper parser(objects, strInput);
        REQUIRE(!parser.IsPdfFile());
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
    catch (exception&)
    {
        FAIL("Wrong exception type");
    }
}

TEST_CASE("testRoundTripIndirectTrailerID")
{
    ostringstream oss;
    oss << "%PDF-1.1\n";
    unsigned currObj = 0;
    streamoff objPos[20];

    // Pages

    unsigned pagesObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "<</Type /Pages /Count 0 /Kids []>>\n";
    oss << "endobj";

    // Root catalog

    unsigned rootObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "<</Type /Catalog /Pages " << pagesObj << " 0 R>>\n";
    oss << "endobj\n";

    // ID
    unsigned idObj = currObj;
    objPos[currObj] = oss.tellp();
    oss << currObj++ << " 0 obj\n";
    oss << "[<F1E375363A6314E3766EDF396D614748> <F1E375363A6314E3766EDF396D614748>]\n";
    oss << "endobj\n";

    streamoff xrefPos = oss.tellp();
    oss << "xref\n";
    oss << "0 " << currObj << "\n";
    string objRec;
    for (unsigned i = 0; i < currObj; i++)
    {
        cmn::FormatTo(objRec, "{:010d} 00000 n \n", objPos[i]);
        oss << objRec;
    }
    oss << "trailer <<\n"
        << "  /Size " << currObj << "\n"
        << "  /Root " << rootObj << " 0 R\n"
        << "  /ID " << idObj << " 0 R\n" // indirect ID
        << ">>\n"
        << "startxref\n"
        << xrefPos << "\n"
        << "%%EOF\n";

    string inputBuff = oss.str();
    try
    {
        PdfMemDocument doc;
        // load for update
        doc.LoadFromBuffer(inputBuff);

        string outBuf;
        PdfStringOutputDevice outDev(outBuf);

        doc.SaveUpdate(outBuf);
        // should not throw
        REQUIRE(true);
    }
    catch (PdfError&)
    {
        FAIL("Unexpected PdfError");
    }
}

string generateXRefEntries(size_t count)
{
    string strXRefEntries;

    // generates a block of 20-byte xref entries
    // 0000000000 65535 f\r\n
    // 0000000120 00000 n\r\n    
    // 0000000120 00000 n\r\n
    // 0000000120 00000 n\r\n
    try
    {
        strXRefEntries.reserve(count * 20);
        for (size_t i = 0; i < count; i++)
        {
            if (i == 0)
                strXRefEntries.append("0000000000 65535 f\r\n");
            else
                strXRefEntries.append("0000000120 00000 n\r\n");
        }
    }
    catch (exception&)
    {
        // if this fails it's a bug in the unit tests and not PoDoFo
        FAIL("generateXRefEntries memory allocation failure");
    }

    return strXRefEntries;
}

bool canOutOfMemoryKillUnitTests()
{
    // test if out of memory conditions will kill the unit test process
    // which prevents tests completing

#if defined(_WIN32)
    // on Windows 32/64 allocations close to size of VM address space always fail gracefully
    bool canTerminateProcess = false;
#elif defined( __APPLE__ )
    // on macOS/iOS allocations close to size of VM address space fail gracefully
    // unless Address Sanitizer (ASAN) is enabled
#if __has_feature(address_sanitizer)
    // ASAN terminates the process if alloc fails - and using allocator_may_return_null=1
    // to continue after an allocation doesn't work in C++ because new returns null which is
    // forbidden by the C++ spec and terminates process when 'this' is dereferenced in constructor
    // see https://github.com/google/sanitizers/issues/295
    bool canTerminateProcess = true;
#else
    // if alloc fails following message is logged
    // *** mach_vm_map failed (error code=3)
    // *** error: can't allocate region
    // *** set a breakpoint in malloc_error_break to debug
    bool canTerminateProcess = false;
#endif
#elif defined( __linux__ )
    // TODO do big allocs succeed then trigger OOM-killer fiasco??
    bool canTerminateProcess = false;
#else
    // other systems - assume big allocs faily gracefully and throw bad_alloc
    bool canTerminateProcess = false;
#endif
    return canTerminateProcess;
}


