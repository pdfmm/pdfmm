/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "TestUtils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

#ifdef CreateFont
#undef CreateFont
#endif // CreateFont

#ifdef DrawText
#undef DrawText
#endif // DrawText

#endif // _WIN32 || _WIN64

#include <podofo.h>

std::string TestUtils::getTempFilename()
{
    const long lLen = 256;
    char tmpFilename[lLen];
#if defined(_WIN32) || defined(_WIN64)
	char tmpDir[lLen];
	GetTempPathA(lLen, tmpDir);
	GetTempFileNameA(tmpDir, "podofo", 0, tmpFilename);
#else
    strncpy( tmpFilename, "/tmp/podofoXXXXXX", lLen);
    int handle = mkstemp(tmpFilename);
    close(handle);
#endif // _WIN32 || _WIN64

    printf("Created tempfile: %s\n", tmpFilename);
    std::string sFilename = tmpFilename;
    return sFilename;
}

void TestUtils::deleteFile( const char* pszFilename )
{
#if defined(_WIN32) || defined(_WIN64)
    _unlink(pszFilename);
#else
    unlink(pszFilename);
#endif // _WIN32 || _WIN64
}

char* TestUtils::readDataFile( const char* pszFilename )
{
    // TODO: determine correct prefix during runtime
    std::string sFilename = "/home/dominik/podofotmp/test/unit/data/";
    sFilename = sFilename + pszFilename;
    PoDoFo::PdfFileInputStream stream( sFilename.c_str() );
    long lLen = stream.GetFileLength();

    char* pBuffer = static_cast<char*>(malloc(sizeof(char) * lLen));
    stream.Read(pBuffer, lLen);

    return pBuffer;
}


