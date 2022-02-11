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
#include <filesystem>

namespace mm
{
    namespace fs = std::filesystem;

    /**
     * This class contains utility methods that are
     * often needed when writing tests.
     */
    class TestUtils
    {
    public:
        static std::string GetTestOutputFilePath(const std::string_view& filename);
        static std::string GetTestInputFilePath(const std::string_view& filename);
        static const fs::path& GetTestInputPath();
        static const fs::path& GetTestOutputPath();
    };
}

#endif // TEST_UTILS_H
