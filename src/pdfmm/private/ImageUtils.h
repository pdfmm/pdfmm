/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <pdfmm/base/PdfOutputStream.h>

#ifdef PDFMM_HAVE_JPEG_LIB
#include <pdfmm/private/JpegCommon.h>
#endif // PDFMM_HAVE_JPEG_LIB

#include <pdfium/core/fxcodec/scanlinedecoder.h>

namespace utls
{
    /** Fetch a RGB image and write it to the stream
     */
    void FetchImageRGB(mm::OutputStream& stream, unsigned width, unsigned heigth, mm::PdfPixelFormat format,
        const unsigned char* imageData, const mm::charbuff& smaskData, mm::charbuff& scanLine);

    /** Fetch a GrayScale image and write it to the stream
     */
    void FetchImageGrayScale(mm::OutputStream& stream, unsigned width, unsigned heigth, mm::PdfPixelFormat format,
        const unsigned char* imageData, const mm::charbuff& smaskData, mm::charbuff& scanLine);

    /** Fetch a black and white image and write it to the stream
     */
    void FetchImageBW(mm::OutputStream& stream, unsigned width, unsigned heigth, mm::PdfPixelFormat format,
        fxcodec::ScanlineDecoder& decoder, const mm::charbuff& smaskData, mm::charbuff& scanLine);

#ifdef PDFMM_HAVE_JPEG_LIB
    void FetchImageJPEG(mm::OutputStream& stream, mm::PdfPixelFormat format, jpeg_decompress_struct* ctx,
        JSAMPARRAY jScanLine, const mm::charbuff& smaskData, mm::charbuff& scanLine);
#endif // PDFMM_HAVE_JPEG_LIB
}

#endif // IMAGE_UTILS_H
