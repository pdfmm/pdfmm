#ifndef CHARCONV_COMPAT_H
#define CHARCONV_COMPAT_H

#include <charconv>

// Older gcc and clang may have no floating point from_chars
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 11
#define USE_FAST_FLOAT
#define USE_FAST_FLOAT_CHARS_FORMAT
#elif defined(__clang__) && __clang_major__ < 14
#define USE_FAST_FLOAT
#endif

#ifdef USE_FAST_FLOAT
#include <fast_float.h>

namespace std
{
#ifdef USE_FAST_FLOAT_CHARS_FORMAT
using chars_format = fast_float::chars_format;
#endif // USE_FAST_FLOAT_CHARS_FORMAT

inline from_chars_result from_chars(const char* first, const char* last, double& value,
    chars_format fmt = chars_format::general)
{
    auto ret = fast_float::from_chars(first, last, value, (fast_float::chars_format)fmt);
    return { ret.ptr, ret.ec };
}
}

#endif // USE_FAST_FLOAT

#endif // CHARCONV_COMPAT_H
