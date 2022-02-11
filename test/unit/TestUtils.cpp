/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "TestUtils.h"

#include <utfcpp/utf8.h>
#include <filesystem>

#include <TestConfig.h>

using namespace std;
using namespace mm;

static struct TestPaths
{
    TestPaths() :
        Input(PDF_TEST_RESOURCE_PATH), Output(PDF_TEST_OUTPUT_PATH) { }
    fs::path Input;
    fs::path Output;
} s_paths;


string TestUtils::GetTestOutputFilePath(const string_view& filename)
{
    return (s_paths.Output / filename).u8string();
}

string TestUtils::GetTestInputFilePath(const string_view& filename)
{
    return (s_paths.Input / filename).u8string();
}

const fs::path& TestUtils::GetTestInputPath()
{
    return s_paths.Input;
}

const fs::path& TestUtils::GetTestOutputPath()
{
    return s_paths.Output;
}
