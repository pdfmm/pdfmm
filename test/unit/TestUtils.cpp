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

#ifdef _WIN32
#include <pdfmm/private/WindowsLeanMean.h>
#elif __linux__
#include <unistd.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <limits.h>
#endif

#include <TestConfig.h>

using namespace std;
namespace fs = std::filesystem;

static string GetExecutablePath();

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

#ifdef _WIN32

string GetExecutablePath()
{
    wchar_t buf[MAX_PATH];
    DWORD rc = GetModuleFileNameW(NULL, buf, MAX_PATH);
    if (rc == 0 || rc == sizeof(buf))
        throw runtime_error("Can't get module file name");

    return utf8::utf16to8(u16string_view((char16_t*)buf, rc));
}

#elif __linux__

string GetExecutablePath()
{
    char buf[1024] = { 0 };
    ssize_t size = readlink("/proc/self/exe", buf, sizeof(buf));
    if (size == 0 || size == sizeof(buf))
        throw std::runtime_error("Can't read executable path on /proc/self/exe");
    return std::string(buf, size);
}

#elif __APPLE__

string GetExecutablePath()
{
    string ret[PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    if (!_NSGetExecutablePath(buf, &bufsize))
        throw std::runtime_error("Can't read executable path");

    ret.resize(bufsize);
    return ret;
}

#endif
