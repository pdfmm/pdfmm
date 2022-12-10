/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include "PdfDeclarationsPrivate.h"
#include "ImageUtils.h"

using namespace std;
using namespace mm;

static void fetchScanLineRGB(unsigned char* dstScanLine, unsigned width,
    PdfPixelFormat format, const unsigned char* srcScanLine);
static void fetchScanLineRGB(unsigned char* dstScanLine, unsigned width,
    PdfPixelFormat format, const unsigned char* srcScanLine,
    const unsigned char* srcAphaLine);
static void fetchScanLineGrayScale(unsigned char* dstScanLine, unsigned width,
    PdfPixelFormat format, const unsigned char* srcScanLine);
static void fetchScanLineGrayScale(unsigned char* dstScanLine, unsigned width,
    PdfPixelFormat format, const unsigned char* srcScanLine,
    const unsigned char* srcAphaLine);


void utls::FetchImageRGB(OutputStream& stream, unsigned width, unsigned heigth, PdfPixelFormat format,
    const unsigned char* imageData, const charbuff& smaskData, charbuff& scanLine)
{
    unsigned srcRowSize = width * 3;
    if (smaskData.size() == 0)
    {
        for (unsigned i = 0; i < heigth; i++)
        {
            fetchScanLineRGB((unsigned char*)scanLine.data(),
                width, format, imageData + i * srcRowSize);
            stream.Write(scanLine.data(), scanLine.size());
        }
    }
    else
    {
        for (unsigned i = 0; i < heigth; i++)
        {
            fetchScanLineRGB((unsigned char*)scanLine.data(),
                width, format, imageData + i * srcRowSize,
                (const unsigned char*)smaskData.data() + i * width);
            stream.Write(scanLine.data(), scanLine.size());
        }
    }
}

void utls::FetchImageGrayScale(OutputStream& stream, unsigned width, unsigned heigth, PdfPixelFormat format,
    const unsigned char* imageData, const charbuff& smaskData, charbuff& scanLine)
{
    unsigned srcRowSize = width * 3;
    if (smaskData.size() == 0)
    {
        for (unsigned i = 0; i < heigth; i++)
        {
            fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                width, format, imageData + i * srcRowSize);
            stream.Write(scanLine.data(), scanLine.size());
        }
    }
    else
    {
        for (unsigned i = 0; i < heigth; i++)
        {
            fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                width, format, imageData + i * srcRowSize,
                (const unsigned char*)smaskData.data() + i * width);
            stream.Write(scanLine.data(), scanLine.size());
        }
    }
}

void utls::FetchImageJPEG(mm::OutputStream& stream, mm::PdfPixelFormat format,
    jpeg_decompress_struct* ctx, JSAMPARRAY jScanLine, const charbuff& smaskData, charbuff& scanLine)
{
    switch (ctx->out_color_space)
    {
        case JCS_RGB:
        {
            if (smaskData.size() == 0)
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineRGB((unsigned char*)scanLine.data(),
                        ctx->output_width, format, jScanLine[0]);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            else
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineRGB((unsigned char*)scanLine.data(), ctx->output_width, format,
                        jScanLine[0], (const unsigned char*)smaskData.data()
                        + i * ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            break;
        }
        case JCS_GRAYSCALE:
        {
            if (smaskData.size() == 0)
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineGrayScale((unsigned char*)scanLine.data(),
                        ctx->output_width, format, jScanLine[0]);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            else
            {
                for (unsigned i = 0; i < ctx->output_height; i++)
                {
                    jpeg_read_scanlines(ctx, jScanLine, 1);
                    fetchScanLineGrayScale((unsigned char*)scanLine.data(), ctx->output_width, format,
                        jScanLine[0], (const unsigned char*)smaskData.data()
                        + i * ctx->output_width);
                    stream.Write(scanLine.data(), scanLine.size());
                }
            }
            break;
        }
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

}

void fetchScanLineRGB(unsigned char* dstScanLine, unsigned width, PdfPixelFormat format,
    const unsigned char* srcScanLine)
{
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 4 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 4 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::RGB24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 3 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * 3 + 2];
            }
            break;
        }
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 3 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * 3 + 0];
            }
            break;
        }
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}

void fetchScanLineRGB(unsigned char* dstScanLine, unsigned width, PdfPixelFormat format,
    const unsigned char* srcScanLine, const unsigned char* srcAphaLine)
{
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 4 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 4 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::RGB24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 3 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * 3 + 2];
            }
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 3 + 0] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 3 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 3 + 2] = srcScanLine[i * 3 + 0];
            }
            break;
        }
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}

void fetchScanLineGrayScale(unsigned char* dstScanLine, unsigned width, PdfPixelFormat format,
    const unsigned char* srcScanLine)
{
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 4 + 0] = gray;
                dstScanLine[i * 4 + 1] = gray;
                dstScanLine[i * 4 + 2] = gray;
                dstScanLine[i * 4 + 3] = 255;
            }
            break;
        }
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 3 + 0] = gray;
                dstScanLine[i * 3 + 1] = gray;
                dstScanLine[i * 3 + 2] = gray;
            }
            break;
        }
        case PdfPixelFormat::Grayscale:
        {
            for (unsigned i = 0; i < width; i++)
                dstScanLine[i] = srcScanLine[i];
            break;
        }
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}

void fetchScanLineGrayScale(unsigned char* dstScanLine, unsigned width, PdfPixelFormat format,
    const unsigned char* srcScanLine, const unsigned char* srcAphaLine)
{
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 4 + 0] = gray;
                dstScanLine[i * 4 + 1] = gray;
                dstScanLine[i * 4 + 2] = gray;
                dstScanLine[i * 4 + 3] = srcAphaLine[i];
            }
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::RGB24:
        case PdfPixelFormat::BGR24:
        {
            for (unsigned i = 0; i < width; i++)
            {
                unsigned char gray = srcScanLine[i];
                dstScanLine[i * 3 + 0] = gray;
                dstScanLine[i * 3 + 1] = gray;
                dstScanLine[i * 3 + 2] = gray;
            }
            break;
        }
        // TODO: Handle alpha?
        case PdfPixelFormat::Grayscale:
        {
            for (unsigned i = 0; i < width; i++)
                dstScanLine[i] = srcScanLine[i];
            break;
        }
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}
