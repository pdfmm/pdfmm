#ifndef PDFMM_FORMAT_H
#define PDFMM_FORMAT_H

// Define FMT_FORMAT_H externally to force a difference location for {fmt}
#ifndef FMT_FORMAT_H
#define FMT_FORMAT_H "fmt/format.h"
#endif

#define FMT_HEADER_ONLY
#include FMT_FORMAT_H

/** \def PDFMM_FORMAT(fmt, ...)
 *
  * Format the string, if needed
 */
#define PDFMM_FORMAT(fmt, ...) ::mm::FormatHelper::TryFormat(fmt, __VA_ARGS__)

namespace mm
{
    template <typename... Args>
    inline static std::string Format(const std::string_view& fmt, const Args&... args)
    {
        return fmt::format(fmt, args...);
    }

    template <typename... Args>
    inline static void FormatTo(std::string& dst, const std::string_view& fmt, const Args&... args)
    {
        fmt::format_to(std::back_inserter(dst), fmt, args...);
    }

    template <typename... Args>
    inline static void FormatTo(char* dst, size_t n, const std::string_view& fmt, const Args&... args)
    {
        fmt::format_to_n(dst, n, fmt, args...);
    }

    /** Helper class. Use PDFMM_FORMAT macro instead
     */
    class FormatHelper
    {
    public:
        inline static std::string TryFormat(const std::string_view& str)
        {
            return (std::string)str;
        }

        inline static std::string TryFormat(const char* str)
        {
            return str;
        }

        inline static std::string TryFormat(const std::string& str)
        {
            return str;
        }

        inline static std::string TryFormat(std::string&& str)
        {
            return std::move(str);
        }

        template <typename... Args>
        inline static std::string TryFormat(const std::string_view& fmt, const Args&... args)
        {
            return fmt::format(fmt, args...);
        }
    };
}

#endif // PDFMM_FORMAT_H
