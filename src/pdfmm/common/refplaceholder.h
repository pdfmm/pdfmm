#ifndef PDFMM_REF_PLACEHOLDER_H
#define PDFMM_REF_PLACEHOLDER_H
#pragma once

#include <type_traits>

namespace mm {

/** Reference wrapper that is nullable
 */
template <typename T, typename = std::enable_if_t<std::is_class<T>::value>>
class refplaceholder
{
public:
    refplaceholder() : m_ptr(nullptr) { }
    refplaceholder(T& ref) : m_ptr(&ref) { }
    T& get() { return *m_ptr; }
    const T& get() const { return *m_ptr; }
    T& operator*() { return *m_ptr; }
    const T& operator*() const { return *m_ptr; }
public:
    refplaceholder(const refplaceholder&) = default;
    refplaceholder& operator=(const refplaceholder&) = default;
private:
    T* m_ptr;
};

} // namespace mm

#endif // PDFMM_REF_PLACEHOLDER_H
