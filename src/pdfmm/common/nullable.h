#ifndef COMMON_NULLABLE_H
#define COMMON_NULLABLE_H
#pragma once

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace COMMON_NAMESPACE
{
    class bad_nullable_access : public std::runtime_error
    {
    public:
        bad_nullable_access()
            : std::runtime_error("nullable object doesn't have a value") { }
    };

    /**
     * Alternative to std::optional that supports reference (but not pointer) types
     */
    template <typename T, typename = std::enable_if_t<!std::is_pointer<T>::value>>
    class nullable final
    {
    public:
        nullable()
            : m_hasValue(false), m_value{ } { }

        nullable(T value)
            : m_hasValue(true), m_value(std::move(value)) { }

        nullable(std::nullptr_t)
            : m_hasValue(false), m_value{ } { }

        nullable(const nullable& value) = default;

        nullable& operator=(const nullable& value) = default;

        nullable& operator=(T value)
        {
            m_hasValue = true;
            m_value = std::move(value);
            return *this;
        }

        nullable& operator=(std::nullptr_t)
        {
            m_hasValue = false;
            m_value = { };
            return *this;
        }

        const T& value() const
        {
            if (!m_hasValue)
                throw bad_nullable_access();

            return m_value;
        }

        T& value()
        {
            if (!m_hasValue)
                throw bad_nullable_access();

            return m_value;
        }

        bool has_value() const { return m_hasValue; }
        const T* operator->() const { return &m_value; }
        T* operator->() { return &m_value; }
        const T& operator*() const { return m_value; }
        T& operator*() { return m_value; }

    public:
        template <typename T2>
        friend bool operator==(const nullable<T2>& op1, const nullable<T2>& op2);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op1, const nullable<T2>& op2);

        template <typename T2>
        friend bool operator==(const nullable<T2>& op1, const nullable<const T2&>& op2);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op1, const nullable<const T2&>& op2);

        template <typename T2>
        friend bool operator==(const nullable<T2>& op, const T2& value);

        template <typename T2>
        friend bool operator==(const T2& value, const nullable<T2>& op);

        template <typename T2>
        friend bool operator==(const nullable<T2>& op, std::nullptr_t);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op, const T2& value);

        template <typename T2>
        friend bool operator!=(const T2& value, const nullable<T2>& op);

        template <typename T2>
        friend bool operator==(std::nullptr_t, const nullable<T2>& op);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op, std::nullptr_t);

        template <typename T2>
        friend bool operator!=(std::nullptr_t, const nullable<T2>& op);

    private:
        bool m_hasValue;
        T m_value;
    };

    // Template spacialization for references
    template <typename T>
    class nullable<T&> final
    {
    public:
        nullable()
            : m_hasValue(false), m_value{ } { }

        nullable(T& value)
            : m_hasValue(true), m_value(&value) { }

        nullable(std::nullptr_t)
            : m_hasValue(false), m_value{ } { }

        nullable(const nullable& value) = default;

        // NOTE: We dont't do rebinding from other references
        nullable& operator=(const nullable& value) = delete;

        const T& value() const
        {
            if (!m_hasValue)
                throw bad_nullable_access();

            return *m_value;
        }

        T& value()
        {
            if (!m_hasValue)
                throw bad_nullable_access();

            return *m_value;
        }

        bool has_value() const { return m_hasValue; }
        const T* operator->() const { return m_value; }
        T* operator->() { return m_value; }
        const T& operator*() const { return *m_value; }
        T& operator*() { return *m_value; }

    public:
        // NOTE: We don't provide comparison against value since
        // it would be ambiguous

        template <typename T2>
        friend bool operator==(const nullable<T2>& op1, const nullable<T2>& op2);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op1, const nullable<T2>& op2);

        template <typename T2>
        friend bool operator==(const nullable<T2>& op1, const nullable<const T2&>& op2);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op1, const nullable<const T2&>& op2);

        template <typename T2>
        friend bool operator==(const nullable<T2>& op, std::nullptr_t);

        template <typename T2>
        friend bool operator==(std::nullptr_t, const nullable<T2>& op);

        template <typename T2>
        friend bool operator!=(const nullable<T2>& op, std::nullptr_t);

        template <typename T2>
        friend bool operator!=(std::nullptr_t, const nullable<T2>& op);

    private:
        bool m_hasValue;
        T* m_value;
    };

    template <typename T2>
    bool operator==(const nullable<T2>& n1, const nullable<T2>& n2)
    {
        if (n1.m_hasValue != n2.m_hasValue)
            return false;

        if (n1.m_hasValue)
            return n1.m_value == n2.m_value;
        else
            return true;
    }

    template <typename T2>
    bool operator!=(const nullable<T2>& op1, const nullable<T2>& op2)
    {
        if (op1.m_hasValue != op2.m_hasValue)
            return true;

        if (op1.m_hasValue)
            return op1.m_value != op2.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator==(const nullable<T2>& op1, const nullable<const T2&>& op2)
    {
        if (op1.m_hasValue != op2.m_hasValue)
            return true;

        if (op1.m_hasValue)
            return op1.m_value != *op2.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator!=(const nullable<T2>& op1, const nullable<const T2&>& op2)
    {
        if (op1.m_hasValue != op2.m_hasValue)
            return true;

        if (op1.m_hasValue)
            return op1.m_value != *op2.m_value;
        else
            return false;
    }

    template <typename T2>
    bool operator==(const nullable<T2>& n, const T2& v)
    {
        if (!n.m_hasValue)
            return false;

        return n.m_value == v;
    }

    template <typename T2>
    bool operator!=(const nullable<T2>& n, const T2& v)
    {
        if (!n.m_hasValue)
            return true;

        return n.m_value != v;
    }

    template <typename T2>
    bool operator==(const T2& v, const nullable<T2>& n)
    {
        if (!n.m_hasValue)
            return false;

        return n.m_value == v;
    }

    template <typename T2>
    bool operator!=(const T2& v, const nullable<T2>& n)
    {
        if (!n.m_hasValue)
            return false;

        return n.m_value != v;
    }

    template <typename T2>
    bool operator==(const nullable<T2>& n, std::nullptr_t)
    {
        return !n.m_hasValue;
    }

    template <typename T2>
    bool operator!=(const nullable<T2>& n, std::nullptr_t)
    {
        return n.m_hasValue;
    }

    template <typename T2>
    bool operator==(std::nullptr_t, const nullable<T2>& n)
    {
        return !n.m_hasValue;
    }

    template <typename T2>
    bool operator!=(std::nullptr_t, const nullable<T2>& n)
    {
        return n.m_hasValue;
    }
}

#endif // COMMON_NULLABLE_H
