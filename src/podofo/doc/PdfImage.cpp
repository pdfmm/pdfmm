/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfImage.h"

#include <utfcpp/utf8.h>
#include <sstream>

#include <doc/PdfDocument.h>
#include "base/PdfDictionary.h"
#include "base/PdfArray.h"
#include "base/PdfColor.h"
#include "base/PdfStream.h"
#include "base/PdfFiltersPrivate.h"

// TIFF and JPEG headers already included through "base/PdfFiltersPrivate.h",
// although in opposite order (first JPEG, then TIFF), if available of course

using namespace std;
using namespace PoDoFo;

#ifdef PODOFO_HAVE_PNG_LIB
#include <png.h>
static void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length);
static void LoadFromPngContent(PdfImage& image, png_structp pPng, png_infop pInfo);
#endif // PODOFO_HAVE_PNG_LIB

PdfImage::PdfImage(PdfDocument& doc, const string_view& prefix)
    : PdfXObject(doc, PdfXObjectType::Image, prefix)
{
    this->SetImageColorSpace(PdfColorSpace::DeviceRGB);
}

PdfImage::PdfImage(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::Image)
{
    m_width = static_cast<unsigned>(this->GetObject().GetDictionary().MustFindKey("Width").GetNumber());
    m_height = static_cast<unsigned>(this->GetObject().GetDictionary().MustFindKey("Height").GetNumber());
}

void PdfImage::SetImageColorSpace(PdfColorSpace eColorSpace, const PdfArray* indexedData)
{
    if (eColorSpace == PdfColorSpace::Indexed)
    {
        PODOFO_RAISE_LOGIC_IF(!indexedData, "PdfImage::SetImageColorSpace: indexedData cannot be nullptr for Indexed color space.");

        PdfArray array(*indexedData);

        array.insert(array.begin(), ColorspaceToName(eColorSpace));
        this->GetObject().GetDictionary().AddKey(PdfName("ColorSpace"), array);
    }
    else
    {
        this->GetObject().GetDictionary().AddKey(PdfName("ColorSpace"), ColorspaceToName(eColorSpace));
    }
}

PdfColorSpace PdfImage::GetImageColorSpace()
{
    PdfObject* colorSpace = GetObject().GetDictionary().FindKey("ColorSpace");
    if (colorSpace == nullptr)
        return PdfColorSpace::Unknown;

    if (colorSpace->IsArray())
        return PdfColorSpace::Indexed;

    if (colorSpace->IsName())
        return PdfColor::GetColorSpaceForName(colorSpace->GetName());

    return PdfColorSpace::Unknown;
}

void PdfImage::SetImageICCProfile(PdfInputStream& stream, unsigned lColorComponents, PdfColorSpace eAlternateColorSpace)
{
    // Check lColorComponents for a valid value
    if (lColorComponents != 1 &&
        lColorComponents != 3 &&
        lColorComponents != 4)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "SetImageICCProfile lColorComponents must be 1,3 or 4!");
    }

    // Create a colorspace object
    PdfObject* pIccObject = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
    pIccObject->GetDictionary().AddKey(PdfName("Alternate"), ColorspaceToName(eAlternateColorSpace));
    pIccObject->GetDictionary().AddKey(PdfName("N"), static_cast<int64_t>(lColorComponents));
    pIccObject->GetOrCreateStream().Set(stream);

    // Add the colorspace to our image
    PdfArray array;
    array.push_back(PdfName("ICCBased"));
    array.push_back(pIccObject->GetIndirectReference());
    this->GetObject().GetDictionary().AddKey(PdfName("ColorSpace"), array);
}

void PdfImage::SetImageSoftmask(const PdfImage& softmask)
{
    GetObject().GetDictionary().AddKey("SMask", softmask.GetObject().GetIndirectReference());
}

void PdfImage::SetImageData(PdfInputStream& stream, unsigned nWidth, unsigned nHeight,
                             unsigned nBitsPerComponent, bool writeRect)
{
    TVecFilters vecFlate;
    vecFlate.push_back(PdfFilterType::FlateDecode);

    this->SetImageData(stream, nWidth, nHeight, nBitsPerComponent, vecFlate, writeRect);
}

void PdfImage::SetImageData(PdfInputStream& stream, unsigned nWidth, unsigned nHeight,
    unsigned nBitsPerComponent, TVecFilters& vecFilters, bool writeRect)
{
    m_width = nWidth;
    m_height = nHeight;

    if (writeRect)
        SetRect(PdfRect(0, 0, nWidth, nHeight));

    this->GetObject().GetDictionary().AddKey("Width", PdfVariant(static_cast<int64_t>(nWidth)));
    this->GetObject().GetDictionary().AddKey("Height", PdfVariant(static_cast<int64_t>(nHeight)));
    this->GetObject().GetDictionary().AddKey("BitsPerComponent", PdfVariant(static_cast<int64_t>(nBitsPerComponent)));
    this->GetObject().GetOrCreateStream().Set(stream, vecFilters);
}

void PdfImage::SetImageDataRaw(PdfInputStream& pStream, unsigned nWidth, unsigned nHeight,
                                unsigned nBitsPerComponent)
{
    m_width = nWidth;
    m_height = nHeight;

    this->GetObject().GetDictionary().AddKey("Width", PdfVariant(static_cast<int64_t>(nWidth)));
    this->GetObject().GetDictionary().AddKey("Height", PdfVariant(static_cast<int64_t>(nHeight)));
    this->GetObject().GetDictionary().AddKey("BitsPerComponent", PdfVariant(static_cast<int64_t>(nBitsPerComponent)));
    this->GetObject().GetOrCreateStream().SetRawData(pStream, -1);
}

void PdfImage::LoadFromFile(const string_view& filename)
{
    if (filename.length() > 3)
    {
        const char* pszExtension = filename.data() + filename.length() - 3;

#ifdef PODOFO_HAVE_TIFF_LIB
        if (PoDoFo::compat::strncasecmp(pszExtension, "tif", 3) == 0 ||
            PoDoFo::compat::strncasecmp(pszExtension, "iff", 3) == 0) // "tiff"
        {
            LoadFromTiff(filename);
            return;
        }
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
        if (PoDoFo::compat::strncasecmp(pszExtension, "jpg", 3) == 0)
        {
            LoadFromJpeg(filename);
            return;
        }
#endif

#ifdef PODOFO_HAVE_PNG_LIB
        if (PoDoFo::compat::strncasecmp(pszExtension, "png", 3) == 0)
        {
            LoadFromPng(filename);
            return;
        }
#endif

    }
    PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedImageFormat, filename.data());
}

void PdfImage::LoadFromData(const unsigned char* pData, size_t dwLen)
{
    if (dwLen > 4)
    {
        unsigned char magic[4];
        memcpy(magic, pData, 4);

#ifdef PODOFO_HAVE_TIFF_LIB
        if ((magic[0] == 0x4D &&
            magic[1] == 0x4D &&
            magic[2] == 0x00 &&
            magic[3] == 0x2A) ||
            (magic[0] == 0x49 &&
                magic[1] == 0x49 &&
                magic[2] == 0x2A &&
                magic[3] == 0x00))
        {
            LoadFromTiffData(pData, dwLen);
            return;
        }
#endif

#ifdef PODOFO_HAVE_JPEG_LIB
        if (magic[0] == 0xFF &&
            magic[1] == 0xD8)
        {
            LoadFromJpegData(pData, dwLen);
            return;
        }
#endif

#ifdef PODOFO_HAVE_PNG_LIB
        if (magic[0] == 0x89 &&
            magic[1] == 0x50 &&
            magic[2] == 0x4E &&
            magic[3] == 0x47)
        {
            LoadFromPngData(pData, dwLen);
            return;
        }
#endif

    }
    PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedImageFormat, "Unknown magic number");
}

#ifdef PODOFO_HAVE_JPEG_LIB

void PdfImage::LoadFromJpeg(const string_view& filename)
{
    FILE* file = io::fopen(filename, "rb");
    try
    {
        LoadFromJpegHandle(file, filename);
    }
    catch (...)
    {
        fclose(file);
        throw;
    }

    fclose(file);
}

void PdfImage::LoadFromJpegHandle(FILE* pInStream, const string_view& filename)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &JPegErrorExit;
    jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, pInStream);

    if (jpeg_read_header(&cinfo, TRUE) <= 0)
    {
        jpeg_destroy_decompress(&cinfo);
        PODOFO_RAISE_ERROR(EPdfError::UnexpectedEOF);
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
            this->SetImageColorSpace(PdfColorSpace::DeviceRGB);
            break;
        }
        case 4:
        {
            this->SetImageColorSpace(PdfColorSpace::DeviceCMYK);
            // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
            // in a inverted fashion. Fix by attaching a decode array
            PdfArray decode;
            decode.push_back(1.0);
            decode.push_back(0.0);
            decode.push_back(1.0);
            decode.push_back(0.0);
            decode.push_back(1.0);
            decode.push_back(0.0);
            decode.push_back(1.0);
            decode.push_back(0.0);

            this->GetObject().GetDictionary().AddKey(PdfName("Decode"), decode);
            break;
        }
        default:
        {
            this->SetImageColorSpace(PdfColorSpace::DeviceGray);
            break;
        }
    }

    // Set the filters key to DCTDecode
    this->GetObject().GetDictionary().AddKey(PdfName::KeyFilter, PdfName("DCTDecode"));

    // Do not apply any filters as JPEG data is already DCT encoded.
    PdfFileInputStream stream(filename);
    this->SetImageDataRaw(stream, cinfo.output_width, cinfo.output_height, 8);
    jpeg_destroy_decompress(&cinfo);
}

void PdfImage::LoadFromJpegData(const unsigned char* pData, size_t dwLen)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = &JPegErrorExit;
    jerr.emit_message = &JPegErrorOutput;

    jpeg_create_decompress(&cinfo);

    jpeg_memory_src(&cinfo, pData, dwLen);

    if (jpeg_read_header(&cinfo, TRUE) <= 0)
    {
        jpeg_destroy_decompress(&cinfo);
        PODOFO_RAISE_ERROR(EPdfError::UnexpectedEOF);
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
            this->SetImageColorSpace(PdfColorSpace::DeviceRGB);
            break;
        }
        case 4:
        {
            this->SetImageColorSpace(PdfColorSpace::DeviceCMYK);
            // The jpeg-doc ist not specific in this point, but cmyk's seem to be stored
            // in a inverted fashion. Fix by attaching a decode array
            PdfArray decode;
            decode.push_back(1.0);
            decode.push_back(0.0);
            decode.push_back(1.0);
            decode.push_back(0.0);
            decode.push_back(1.0);
            decode.push_back(0.0);
            decode.push_back(1.0);
            decode.push_back(0.0);

            this->GetObject().GetDictionary().AddKey(PdfName("Decode"), decode);
        }
        break;
        default:
            this->SetImageColorSpace(PdfColorSpace::DeviceGray);
            break;
    }

    // Set the filters key to DCTDecode
    this->GetObject().GetDictionary().AddKey(PdfName::KeyFilter, PdfName("DCTDecode"));

    PdfMemoryInputStream fInpStream((const char*)pData, dwLen);
    this->SetImageDataRaw(fInpStream, cinfo.output_width, cinfo.output_height, 8);

    jpeg_destroy_decompress(&cinfo);
}

#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB

static void TIFFErrorWarningHandler(const char*, const char*, va_list)
{
    
}

void PdfImage::LoadFromTiffHandle(void* hInHandle)
{

    TIFF* hInTiffHandle = (TIFF*)hInHandle;

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
        PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
    }

    if (planarConfig != PLANARCONFIG_CONTIG && colorChannels != 1)
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
    }

    if (orientation != ORIENTATION_TOPLEFT)
    {
        TIFFClose(hInTiffHandle);
        PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
    }

    switch (photoMetric)
    {
        case PHOTOMETRIC_MINISBLACK:
        {
            if (bitsPixel == 1)
            {
                PdfArray decode;
                decode.insert(decode.end(), PdfVariant(static_cast<int64_t>(0)));
                decode.insert(decode.end(), PdfVariant(static_cast<int64_t>(1)));
                this->GetObject().GetDictionary().AddKey(PdfName("Decode"), decode);
                this->GetObject().GetDictionary().AddKey(PdfName("ImageMask"), PdfVariant(true));
                this->GetObject().GetDictionary().RemoveKey(PdfName("ColorSpace"));
            }
            else if (bitsPixel == 8 || bitsPixel == 16)
                SetImageColorSpace(PdfColorSpace::DeviceGray);
            else
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
            }
        }
        break;

        case PHOTOMETRIC_MINISWHITE:
        {
            if (bitsPixel == 1)
            {
                PdfArray decode;
                decode.insert(decode.end(), PdfVariant(static_cast<int64_t>(1)));
                decode.insert(decode.end(), PdfVariant(static_cast<int64_t>(0)));
                this->GetObject().GetDictionary().AddKey(PdfName("Decode"), decode);
                this->GetObject().GetDictionary().AddKey(PdfName("ImageMask"), PdfVariant(true));
                this->GetObject().GetDictionary().RemoveKey(PdfName("ColorSpace"));
            }
            else if (bitsPixel == 8 || bitsPixel == 16)
                SetImageColorSpace(PdfColorSpace::DeviceGray);
            else
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
            }
        }
        break;

        case PHOTOMETRIC_RGB:
            if (bitsPixel != 24)
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
            }
            SetImageColorSpace(PdfColorSpace::DeviceRGB);
            break;

        case PHOTOMETRIC_SEPARATED:
            if (bitsPixel != 32)
            {
                TIFFClose(hInTiffHandle);
                PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
            }
            SetImageColorSpace(PdfColorSpace::DeviceCMYK);
            break;

        case PHOTOMETRIC_PALETTE:
        {
            int numColors = (1 << bitsPixel);

            PdfArray decode;
            decode.insert(decode.end(), PdfVariant(static_cast<int64_t>(0)));
            decode.insert(decode.end(), PdfVariant(static_cast<int64_t>(numColors) - 1));
            this->GetObject().GetDictionary().AddKey(PdfName("Decode"), decode);

            uint16* rgbRed;
            uint16* rgbGreen;
            uint16* rgbBlue;
            TIFFGetField(hInTiffHandle, TIFFTAG_COLORMAP, &rgbRed, &rgbGreen, &rgbBlue);

            char* datap = new char[numColors * 3];

            for (int clr = 0; clr < numColors; clr++)
            {
                datap[3 * clr + 0] = rgbRed[clr] / 257;
                datap[3 * clr + 1] = rgbGreen[clr] / 257;
                datap[3 * clr + 2] = rgbBlue[clr] / 257;
            }
            PdfMemoryInputStream stream(datap, numColors * 3);

            // Create a colorspace object
            PdfObject* pIdxObject = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
            pIdxObject->GetOrCreateStream().Set(stream);

            // Add the colorspace to our image
            PdfArray array;
            array.push_back(PdfName("Indexed"));
            array.push_back(PdfName("DeviceRGB"));
            array.push_back(static_cast<int64_t>(numColors) - 1);
            array.push_back(pIdxObject->GetIndirectReference());
            this->GetObject().GetDictionary().AddKey(PdfName("ColorSpace"), array);

            delete[] datap;
        }
        break;

        default:
            TIFFClose(hInTiffHandle);
            PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
            break;
    }

    size_t scanlineSize = TIFFScanlineSize(hInTiffHandle);
    size_t bufferSize = scanlineSize * height;
    buffer_t buffer(bufferSize);
    for (row = 0; row < height; row++)
    {
        if (TIFFReadScanline(hInTiffHandle,
            &buffer[row * scanlineSize],
            row) == (-1))
        {
            TIFFClose(hInTiffHandle);
            PODOFO_RAISE_ERROR(EPdfError::UnsupportedImageFormat);
        }
    }

    PdfMemoryInputStream stream(buffer.data(), bufferSize);

    SetImageData(stream, static_cast<unsigned>(width),
        static_cast<unsigned>(height),
        static_cast<unsigned>(bitsPerSample));
}
 
void PdfImage::LoadFromTiff(const string_view& filename)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);

    if (filename.length() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    TIFF* hInfile = TIFFOpenW((wchar_t*)filename16.c_str(), "rb");
#else
    TIFF* hInfile = TIFFOpen(filename.data(), "rb");
#endif

    if (!hInfile)
        PODOFO_RAISE_ERROR_INFO(EPdfError::FileNotFound, filename.data());

    try
    {
        LoadFromTiffHandle(hInfile);
    }
    catch (...)
    {
        TIFFClose(hInfile);
        throw;
    }

    TIFFClose(hInfile);
}

struct tiffData
{
    tiffData(const unsigned char* data, tsize_t size):_data(data), _pos(0), _size(size) {}
    
    tsize_t read(tdata_t data, tsize_t length)
    {
        tsize_t bytesRead = 0;
        if (length > _size - static_cast<tsize_t>(_pos))
        {
            memcpy(data, &_data[_pos], _size - (tsize_t)_pos);
            bytesRead = _size - (tsize_t)_pos;
            _pos = _size;
        }
        else
        {
            memcpy(data, &_data[_pos], length);
            bytesRead = length;
            _pos += length;
        }
        return bytesRead;
    }
    
    toff_t size()
    {
        return _size;
    }
    
    toff_t seek(toff_t pos, int whence)
    {
        if (pos == 0xFFFFFFFF) {
            return 0xFFFFFFFF;
        }
        switch(whence)
        {
            case SEEK_SET:
                if (static_cast<tsize_t>(pos) > _size)
                {
                    _pos = _size;
                }
                else
                {
                    _pos = pos;
                }
                break;
            case SEEK_CUR:
                if (static_cast<tsize_t>(pos + _pos) > _size)
                {
                    _pos = _size;
                }
                else
                {
                    _pos += pos;
                }
                break;
            case SEEK_END:
                if (static_cast<tsize_t>(pos) > _size)
                {
                    _pos = 0;
                }
                else
                {
                    _pos = _size - pos;
                }
                break;
        }
        return _pos;
    }
    
private:
    const unsigned char* _data;
    toff_t _pos;
    tsize_t _size;
};
tsize_t tiff_Read(thandle_t st, tdata_t buffer, tsize_t size)
{
    tiffData* data = (tiffData*)st;
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
    tiffData* data = (tiffData*)st;
    return data->seek(pos, whence);
};
toff_t tiff_Size(thandle_t st)
{
    tiffData* data = (tiffData*)st;
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
void PdfImage::LoadFromTiffData(const unsigned char* pData, size_t dwLen)
{
    TIFFSetErrorHandler(TIFFErrorWarningHandler);
    TIFFSetWarningHandler(TIFFErrorWarningHandler);

    if (pData == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    tiffData data(pData, (tsize_t)dwLen);
    TIFF* hInHandle = TIFFClientOpen("Memory", "r", (thandle_t)&data,
        tiff_Read, tiff_Write, tiff_Seek, tiff_Close, tiff_Size,
        tiff_Map, tiff_Unmap);
    if (hInHandle == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    LoadFromTiffHandle(hInHandle);
}

#endif // PODOFO_HAVE_TIFF_LIB

#ifdef PODOFO_HAVE_PNG_LIB

void PdfImage::LoadFromPng(const std::string_view& filename)
{
    FILE* file = io::fopen(filename, "rb");

    try
    {
        LoadFromPngHandle(file);
    }
    catch (...)
    {
        fclose(file);
        throw;
    }

    fclose(file);
}

void PdfImage::LoadFromPngHandle(FILE* hFile)
{
    png_byte header[8];
    if (fread(header, 1, 8, hFile) != 8 ||
        png_sig_cmp(header, 0, 8))
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedImageFormat, "The file could not be recognized as a PNG file.");
    }

    png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (pPng == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    png_infop pInfo = png_create_info_struct(pPng);
    if (pInfo == nullptr)
    {
        png_destroy_read_struct(&pPng, (png_infopp)nullptr, (png_infopp)nullptr);
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    if (setjmp(png_jmpbuf(pPng)))
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)nullptr);
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    png_init_io(pPng, hFile);
    LoadFromPngContent(*this, pPng, pInfo);
}

struct pngData
{
    pngData(const unsigned char* data, png_size_t size) :
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

void PdfImage::LoadFromPngData(const unsigned char* pData, size_t dwLen)
{
    if (pData == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    pngData data(pData, dwLen);
    png_byte header[8];
    data.read(header, 8);
    if (png_sig_cmp(header, 0, 8))
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnsupportedImageFormat, "The file could not be recognized as a PNG file.");
    }

    png_structp pPng = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (pPng == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    png_infop pInfo = png_create_info_struct(pPng);
    if (pInfo == nullptr)
    {
        png_destroy_read_struct(&pPng, (png_infopp)nullptr, (png_infopp)nullptr);
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    if (setjmp(png_jmpbuf(pPng)))
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)nullptr);
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    png_set_read_fn(pPng, (png_voidp)&data, pngReadData);
    LoadFromPngContent(*this, pPng, pInfo);
}

void LoadFromPngContent(PdfImage& image, png_structp pPng, png_infop pInfo)
{
    png_set_sig_bytes(pPng, 8);
    png_read_info(pPng, pInfo);

    // Begin
    png_uint_32 width;
    png_uint_32 height;
    int depth;
    int color_type;
    int interlace;

    png_get_IHDR(pPng, pInfo,
        &width, &height, &depth,
        &color_type, &interlace, NULL, NULL);

    // convert palette/gray image to rgb
    // expand gray bit depth if needed
    if (color_type == PNG_COLOR_TYPE_GRAY)
    {
#if PNG_LIBPNG_VER >= 10209
        png_set_expand_gray_1_2_4_to_8(pPng);
#else
        png_set_gray_1_2_4_to_8(pPng);
#endif
    }
    else if (color_type != PNG_COLOR_TYPE_PALETTE && depth < 8)
    {
        png_set_packing(pPng);
    }

    // transform transparency to alpha
    if (color_type != PNG_COLOR_TYPE_PALETTE && png_get_valid(pPng, pInfo, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(pPng);

    if (depth == 16)
        png_set_strip_16(pPng);

    if (interlace != PNG_INTERLACE_NONE)
        png_set_interlace_handling(pPng);

    //png_set_filler (pPng, 0xff, PNG_FILLER_AFTER);

    // recheck header after setting EXPAND options
    png_read_update_info(pPng, pInfo);
    png_get_IHDR(pPng, pInfo,
        &width, &height, &depth,
        &color_type, &interlace, NULL, NULL);
    // End

    // Read the file
    if (setjmp(png_jmpbuf(pPng)))
    {
        png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    size_t lRowLen = png_get_rowbytes(pPng, pInfo);
    size_t lLen = lRowLen * height;
    buffer_t buffer(lLen);

    unique_ptr<png_bytep[]> rows(new png_bytep[height]);
    for (unsigned int y = 0; y < height; y++)
    {
        rows[y] = reinterpret_cast<png_bytep>(buffer.data() + y * lRowLen);
    }

    png_read_image(pPng, rows.get());

    png_bytep paletteTrans;
    int numTransColors;
    if (color_type & PNG_COLOR_MASK_ALPHA
        || (color_type == PNG_COLOR_TYPE_PALETTE
            && png_get_valid(pPng, pInfo, PNG_INFO_tRNS)
            && png_get_tRNS(pPng, pInfo, &paletteTrans, &numTransColors, NULL)))
    {
        // Handle alpha channel and create smask
        buffer_t smask(width * height);
        png_uint_32 smaskIndex = 0;
        if (color_type == PNG_COLOR_TYPE_PALETTE)
        {
            for (png_uint_32 r = 0; r < height; r++)
            {
                png_bytep row = rows[r];
                for (png_uint_32 c = 0; c < width; c++)
                {
                    png_byte color;
                    if (depth == 8)
                        color = row[c];
                    else if (depth == 4)
                        color = c % 2 ? row[c / 2] >> 4 : row[c / 2] & 0xF;
                    else if (depth == 2)
                        color = (row[c / 4] >> c % 4 * 2) & 3;
                    else if (depth == 1)
                        color = (row[c / 4] >> c % 8) & 1;

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
            lLen = 3 * width * height;
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
            lLen = width * height;
        }
        PdfMemoryInputStream smaskstream(smask.data(), width * height);
        PdfImage smakeImage(*image.GetObject().GetDocument());
        smakeImage.SetImageColorSpace(PdfColorSpace::DeviceGray);
        smakeImage.SetImageData(smaskstream, width, height, 8);
        image.SetImageSoftmask(smakeImage);
    }

    // Set color space
    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_color* pColors;
        int numColors;
        png_get_PLTE(pPng, pInfo, &pColors, &numColors);

        char* datap = new char[numColors * 3];
        for (int i = 0; i < numColors; i++, pColors++)
        {
            datap[3 * i + 0] = pColors->red;
            datap[3 * i + 1] = pColors->green;
            datap[3 * i + 2] = pColors->blue;
        }
        PdfMemoryInputStream stream(datap, numColors * 3);
        PdfObject* pIdxObject = image.GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
        pIdxObject->GetOrCreateStream().Set(stream);

        PdfArray array;
        array.push_back(PdfName("DeviceRGB"));
        array.push_back(static_cast<int64_t>(numColors - 1));
        array.push_back(pIdxObject->GetIndirectReference());
        image.SetImageColorSpace(PdfColorSpace::Indexed, &array);
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        image.SetImageColorSpace(PdfColorSpace::DeviceGray);
    }
    else
    {
        image.SetImageColorSpace(PdfColorSpace::DeviceRGB);
    }

    // Set the image data and flate compress it
    PdfMemoryInputStream stream(buffer.data(), lLen);
    image.SetImageData(stream, width, height, depth);

    png_destroy_read_struct(&pPng, &pInfo, (png_infopp)NULL);
}

void pngReadData(png_structp pngPtr, png_bytep data, png_size_t length)
{
    pngData* a = (pngData*)png_get_io_ptr(pngPtr);
    a->read(data, length);
}

#endif // PODOFO_HAVE_PNG_LIB

PdfName PdfImage::ColorspaceToName( PdfColorSpace eColorSpace )
{
    return PdfColor::GetNameForColorSpace( eColorSpace ).GetString();
}

void PdfImage::SetImageChromaKeyMask(int64_t r, int64_t g, int64_t b, int64_t threshold)
{
    PdfArray array;
    array.push_back(r - threshold);
    array.push_back(r + threshold);
    array.push_back(g - threshold);
    array.push_back(g + threshold);
    array.push_back(b - threshold);
    array.push_back(b + threshold);

    this->GetObject().GetDictionary().AddKey("Mask", array);
}

void PdfImage::SetInterpolate(bool bValue)
{
    this->GetObject().GetDictionary().AddKey("Interpolate", PdfVariant(bValue));
}

unsigned PdfImage::GetWidth() const
{
    return m_width;
}

unsigned PdfImage::GetHeight() const
{
    return m_height;
}
