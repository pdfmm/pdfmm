#ifndef SPAN_H
#define SPAN_H

#include "span.hpp"

namespace cmn
{
    // https://stackoverflow.com/questions/56845801/what-happened-to-stdcspan
    /** Constant span
     */
    template <class T, size_t Extent = tcb::dynamic_extent>
    using cspan = tcb::span<const T, Extent>;

    /** Mutable span
     */
    template <class T, size_t Extent = tcb::dynamic_extent, typename std::enable_if<!std::is_const<T>::value, int>::type = 0>
    using mspan = tcb::span<T, Extent>;
}

#endif // SPAN_H
