/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#pragma once

#include <sstream>

/** Check if a suitable error message is returned
 * Asserts that the given expression throws an exception of the specified type
 */
#define ASSERT_THROW_WITH_ERROR_CODE(expression, errorCode)      \
    do                                                                          \
    {                                                                           \
        bool cpputCorrectExceptionThrown_ = false;                              \
        std::ostringstream stream;                                              \
        stream << "expected exception not thrown" << std::endl;                 \
        stream << "Expected: " #errorCode << std::endl;                     \
                                                                                \
        try                                                                     \
        {                                                                       \
            expression;                                                         \
        }                                                                       \
        catch (const PdfError &e)                                          \
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
