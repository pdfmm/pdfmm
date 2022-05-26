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
#include <sstream>
#include <filesystem>

 /** Check if a suitable error message is returned
  * Asserts that the given expression throws an exception of the specified type
  */
#define ASSERT_THROW_WITH_ERROR_CODE(expression, errorCode)                     \
    do                                                                          \
    {                                                                           \
        bool cpputCorrectExceptionThrown_ = false;                              \
        std::ostringstream stream;                                              \
        stream << "expected exception not thrown" << std::endl;                 \
        stream << "Expected: " #errorCode << std::endl;                         \
                                                                                \
        try                                                                     \
        {                                                                       \
            expression;                                                         \
        }                                                                       \
        catch (const PdfError &e)                                               \
        {                                                                       \
            if (e.GetError() == errorCode)                                      \
            {                                                                   \
                cpputCorrectExceptionThrown_ = true;                            \
            }                                                                   \
            else                                                                \
            {                                                                   \
                stream << "Error type mismatch. Actual: " << #errorCode         \
                    << std::endl;                                               \
                stream << "What: " << PdfError::ErrorName(e.GetError());        \
            }                                                                   \
        }                                                                       \
        catch (const std::exception &e)                                         \
        {                                                                       \
            stream << "Actual std::exception or derived" << std::endl;          \
            stream << "What: " << e.what() << std::endl;                        \
        }                                                                       \
        catch (...)                                                             \
        {                                                                       \
            stream << "Actual exception unknown." << std::endl;                 \
        }                                                                       \
                                                                                \
        if (cpputCorrectExceptionThrown_)                                       \
           break;                                                               \
                                                                                \
        FAIL(stream.str());                                                     \
    } while (false);


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
        static void ReadTestInputFileTo(std::string& str, const std::string_view& filename);
    };
}

#endif // TEST_UTILS_H
