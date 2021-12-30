/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "TestUtils.h"

#include <iostream>

#ifdef _WIN32
#include <pdfmm/private/WindowsLeanMean.h>
#else
#include <stdlib.h>
#endif // _WIN32

using namespace std;

string TestUtils::getTempFilename()
{
    constexpr unsigned len = 260;
    char tmpFilename[len];
#ifdef _WIN32
	char tmpDir[len];
	GetTempPathA(len, tmpDir);
	GetTempFileNameA(tmpDir, "podofo", 0, tmpFilename);
#else // Unix
    strncpy( tmpFilename, "/tmp/podofoXXXXXX", len);
    int handle = mkstemp(tmpFilename);
    close(handle);
#endif

    cout << "Created tempfile:" << tmpFilename << endl;
    return tmpFilename;
}

void TestUtils::deleteFile(const string_view& filename)
{
#ifdef _WIN32
    _unlink(filename.data());
#else // Unix
    unlink(filename.data());
#endif
}

