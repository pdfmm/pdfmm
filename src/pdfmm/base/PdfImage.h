/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_IMAGE_H
#define PDF_IMAGE_H

#include "PdfDeclarations.h"

#include <cstdio>

#include "PdfFilter.h"
#include "PdfXObject.h"

namespace mm {

class PdfArray;
class PdfDocument;
class InputStream;
class PdfObject;
class PdfIndirectObjectList;

/** A PdfImage object is needed when ever you want to embedd an image
 *  file into a PDF document.
 *  The PdfImage object is embedded once and can be drawn as often
 *  as you want on any page in the document using PdfPainter
 *
 *  \see GetImageReference
 *  \see PdfPainter::DrawImage
 *
 *  \see SetImageData
 */
class PDFMM_API PdfImage final : public PdfXObject
{
    friend class PdfXObject;

public:
    /** Constuct a new PdfImage object
     *  This is an overloaded constructor.
     *
     *  \param parent parent document
     *  \param prefix optional prefix for XObject-name
     */
    PdfImage(PdfDocument& doc, const std::string_view& prefix = { });

    void DecodeTo(charbuff& buff, PdfPixelFormat format);
    void DecodeTo(void* buffer, PdfPixelFormat format, int stride);
    void DecodeTo(OutputStream& stream, PdfPixelFormat format, int stride = -1);

    charbuff GetDecodedCopy(PdfPixelFormat format);

    /** Set the color space of this image. The default value is
     *  PdfColorSpace::DeviceRGB.
     *  \param colorSpace one of PdfColorSpace::DeviceGray, PdfColorSpace::DeviceRGB and
     *                     PdfColorSpace::DeviceCMYK, PdfColorSpace::Indexed
     *  \param indexedData this parameter is required only for PdfColorSpace::Indexed and
     *       it contains string with one number and then color palette, like "/DeviceRGB 15 <000000 00FF00...>"
     *       or the string array can be a resource name.
     *
     *  \see SetImageICCProfile to set an ICC profile instead of a simple colorspace
     */
    void SetColorSpace(PdfColorSpace colorSpace, const PdfArray* indexedData = nullptr);

    /** Get the color space of the image
    *
    *  \returns the color space of the image
    */
    PdfColorSpace GetColorSpace() const;

    /** Set an ICC profile for this image.
     *
     *  \param stream an input stream from which the ICC profiles data can be read
     *  \param colorComponents the number of colorcomponents of the ICC profile
     *  \param alternateColorSpace an alternate colorspace to use if the ICC profile cannot be used
     *
     *  \see SetImageColorSpace to set an colorspace instead of an ICC profile for this image
     */
    void SetICCProfile(InputStream& stream, unsigned colorComponents,
        PdfColorSpace alternateColorSpace = PdfColorSpace::DeviceRGB);

    //PdfColorSpace GetImageColorSpace() const;

    /** Set a softmask for this image.
     *  \param pSoftmask a PdfImage pointer to the image, which is to be set as softmask, must be 8-Bit-Grayscale
     *
     */
    void SetSoftmask(const PdfImage& softmask);

    /** Get the width of the image when drawn in PDF units
     *  \returns the width in PDF units
     */
    unsigned GetWidth() const;

    /** Get the height of the image when drawn in PDF units
     *  \returns the height in PDF units
     */
    unsigned GetHeight() const;

    /** Set the actual image data from an input stream
     *
     *  The image data will be flate compressed.
     *  If you want no compression or another filter to be applied
     *  use the overload of SetImageData which takes a filter list
     *  as argument.
     *
     *  \param stream stream supplieding raw image data
     *  \param width width of the image in pixels
     *  \param height height of the image in pixels
     *  \param bitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *
     *  \see SetImageData
     */
    void SetData(InputStream& stream, unsigned width, unsigned height,
        unsigned bitsPerComponent);

    /** Set the actual image data from an input stream
     *
     *  \param stream stream suplying raw image data
     *  \param width width of the image in pixels
     *  \param height height of the image in pixels
     *  \param bitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param filters these filters will be applied to compress the image data
     */
    void SetData(InputStream& stream, unsigned width, unsigned height,
                      unsigned bitsPerComponent, PdfFilterList& filters);

    /** Set the actual image data from an input stream.
     *  The data has to be encoded already and an appropriate
     *  filters key entry has to be set manually before!
     *
     *  \param stream stream supplieding raw image data
     *  \param width width of the image in pixels
     *  \param height height of the image in pixels
     *  \param bitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     */
    void SetDataRaw(InputStream& stream, unsigned width, unsigned height,
        unsigned bitsPerComponent);

    /** Load the image data from a file
     *  \param filename
     */
    void LoadFromFile(const std::string_view& filename);

    /** Load the image data from bytes
     *  \param data bytes
     *  \param len number of bytes
     */
    void LoadFromData(const unsigned char* data, size_t len);

#ifdef PDFMM_HAVE_JPEG_LIB
    /** Load the image data from a JPEG file
     *  \param filename
     */
    void LoadFromJpeg(const std::string_view& filename);

    /** Load the image data from JPEG bytes
     *  \param data JPEG bytes
     *  \param len number of bytes
     */
    void LoadFromJpegData(const unsigned char* data, size_t len);

#endif // PDFMM_HAVE_JPEG_LIB
#ifdef PDFMM_HAVE_TIFF_LIB
    /** Load the image data from a TIFF file
     *  \param filename
     */
    void LoadFromTiff(const std::string_view& filename);

    /** Load the image data from TIFF bytes
     *  \param data TIFF bytes
     *  \param len number of bytes
     */
    void LoadFromTiffData(const unsigned char* data, size_t len);

#endif // PDFMM_HAVE_TIFF_LIB
#ifdef PDFMM_HAVE_PNG_LIB
    /** Load the image data from a PNG file
     *  \param filename
     */
    void LoadFromPng(const std::string_view& filename);

    /** Load the image data from PNG bytes
     *  \param data PNG bytes
     *  \param len number of bytes
     */
    void LoadFromPngData(const unsigned char* data, size_t len);
#endif // PDFMM_HAVE_PNG_LIB

    /** Set an color/chroma-key mask on an image.
     *  The masked color will not be painted, i.e. masked as being transparent.
     *
     *  \param r red RGB value of color that should be masked
     *  \param g green RGB value of color that should be masked
     *  \param b blue RGB value of color that should be masked
     *  \param threshold colors are masked that are in the range [(r-threshold, r+threshold),(g-threshold, g+threshold),(b-threshold, b+threshold)]
     */
    void SetChromaKeyMask(int64_t r, int64_t g, int64_t b, int64_t threshold = 0);

    /**
     * Apply an interpolation to the image if the source resolution
     * is lower than the resolution of the output device.
     * Default is false.
     * \param value whether the image should be interpolated
     */
    void SetInterpolate(bool value);

    PdfRect GetRect() const override;

private:
    /** Construct an image from an existing PdfObject
     *
     *  \param obj a PdfObject that has to be an image
     */
    PdfImage(PdfObject& obj);

    charbuff initScanLine(PdfPixelFormat format, int stride, charbuff& smask);

    unsigned getBufferSize(PdfPixelFormat format) const;

    /** Converts a PdfColorSpace enum to a name key which can be used in a
     *  PDF dictionary.
     *  \param colorSpace a valid colorspace
     *  \returns a valid key for this colorspace.
     */
    static PdfName colorSpaceToName(PdfColorSpace colorSpace);

#ifdef PDFMM_HAVE_JPEG_LIB
    void loadFromJpegHandle(FILE* stream, const std::string_view& filename);
#endif // PDFMM_HAVE_JPEG_LIB
#ifdef PDFMM_HAVE_TIFF_LIB
    void loadFromTiffHandle(void* handle);
#endif // PDFMM_HAVE_TIFF_LIB
#ifdef PDFMM_HAVE_PNG_LIB
    void loadFromPngHandle(FILE* stream);
#endif // PDFMM_HAVE_PNG_LIB

private:
    unsigned m_Width;
    unsigned m_Height;
};

};

#endif // PDF_IMAGE_H
