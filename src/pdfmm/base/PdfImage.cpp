/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfImage.h"

#ifdef PDFMM_HAVE_JPEG_LIB
#include <pdfmm/private/JpegCommon.h>
#endif // PDFMM_HAVE_JPEG_LIB

#ifdef PDFMM_HAVE_TIFF_LIB
extern "C" {
#include <tiffio.h>
}
#endif // PDFMM_HAVE_TIFF_LIB

#include <utfcpp/utf8.h>

#include <pdfmm/private/FileSystem.h>
#include <pdfmm/private/PdfFiltersPrivate.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfArray.h"
#include "PdfColor.h"
#include "PdfObjectStream.h"
#include "PdfStreamDevice.h"

// TIFF and JPEG headers already included through "PdfFiltersPrivate.h",
// although in opposite order (first JPEG, then TIFF), if available of course

using namespace std;
using namespace mm;

#ifdef PDFMM_HAVE_PNG_LIB
#include <png.h>
static void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length);
static void LoadFromPngContent(PdfImage& image, png_structp png, png_infop info);
#endif // PDFMM_HAVE_PNG_LIB

static void fetchScanLine(unsigned char* dstScanLine, PdfPixelFormat format,
    unsigned width, const unsigned char* srcScanLine);
static void fetchScanLine(unsigned char* dstScanLine, PdfPixelFormat format,
    unsigned width, const unsigned char* srcScanLine,
    const unsigned char* srcAphaLine);

PdfImage::PdfImage(PdfDocument& doc, const string_view& prefix)
    : PdfXObject(doc, PdfXObjectType::Image, prefix)
{
    this->SetColorSpace(PdfColorSpace::DeviceRGB);
}

void PdfImage::DecodeTo(charbuff& buffer, PdfPixelFormat format)
{
    buffer.clear();
    BufferStreamDevice stream(buffer);
    DecodeTo(stream, format);
}

// TODO: Improve performance and format support
void PdfImage::DecodeTo(OutputStream& stream, PdfPixelFormat format)
{
    auto istream = GetObject().MustGetStream().GetInputStream();
    auto& mediaFilters = istream.GetMediaFilters();
    charbuff buffer;
    ContainerStreamDevice device(buffer);
    istream.CopyTo(device);

    if (mediaFilters.size() == 0)
    {
        if (GetColorSpace() != PdfColorSpace::DeviceRGB)
            PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);

        charbuff smaskData;
        unsigned lineSize;
        getScanLineInfo(format, lineSize, smaskData);
        charbuff scanLine(lineSize);
        unsigned srcLineSize = m_Width * 3;
        if (smaskData.size() == 0)
        {
            for (unsigned i = 0; i < m_Height; i++)
            {
                fetchScanLine((unsigned char*)scanLine.data(),
                    format, m_Width, (const unsigned char*)buffer.data() + i * srcLineSize);
                stream.Write(scanLine.data(), scanLine.size());
            }
        }
        else
        {
            for (unsigned i = 0; i < m_Width; i++)
            {
                fetchScanLine((unsigned char*)scanLine.data(),
                    format, m_Width, (const unsigned char*)buffer.data() + i * srcLineSize,
                    (const unsigned char*)smaskData.data() + i * m_Width);
                stream.Write(scanLine.data(), scanLine.size());
            }
        }
    }
    else
    {
        switch (mediaFilters[0])
        {
            case PdfFilterType::DCTDecode:
            {
                string error;

                jpeg_decompress_struct jdecompress{ };
                jpeg_error_mgr jerr{ };

                // Setup variables for JPEGLib
                jdecompress.out_color_space = format == PdfPixelFormat::Grayscale ? JCS_GRAYSCALE : JCS_RGB;
                jdecompress.client_data = &error;
                jdecompress.err = jpeg_std_error(&jerr);
                jerr.error_exit = &JPegErrorExit;
                jerr.emit_message = &JPegErrorOutput;

                jpeg_create_decompress(&jdecompress);

                if (error.length() != 0)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, error);

                mm::jpeg_memory_src(&jdecompress, reinterpret_cast<JOCTET*>(buffer.data()), buffer.size());

                if (jpeg_read_header(&jdecompress, TRUE) <= 0)
                {
                    jpeg_destroy_decompress(&jdecompress);
                    PDFMM_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);
                }

                jpeg_start_decompress(&jdecompress);

                unsigned rowBytes = (unsigned)(jdecompress.output_width * jdecompress.output_components);

                // buffer will be deleted by jpeg_destroy_decompress
                JSAMPARRAY jScanLine = (*jdecompress.mem->alloc_sarray)(reinterpret_cast<j_common_ptr>(&jdecompress), JPOOL_IMAGE, rowBytes, 1);
                charbuff smaskData;
                unsigned lineSize;
                getScanLineInfo(format, lineSize, smaskData);
                charbuff scanLine(lineSize);
                if (smaskData.size() == 0)
                {
                    for (unsigned i = 0; i < jdecompress.output_height; i++)
                    {
                        jpeg_read_scanlines(&jdecompress, jScanLine, 1);
                        fetchScanLine((unsigned char*)scanLine.data(),
                            format, m_Width, jScanLine[0]);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }
                else
                {
                    for (unsigned i = 0; i < jdecompress.output_height; i++)
                    {
                        jpeg_read_scanlines(&jdecompress, jScanLine, 1);
                        fetchScanLine((unsigned char*)scanLine.data(), format,
                            m_Width, jScanLine[0], (const unsigned char*)smaskData.data()
                                + i * jdecompress.output_width);
                        stream.Write(scanLine.data(), scanLine.size());
                    }
                }

                jpeg_destroy_decompress(&jdecompress);
                break;
            }
            case PdfFilterType::CCITTFaxDecode:
            case PdfFilterType::JBIG2Decode:
            case PdfFilterType::JPXDecode:
            default:
                PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedFilter);
        }
    }
}

charbuff PdfImage::GetDecodedCopy(PdfPixelFormat format)
{
    charbuff buffer;
    DecodeTo(buffer, format);
    return buffer;
}

PdfImage::PdfImage(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::Image)
{
    m_Width = static_cast<unsigned>(this->GetDictionary().MustFindKey("Width").GetNumber());
    m_Height = static_cast<unsigned>(this->GetDictionary().MustFindKey("Height").GetNumber());
}

void PdfImage::getScanLineInfo(PdfPixelFormat format, unsigned& lineSize, charbuff& smaskData)
{
    switch (format)
    {
        case PdfPixelFormat::RGBA:
        case PdfPixelFormat::BGRA:
        {
            auto smaskObj = GetObject().GetDictionary().FindKey("SMask");
            unique_ptr<PdfImage> smask;
            if (PdfXObject::TryCreateFromObject(*smaskObj, smask))
                smask->GetObject().MustGetStream().UnwrapTo(smaskData);

            lineSize = 4 * m_Width;
            break;
        }
        case PdfPixelFormat::Grayscale:
        {
            lineSize = m_Width;
            break;
        }
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

void PdfImage::SetColorSpace(PdfColorSpace colorSpace, const PdfArray* indexedData)
{
    if (colorSpace == PdfColorSpace::Indexed)
    {
        PDFMM_RAISE_LOGIC_IF(!indexedData, "PdfImage::SetImageColorSpace: indexedData cannot be nullptr for Indexed color space.");

        PdfArray array(*indexedData);

        array.insert(array.begin(), colorSpaceToName(colorSpace));
        this->GetDictionary().AddKey("ColorSpace", array);
    }
    else
    {
        this->GetDictionary().AddKey("ColorSpace", colorSpaceToName(colorSpace));
    }
}

PdfColorSpace PdfImage::GetColorSpace() const
{
    auto colorSpace = GetDictionary().FindKey("ColorSpace");
    if (colorSpace == nullptr)
        return PdfColorSpace::Unknown;

    if (colorSpace->IsArray())
        return PdfColorSpace::Indexed;

    if (colorSpace->IsName())
        return PdfColor::GetColorSpaceForName(colorSpace->GetName());

    return PdfColorSpace::Unknown;
}

void PdfImage::SetICCProfile(InputStream& stream, unsigned colorComponents, PdfColorSpace alternateColorSpace)
{
    // Check lColorComponents for a valid value
    if (colorComponents != 1 &&
        colorComponents != 3 &&
        colorComponents != 4)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "SetImageICCProfile lColorComponents must be 1,3 or 4!");
    }

    // Create a colorspace object
    PdfObject* iccObject = this->GetDocument().GetObjects().CreateDictionaryObject();
    iccObject->GetDictionary().AddKey("Alternate", colorSpaceToName(alternateColorSpace));
    iccObject->GetDictionary().AddKey("N", static_cast<int64_t>(colorComponents));
    iccObject->GetOrCreateStream().SetData(stream);

    // Add the colorspace to our image
    PdfArray array;
    array.Add(PdfName("ICCBased"));
    array.Add(iccObject->GetIndirectReference());
    this->GetDictionary().AddKey("ColorSpace", array);
}

void PdfImage::SetSoftmask(const PdfImage& softmask)
{
    GetDictionary().AddKeyIndirect("SMask", &softmask.GetObject());
}

void PdfImage::SetData(InputStream& stream, unsigned width, unsigned height,
    unsigned bitsPerComponent)
{
    PdfFilterList filters;
    filters.push_back(PdfFilterType::FlateDecode);

    this->SetData(stream, width, height, bitsPerComponent, filters);
}

void PdfImage::SetData(InputStream& stream, unsigned width, unsigned height,
    unsigned bitsPerComponent, PdfFilterList& filters)
{
    m_Width = width;
    m_Height = height;

    this->GetDictionary().AddKey("Width", static_cast<int64_t>(width));
    this->GetDictionary().AddKey("Height", static_cast<int64_t>(height));
    this->GetDictionary().AddKey("BitsPerComponent", static_cast<int64_t>(bitsPerComponent));
    GetObject().GetOrCreateStream().SetData(stream, filters);
}

void PdfImage::SetDataRaw(InputStream& stream, unsigned width, unsigned height,
    unsigned bitsPerComponent)
{
    m_Width = width;
    m_Height = height;

    this->GetDictionary().AddKey("Width", PdfVariant(static_cast<int64_t>(width)));
    this->GetDictionary().AddKey("Height", PdfVariant(static_cast<int64_t>(height)));
    this->GetDictionary().AddKey("BitsPerComponent", PdfVariant(static_cast<int64_t>(bitsPerComponent)));
    GetObject().GetOrCreateStream().SetData(stream);
}

void PdfImage::LoadFromFile(const string_view& filename)
{
    if (filename.length() > 3)
    {
        auto extension = fs::u8path(filename).extension().u8string();
        extension = utls::ToLower(extension);

#ifdef PDFMM_HAVE_TIFF_LIB
        if (extension == ".tif" || extension == ".tiff")
        {
            LoadFromTiff(filename);
            return;
        }
#endif

#ifdef PDFMM_HAVE_JPEG_LIB
        if (extension == ".jpg" || extension == ".jpeg")
        {
            LoadFromJpeg(filename);
            return;
        }
#endif

#ifdef PDFMM_HAVE_PNG_LIB
        if (extension == ".png")
        {
            LoadFromPng(filename);
            return;
        }
#endif

    }
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, filename);
}

void PdfImage::LoadFromData(const unsigned char* data, size_t len)
{
    if (len > 4)
    {
        unsigned char magic[4];
        memcpy(magic, data, 4);

#ifdef PDFMM_HAVE_TIFF_LIB
        if ((magic[0] == 0x4D &&
            magic[1] == 0x4D &&
            magic[2] == 0x00 &&
            magic[3] == 0x2A) ||
            (magic[0] == 0x49 &&
                magic[1] == 0x49 &&
                magic[2] == 0x2A &&
                magic[3] == 0x00))
        {
            LoadFromTiffData(data, len);
            return;
        }
#endif

#ifdef PDFMM_HAVE_JPEG_LIB
        if (magic[0] == 0xFF &&
            magic[1] == 0xD8)
        {
            LoadFromJpegData(data, len);
            return;
        }
#endif

#ifdef PDFMM_HAVE_PNG_LIB
        if (magic[0] == 0x89 &&
            magic[1] == 0x50 &&
            magic[2] == 0x4E &&
            magic[3] == 0x47)
        {
            LoadFromPngData(data, len);
            return;
        }
#endif

    }
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unknown magic number");
}

#ifdef PDFMM_HAVE_JPEG_LIB

void PdfImage::LoadFromJpeg(const string_view& filename)
{
    FILE* file = utls::fopen(filename, "rb");
    try
    {
        loadFromJpegHandle(file, filename);
    }
    catch (...)
    {
        fclose(file);
        throw;
    }

    fclose(file);
}

void PdfImage::loadFromJpegHandle(FILE* inStream, const string_view& filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &JPegErrorExit;
    jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, inStream);

    if (jpeg_read_header(&cinfo, TRUE) <= 0)
    {
        jpeg_destroy_decompress(&cinfo);
        PDFMM_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);
    }

    jpeg_start_decompress(&cinfo);

    // I am not sure whether this switch is fully correct.
    // it should handle all cases though.
    // Index jpeg files might look strange as jpeglib+
    // returns 1 for them.
    switch (cinfo.output_components)
    {
        case 3:
        {
            this->SetColorSpace(PdfColorSpace::DeviceRGB);
            break;
        }
        case 4:
        {
            this->SetColorSpace(PdfColorSpace::DeviceCMYK);
            // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
            // in a inverted fashion. Fix by attaching a decode array
            PdfArray decode;
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);

            this->GetDictionary().AddKey("Decode", decode);
            break;
        }
        default:
        {
            this->SetColorSpace(PdfColorSpace::DeviceGray);
            break;
        }
    }

    // Set the filters key to DCTDecode
    this->GetObject().GetDictionary().AddKey(PdfName::KeyFilter, PdfName("DCTDecode"));

    // Do not apply any filters as JPEG data is already DCT encoded.
    FileStreamDevice input(filename);
    this->SetDataRaw(input, cinfo.output_width, cinfo.output_height, 8);
    jpeg_destroy_decompress(&cinfo);
}

void PdfImage::LoadFromJpegData(const unsigned char* data, size_t len)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &JPegErrorExit;
    jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&cinfo);

    jpeg_memory_src(&cinfo, data, len);

    if (jpeg_read_header(&cinfo, TRUE) <= 0)
    {
        jpeg_destroy_decompress(&cinfo);
        PDFMM_RAISE_ERROR(PdfErrorCode::UnexpectedEOF);
    }

    jpeg_start_decompress(&cinfo);

    // I am not sure whether this switch is fully correct.
    // it should handle all cases though.
    // Index jpeg files might look strange as jpeglib+
    // returns 1 for them.
    switch (cinfo.output_components)
    {
        case 3:
        {
            this->SetColorSpace(PdfColorSpace::DeviceRGB);
            break;
        }
        case 4:
        {
            this->SetColorSpace(PdfColorSpace::DeviceCMYK);
            // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
            // in a inverted fashion. Fix by attaching a decode array
            PdfArray decode;
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);
            decode.Add(1.0);
            decode.Add(0.0);

            this->GetDictionary().AddKey("Decode", decode);
        }
        break;
        default:
            this->SetColorSpace(PdfColorSpace::DeviceGray);
            break;
    }

    // Set the filters key to DCTDecode
    this->GetDictionary().AddKey(PdfName::KeyFilter, PdfName("DCTDecode"));

    SpanStreamDevice input((const char*)data, len);
    this->SetDataRaw(input, cinfo.output_width, cinfo.output_height, 8);

    jpeg_destroy_decompress(&cinfo);
}

#endif // PDFMM_HAVE_JPEG_LIB

#ifdef PDFMM_HAVE_TIFF_LIB

static void TIFFErrorWarningHandler(const char*, const char*, va_list)
{
}

void PdfImage::loadFromTiffHandle(void* handle)
{
    TIFF* hInTiffHandle = (TIFF*)handle;

    int32 row, width, height;
    uint16 samplesPerPixel, bitsPerSample;
    uint16* sampleInfo;
    uint16 extraSamples;
    uint16 planarConfig, photoMetric, orientation;
    int32 resolutionUnit;

    TIFFGetField(hInTiffHandle, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(hInTiffHandle, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_PLANARCONFIG, &planarConfig);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_PHOTOMETRIC, &photoMetric);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_EXTRASAMPLES, &extraSamples, &sampleInfo);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_ORIENTATION, &orientation);

    resolutionUnit = 0;
    float resX;
    float resY;
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_XRESOLUTION, &resX);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_YRESOLUTION, &resY);
    TIFFGetFieldDefaulted(hInTiffHandle, TIFFTAG_RESOLUTIONUNIT, &resolutionUnit);

    int colorChannels = samplesPerPixel - extraSamples;

    int bitsPixel = bitsPerSample * samplesPerPixel;

    // TODO: implement special cases
    if (TIFFIsTiled(hInTiffHandle))
    {
        TIFFClose(hInTiffHandle);
        PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
    }

    if (planarConfig != PLANARCONFIG_CONTIG && colorChannels != 1)
    {
        TIFFClose(hInTiffHandle);
        PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
    }

    if (orientation != ORIENTATION_TOPLEFT)
    {
        TIFFClose(hInTiffHandle);
        PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
    }

    switch (photoMetric)
    {
        case PHOTOMETRIC_MINISBLACK:
        {
            if (bitsPixel == 1)
            {
                PdfArray decode;
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(0)));
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(1)));
                this->GetDictionary().AddKey("Decode", decode);
                this->GetDictionary().AddKey("ImageMask", true);
                this->GetDictionary().RemoveKey("ColorSpace");
            }
            else if (bitsPixel == 8 || bitsPixel == 16)
                SetColorSpace(PdfColorSpace::DeviceGray);
            else
            {
                TIFFClose(hInTiffHandle);
                PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
        }
        break;

        case PHOTOMETRIC_MINISWHITE:
        {
            if (bitsPixel == 1)
            {
                PdfArray decode;
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(1)));
                decode.insert(decode.end(), PdfObject(static_cast<int64_t>(0)));
                this->GetDictionary().AddKey("Decode", decode);
                this->GetDictionary().AddKey("ImageMask", true);
                this->GetDictionary().RemoveKey("ColorSpace");
            }
            else if (bitsPixel == 8 || bitsPixel == 16)
                SetColorSpace(PdfColorSpace::DeviceGray);
            else
            {
                TIFFClose(hInTiffHandle);
                PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
        }
        break;

        case PHOTOMETRIC_RGB:
            if (bitsPixel != 24)
            {
                TIFFClose(hInTiffHandle);
                PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
            SetColorSpace(PdfColorSpace::DeviceRGB);
            break;

        case PHOTOMETRIC_SEPARATED:
            if (bitsPixel != 32)
            {
                TIFFClose(hInTiffHandle);
                PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            }
            SetColorSpace(PdfColorSpace::DeviceCMYK);
            break;

        case PHOTOMETRIC_PALETTE:
        {
            unsigned numColors = (1 << bitsPixel);

            PdfArray decode;
            decode.insert(decode.end(), PdfObject(static_cast<int64_t>(0)));
            decode.insert(decode.end(), PdfObject(static_cast<int64_t>(numColors) - 1));
            this->GetDictionary().AddKey("Decode", decode);

            uint16* rgbRed;
            uint16* rgbGreen;
            uint16* rgbBlue;
            TIFFGetField(hInTiffHandle, TIFFTAG_COLORMAP, &rgbRed, &rgbGreen, &rgbBlue);

            charbuff data(numColors * 3);

            for (unsigned clr = 0; clr < numColors; clr++)
            {
                data[3 * clr + 0] = rgbRed[clr] / 257;
                data[3 * clr + 1] = rgbGreen[clr] / 257;
                data[3 * clr + 2] = rgbBlue[clr] / 257;
            }

            // Create a colorspace object
            PdfObject* pIdxObject = this->GetDocument().GetObjects().CreateDictionaryObject();
            pIdxObject->GetOrCreateStream().SetData(data);

            // Add the colorspace to our image
            PdfArray array;
            array.Add(PdfName("Indexed"));
            array.Add(PdfName("DeviceRGB"));
            array.Add(static_cast<int64_t>(numColors) - 1);
            array.Add(pIdxObject->GetIndirectReference());
            this->GetDictionary().AddKey(PdfName("ColorSpace"), array);
        }
        break;

        default:
            TIFFClose(hInTiffHandle);
            PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
            break;
    }

    size_t scanlineSize = TIFFScanlineSize(hInTiffHandle);
    size_t bufferSize = scanlineSize * height;
    charbuff buffer(bufferSize);
    for (row = 0; row < height; row++)
    {
        if (TIFFReadScanline(hInTiffHandle,
            &buffer[row * scanlineSize],
            row) == (-1))
        {
            TIFFClose(hInTiffHandle);
            PDFMM_RAISE_ERROR(PdfErrorCode::UnsupportedImageFormat);
        }
    }

    SpanStreamDevice input(buffer);
    SetData(input, static_cast<unsigned>(width),
        static_cast<unsigned>(height),
        static_cast<unsigned>(bitsPerSample));
}

void PdfImage::LoadFromTiff(const string_view& filename)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);

    if (filename.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    TIFF* hInfile = TIFFOpenW((wchar_t*)filename16.c_str(), "rb");
#else
    TIFF* hInfile = TIFFOpen(filename.data(), "rb");
#endif

    if (hInfile == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, filename);

    try
    {
        loadFromTiffHandle(hInfile);
    }
    catch (...)
    {
        TIFFClose(hInfile);
        throw;
    }

    TIFFClose(hInfile);
}

struct TiffData
{
    TiffData(const unsigned char* data, tsize_t size) :m_data(data), m_pos(0), m_size(size) {}

    tsize_t read(tdata_t data, tsize_t length)
    {
        tsize_t bytesRead = 0;
        if (length > m_size - static_cast<tsize_t>(m_pos))
        {
            memcpy(data, &m_data[m_pos], m_size - (tsize_t)m_pos);
            bytesRead = m_size - (tsize_t)m_pos;
            m_pos = m_size;
        }
        else
        {
            memcpy(data, &m_data[m_pos], length);
            bytesRead = length;
            m_pos += length;
        }
        return bytesRead;
    }

    toff_t size()
    {
        return m_size;
    }

    toff_t seek(toff_t pos, int whence)
    {
        if (pos == 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        switch (whence)
        {
            case SEEK_SET:
                if (static_cast<tsize_t>(pos) > m_size)
                {
                    m_pos = m_size;
                }
                else
                {
                    m_pos = pos;
                }
                break;
            case SEEK_CUR:
                if (static_cast<tsize_t>(pos + m_pos) > m_size)
                {
                    m_pos = m_size;
                }
                else
                {
                    m_pos += pos;
                }
                break;
            case SEEK_END:
                if (static_cast<tsize_t>(pos) > m_size)
                {
                    m_pos = 0;
                }
                else
                {
                    m_pos = m_size - pos;
                }
                break;
        }
        return m_pos;
    }

private:
    const unsigned char* m_data;
    toff_t m_pos;
    tsize_t m_size;
};
tsize_t tiff_Read(thandle_t st, tdata_t buffer, tsize_t size)
{
    TiffData* data = (TiffData*)st;
    return data->read(buffer, size);
};
tsize_t tiff_Write(thandle_t st, tdata_t buffer, tsize_t size)
{
    (void)st;
    (void)buffer;
    (void)size;
    return 0;
};
int tiff_Close(thandle_t)
{
    return 0;
};
toff_t tiff_Seek(thandle_t st, toff_t pos, int whence)
{
    TiffData* data = (TiffData*)st;
    return data->seek(pos, whence);
};
toff_t tiff_Size(thandle_t st)
{
    TiffData* data = (TiffData*)st;
    return data->size();
};
int tiff_Map(thandle_t, tdata_t*, toff_t*)
{
    return 0;
};
void tiff_Unmap(thandle_t, tdata_t, toff_t)
{
    return;
};
void PdfImage::LoadFromTiffData(const unsigned char* data, size_t len)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);

    if (data == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    TiffData tiffData(data, (tsize_t)len);
    TIFF* hInHandle = TIFFClientOpen("Memory", "r", (thandle_t)&tiffData,
        tiff_Read, tiff_Write, tiff_Seek, tiff_Close, tiff_Size,
        tiff_Map, tiff_Unmap);
    if (hInHandle == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    loadFromTiffHandle(hInHandle);
}

#endif // PDFMM_HAVE_TIFF_LIB

#ifdef PDFMM_HAVE_PNG_LIB

void PdfImage::LoadFromPng(const std::string_view& filename)
{
    FILE* file = utls::fopen(filename, "rb");

    try
    {
        loadFromPngHandle(file);
    }
    catch (...)
    {
        fclose(file);
        throw;
    }

    fclose(file);
}

void PdfImage::loadFromPngHandle(FILE* stream)
{
    png_byte header[8];
    if (fread(header, 1, 8, stream) != 8 ||
        png_sig_cmp(header, 0, 8))
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "The file could not be recognized as a PNG file");
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    png_infop info = png_create_info_struct(png);
    if (info == nullptr)
    {
        png_destroy_read_struct(&png, (png_infopp)nullptr, (png_infopp)nullptr);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, (png_infopp)nullptr);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    png_init_io(png, stream);
    LoadFromPngContent(*this, png, info);
}

struct PngData
{
    PngData(const unsigned char* data, png_size_t size) :
        m_data(data), m_pos(0), m_size(size) {}

    void read(png_bytep data, png_size_t length)
    {
        if (length > m_size - m_pos)
        {
            memcpy(data, &m_data[m_pos], m_size - m_pos);
            m_pos = m_size;
        }
        else
        {
            memcpy(data, &m_data[m_pos], length);
            m_pos += length;
        }
    }

private:
    const unsigned char* m_data;
    png_size_t m_pos;
    png_size_t m_size;
};

void PdfImage::LoadFromPngData(const unsigned char* data, size_t len)
{
    if (data == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PngData pngData(data, len);
    png_byte header[8];
    pngData.read(header, 8);
    if (png_sig_cmp(header, 0, 8))
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "The file could not be recognized as a PNG file");
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    png_infop info = png_create_info_struct(png);
    if (info == nullptr)
    {
        png_destroy_read_struct(&png, (png_infopp)nullptr, (png_infopp)nullptr);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_read_struct(&png, &info, (png_infopp)nullptr);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    png_set_read_fn(png, (png_voidp)&pngData, pngReadData);
    LoadFromPngContent(*this, png, info);
}

void LoadFromPngContent(PdfImage& image, png_structp png, png_infop info)
{
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    // Begin
    png_uint_32 width;
    png_uint_32 height;
    int depth;
    int color_type;
    int interlace;

    png_get_IHDR(png, info,
        &width, &height, &depth,
        &color_type, &interlace, NULL, NULL);

    // convert palette/gray image to rgb
    // expand gray bit depth if needed
    if (color_type == PNG_COLOR_TYPE_GRAY)
    {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8(png);
#else
        png_set_gray_1_2_4_to_8(pPng);
#endif
    }
    else if (color_type != PNG_COLOR_TYPE_PALETTE && depth < 8)
    {
        png_set_packing(png);
    }

    // transform transparency to alpha
    if (color_type != PNG_COLOR_TYPE_PALETTE && png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (depth == 16)
        png_set_strip_16(png);

    if (interlace != PNG_INTERLACE_NONE)
        png_set_interlace_handling(png);

    //png_set_filler (pPng, 0xff, PNG_FILLER_AFTER);

    // recheck header after setting EXPAND options
    png_read_update_info(png, info);
    png_get_IHDR(png, info,
        &width, &height, &depth,
        &color_type, &interlace, NULL, NULL);
    // End

    // Read the file
    if (setjmp(png_jmpbuf(png)) != 0)
    {
        png_destroy_read_struct(&png, &info, (png_infopp)NULL);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    size_t rowLen = png_get_rowbytes(png, info);
    size_t len = rowLen * height;
    charbuff buffer(len);

    unique_ptr<png_bytep[]> rows(new png_bytep[height]);
    for (unsigned int y = 0; y < height; y++)
    {
        rows[y] = reinterpret_cast<png_bytep>(buffer.data() + y * rowLen);
    }

    png_read_image(png, rows.get());

    png_bytep paletteTrans;
    int numTransColors;
    if (color_type & PNG_COLOR_MASK_ALPHA
        || (color_type == PNG_COLOR_TYPE_PALETTE
            && png_get_valid(png, info, PNG_INFO_tRNS)
            && png_get_tRNS(png, info, &paletteTrans, &numTransColors, NULL)))
    {
        // Handle alpha channel and create smask
        charbuff smask(width * height);
        png_uint_32 smaskIndex = 0;
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    png_byte color;
                    switch (depth)
                    {
                        case 8:
                            color = row[c];
                            break;
                        case 4:
                            color = c % 2 ? row[c / 2] >> 4 : row[c / 2] & 0xF;
                            break;
                        case 2:
                            color = (row[c / 4] >> c % 4 * 2) & 3;
                            break;
                        case 1:
                            color = (row[c / 4] >> c % 8) & 1;
                            break;
                        default:
                            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
                    }

                    smask[smaskIndex++] = color < numTransColors ? paletteTrans[color] : 0xFF;
                }
            }
        }
        else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    memmove(buffer.data() + 3 * smaskIndex, row + 4 * c, 3); // 3 byte for rgb
                    smask[smaskIndex++] = row[c * 4 + 3]; // 4th byte for alpha
                }
            }
            len = 3 * width * height;
        }
        else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    buffer[smaskIndex] = row[c * 2]; // 1 byte for gray
                    smask[smaskIndex++] = row[c * 2 + 1]; // 2nd byte for alpha
                }
            }
            len = width * height;
        }
        SpanStreamDevice smaskinput(smask);
        PdfImage smakeImage(image.GetDocument());
        smakeImage.SetColorSpace(PdfColorSpace::DeviceGray);
        smakeImage.SetData(smaskinput, width, height, 8);
        image.SetSoftmask(smakeImage);
    }

    // Set color space
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_color* colors;
        int colorCount;
        png_get_PLTE(png, info, &colors, &colorCount);

        charbuff data(colorCount * 3);
        for (int i = 0; i < colorCount; i++, colors++)
        {
            data[3 * i + 0] = colors->red;
            data[3 * i + 1] = colors->green;
            data[3 * i + 2] = colors->blue;
        }
        PdfObject* pIdxObject = image.GetDocument().GetObjects().CreateDictionaryObject();
        pIdxObject->GetOrCreateStream().SetData(data);

        PdfArray array;
        array.Add(PdfName("DeviceRGB"));
        array.Add(static_cast<int64_t>(colorCount - 1));
        array.Add(pIdxObject->GetIndirectReference());
        image.SetColorSpace(PdfColorSpace::Indexed, &array);
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        image.SetColorSpace(PdfColorSpace::DeviceGray);
    }
    else
    {
        image.SetColorSpace(PdfColorSpace::DeviceRGB);
    }

    // Set the image data and flate compress it
    SpanStreamDevice input(buffer);
    image.SetData(input, width, height, depth);

    png_destroy_read_struct(&png, &info, (png_infopp)NULL);
}

void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
    PngData* a = (PngData*)png_get_io_ptr(pngPtr);
    a->read(data, length);
}

#endif // PDFMM_HAVE_PNG_LIB

PdfName PdfImage::colorSpaceToName(PdfColorSpace colorSpace)
{
    return PdfColor::GetNameForColorSpace(colorSpace);
}

void PdfImage::SetChromaKeyMask(int64_t r, int64_t g, int64_t b, int64_t threshold)
{
    PdfArray array;
    array.Add(r - threshold);
    array.Add(r + threshold);
    array.Add(g - threshold);
    array.Add(g + threshold);
    array.Add(b - threshold);
    array.Add(b + threshold);

    this->GetDictionary().AddKey("Mask", array);
}

void PdfImage::SetInterpolate(bool value)
{
    this->GetDictionary().AddKey("Interpolate", value);
}

PdfRect PdfImage::GetRect() const
{
    return PdfRect(0, 0, m_Width, m_Height);
}

unsigned PdfImage::GetWidth() const
{
    return m_Width;
}

unsigned PdfImage::GetHeight() const
{
    return m_Height;
}

void fetchScanLine(unsigned char* dstScanLine, PdfPixelFormat format, unsigned width,
    const unsigned char* srcScanLine)
{
    switch (format)
    {
        case PdfPixelFormat::BGRA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 4 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 4 + 3] = 1;
            }
            break;
        }
        case PdfPixelFormat::RGBA:
        {
            for (unsigned i = 0; i < width; i++)
            {
                dstScanLine[i * 4 + 0] = srcScanLine[i * 3 + 0];
                dstScanLine[i * 4 + 1] = srcScanLine[i * 3 + 1];
                dstScanLine[i * 4 + 2] = srcScanLine[i * 3 + 2];
                dstScanLine[i * 4 + 3] = 1;
            }
            break;
        }
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}

void fetchScanLine(unsigned char* dstScanLine, PdfPixelFormat format, unsigned width,
    const unsigned char* srcScanLine, const unsigned char* srcAphaLine)
{
    switch (format)
    {
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
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedImageFormat, "Unsupported pixel format");
    }
}
