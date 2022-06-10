#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H
#pragma once

#include "span.h"

namespace COMMON_NAMESPACE
{
    /** Convenient read-only char buffer span/view
     */
    using bufferview = cspan<char>;

    /** Unicode code point view
     */
    using unicodeview = cspan<char32_t>;

    // TODO: Optimize, maintaining string compatibility
    // Use basic_string::resize_and_overwrite in C++23
    // https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
    /**
     * Convenient type for char array storage and/or buffer with
     * std::string compatibility
     */
    template <typename = void>
    class charbuff_t final : public std::string
    {
    public:
        using ptr = std::shared_ptr<charbuff_t>;
        using const_ptr = std::shared_ptr<charbuff_t>;

    public:
        charbuff_t() { }
        charbuff_t(const charbuff_t&) = default;
        charbuff_t(charbuff_t&&) = default;
        charbuff_t(size_t size)
        {
            resize(size);
        }
        charbuff_t(std::string&& str)
            : std::string(std::move(str)) { }
        explicit charbuff_t(const bufferview& view)
            : std::string(view.data(), view.size()) { }
        explicit charbuff_t(const std::string_view& view)
            : std::string(view) { }
        explicit charbuff_t(const std::string& str)
            : std::string(str) { }

    public:
        charbuff_t& operator=(const charbuff_t&) = default;
        charbuff_t& operator=(charbuff_t&&) = default;
        charbuff_t& operator=(const std::string_view & view)
        {
            std::string::assign(view.data(), view.size());
            return *this;
        }
        charbuff_t& operator=(const std::string& str)
        {
            std::string::assign(str.data(), str.size());
            return *this;
        }
        charbuff_t& operator=(const bufferview& view)
        {
            std::string::assign(view.data(), view.size());
            return *this;
        }
        charbuff_t& operator=(std::string&& str)
        {
            std::string::operator=(std::move(str));
            return *this;
        }
        operator bufferview() const
        {
            return bufferview(data(), size());
        }
    };

    using charbuff = charbuff_t<>;

    /** A const data provider that can hold a view to a
     * static segments or a shared buffer
     *
     */
    template <typename = void>
    class datahandle_t final
    {
    public:
        datahandle_t() { }
        datahandle_t(const bufferview& view)
            : m_view(view) { }
        datahandle_t(const charbuff_t<>::const_ptr& buff)
            : m_view(*buff), m_buff(buff) { }
    public:
        const bufferview& view() const { return m_view; }
    private:
        bufferview m_view;
        charbuff_t<>::const_ptr m_buff;
    };

    using datahandle = datahandle_t<>;
}

#endif // COMMON_TYPES_H
