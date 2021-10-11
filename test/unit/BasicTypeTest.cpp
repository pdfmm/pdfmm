/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "BasicTypeTest.h"
#include <podofo.h>

#include <limits>

using namespace PoDoFo;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(BasicTypeTest);

void BasicTypeTest::setUp()
{
}

void BasicTypeTest::tearDown()
{
}

void BasicTypeTest::testXrefOffsetTypeSize()
{
    CPPUNIT_ASSERT_MESSAGE("pdf_uint64 is big enough to hold an xref entry", std::numeric_limits<uint64_t>::max() >= 9999999999);
}

void BasicTypeTest::testDefaultMaximumNumberOfObjects()
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE("PdfReference allows 8,388,607 indirect objects.", 8388607L, PdfParser::GetMaxObjectCount());
}
