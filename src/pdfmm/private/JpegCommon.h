#ifndef JPEG_COMMON_H
#define JPEG_COMMON_H

#include <cstdio>

extern "C" {
#include <jpeglib.h>
}

extern "C"
{
    void JPegErrorExit(j_common_ptr cinfo);

    void JPegErrorOutput(j_common_ptr, int);
};

namespace mm
{
    void jpeg_memory_src(j_decompress_ptr cinfo, const JOCTET* buffer, size_t bufsize);
}

#endif // JPEG_COMMON_H
