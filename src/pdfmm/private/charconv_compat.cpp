#include "charconv_compat.h"

#ifdef WANT_TO_CHARS

#include <stdio.h>
#include <locale.h>

#if _MSC_VER

// In MSVC snprintf_l is available as _snprintf_l
// https://docs.microsoft.com/it-it/cpp/c-runtime-library/reference/snprintf-s-snprintf-s-l-snwprintf-s-snwprintf-s-l?view=msvc-170

#define snprintf_l(str, size, loc, format, ...) _snprintf_l(str, size, format, loc, __VA_ARGS__)
#define locale_t _locale_t
#define newlocale(category_mask, locale, base) _create_locale(category_mask, locale)
#define freelocale _free_locale

#elif __APPLE__

// Apple has snprintf_l
#include <xlocale.h>

#else // Linux

#include <stdarg.h>

// Fallback snprintf_l implementation
inline int snprintf_l(char* buffer, size_t size, locale_t locale, const char* format, ...)
{
    locale_t prevloc = uselocale((locale_t)0);
    uselocale(locale);
    va_list args;
    va_start(args, format);
    int rc = vsnprintf(buffer, size, format, args);
    va_end(args);
    uselocale(prevloc);
    return rc;
}

#endif

struct Locale
{
    Locale()
    {
        Handle = newlocale(LC_ALL, "C", NULL);
    }
    ~Locale()
    {
        freelocale(Handle);
    }

    locale_t Handle;
};

Locale s_locale;

namespace std
{
    to_chars_result to_chars(char* first, char* last, double value,
        std::chars_format fmt, unsigned char precision)
    {
        (void)fmt;
        int rc = snprintf_l(first, last - first + 1, s_locale.Handle, "%.*f", precision, value);
        return { first + (rc < 0 ? 0 : rc), rc < 0 ? errc::value_too_large : errc{ } };
    }

    to_chars_result to_chars(char* first, char* last, float value,
        std::chars_format fmt, unsigned char precision)
    {
        (void)fmt;
        int rc = snprintf_l(first, last - first + 1, s_locale.Handle, "%.*f", precision, value);
        return { first + (rc < 0 ? 0 : rc), rc < 0 ? errc::value_too_large : errc{ } };
    }
}

#endif // WANT_TO_CHARS
