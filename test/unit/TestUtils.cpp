/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "TestUtils.h"

#include <fstream>
#include <filesystem>

#include <catch.hpp>

#include <utfcpp/utf8.h>
#include <TestConfig.h>

using namespace std;
using namespace mm;

static void readTestInputFileTo(string& str, const string_view& filepath);

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

void TestUtils::ReadTestInputFileTo(string& str, const string_view& filename)
{
    readTestInputFileTo(str, GetTestInputFilePath(filename));
}

void TestUtils::AssertEqual(double expected, double actual, double threshold)
{
    if (std::abs(actual - expected) > threshold)
        FAIL("Expected different than actual");
}

void readTestInputFileTo(string& str, const string_view& filepath)
{
#ifdef _WIN32
    auto filepath16 = utf8::utf8to16((string)filepath);
    ifstream stream((wchar_t*)filepath16.c_str(), ios_base::in | ios_base::binary);
#else
    ifstream stream((string)filepath, ios_base::in | ios_base::binary);
#endif

    stream.seekg(0, ios::end);
    if (stream.tellg() == -1)
        throw runtime_error("Error reading from stream");

    str.reserve((size_t)stream.tellg());
    stream.seekg(0, ios::beg);

    str.assign((istreambuf_iterator<char>(stream)), istreambuf_iterator<char>());
}
