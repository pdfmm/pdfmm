#ifndef CHARCONV_COMPAT_H
#define CHARCONV_COMPAT_H

#include <charconv>

// gcc and clang may have no floating point from_chars
#if defined(__GNUC__) || defined(__clang__)

#include <fast_float.h>

namespace std
{
    using chars_format = fast_float::chars_format;

    inline from_chars_result from_chars(const char* first, const char* last, double& value,
        chars_format fmt = chars_format::general)
    {
        auto ret = fast_float::from_chars(first, last, value, fmt);
        return { ret.ptr, ret.ec };
    }
}

#endif

#endif // CHARCONV_COMPAT_H
