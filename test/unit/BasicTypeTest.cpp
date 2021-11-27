/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <catch.hpp>
#include <limits>

#include <pdfmm/pdfmm.h>
#include <iostream>
using namespace std;
using namespace mm;

/** This class tests the basic integer and other types PoDoFo uses
 *  to make sure they satisfy its requirements for behaviour, size, etc.
 */

TEST_CASE("BasicTypeTest")
{
    REQUIRE(std::numeric_limits<uint64_t>::max() >= 9999999999);
}
