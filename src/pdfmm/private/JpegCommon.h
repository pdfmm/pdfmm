#ifndef JPEG_COMMON_H
#define JPEG_COMMON_H

#include <pdfmm/base/PdfDeclarations.h>
#include <csetjmp>

extern "C" {
#include <jpeglib.h>
}

struct JpegErrorHandler
{
    jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

struct JpegBufferDestination
{
    jpeg_destination_mgr pub;
    mm::charbuff* buff = nullptr;
};

#define INIT_JPEG_ERROR_HANDLER(ctx, jerr)\
if (setjmp(jerr.setjmp_buffer))\
{\
    std::string error;\
    error.resize(JMSG_LENGTH_MAX);\
    (*ctx.err->format_message) ((jpeg_common_struct*)&ctx, error.data());\
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, error);\
}

#define INIT_JPEG_COMPRESS_CONTEXT(ctx, jerr)\
    mm::InitJpegCompressContext(ctx, jerr);\
    INIT_JPEG_ERROR_HANDLER(ctx, jerr)

#define INIT_JPEG_DECOMPRESS_CONTEXT(ctx, jerr)\
    mm::InitJpegDecompressContext(ctx, jerr);\
    INIT_JPEG_ERROR_HANDLER(ctx, jerr)

namespace mm
{
    // NOTE: Don't use directly, use INIT_JPEG_COMPRESS_CONTEXT
    void InitJpegCompressContext(jpeg_compress_struct& ctx, JpegErrorHandler& jerr);
    // NOTE: Don't use directly, use INIT_JPEG_DECOMPRESS_CONTEXT
    void InitJpegDecompressContext(jpeg_decompress_struct& ctx, JpegErrorHandler& jerr);
    void SetJpegBufferDestination(jpeg_compress_struct& ctx, charbuff& buff, JpegBufferDestination& jdest);
    void jpeg_memory_src(j_decompress_ptr cinfo, const JOCTET* buffer, size_t bufsize);
}

#endif // JPEG_COMMON_H
