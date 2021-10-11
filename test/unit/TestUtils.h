/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>

/**
 * This class contains utility methods that are
 * often needed when writing tests.
 */
class TestUtils {

public:
    static std::string getTempFilename();
    static void deleteFile(const char* pszFilename);

    /**
     * Read a test data file into memory and return a malloc'ed buffer.
     *
     * @param pszFilename filename of the data file. The path will be determined automatically.
     */
    static char* readDataFile(const char* pszFilename);
};

#endif // TEST_UTILS_H
