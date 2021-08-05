/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_IMAGE_H
#define PDF_IMAGE_H

#include <cstdio>

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfFilter.h"
#include "PdfXObject.h"

namespace PoDoFo {

class PdfArray;
class PdfDocument;
class PdfInputStream;
class PdfObject;
class PdfVecObjects;

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
class PODOFO_DOC_API PdfImage final : public PdfXObject
{
public:
    /** Constuct a new PdfImage object
     *  This is an overloaded constructor.
     *
     *  \param parent parent document
     *  \param prefix optional prefix for XObject-name
     */
    PdfImage(PdfDocument & doc, const std::string_view & prefix = { });

    /** Construct an image from an existing PdfObject
     *
     *  \param obj a PdfObject that has to be an image
     */
    PdfImage(PdfObject& obj);

    /** Set the color space of this image. The default value is
     *  EPdfColorSpace::DeviceRGB.
     *  \param colorSpace one of EPdfColorSpace::DeviceGray, EPdfColorSpace::DeviceRGB and
     *                     EPdfColorSpace::DeviceCMYK, EPdfColorSpace::Indexed
     *  \param indexedData this parameter is required only for EPdfColorSpace::Indexed and
     *       it contains string with one number and then color palette, like "/DeviceRGB 15 <000000 00FF00...>"
     *       or the string array can be a resource name.
     *
     *  \see SetImageICCProfile to set an ICC profile instead of a simple colorspace
     */
    void SetImageColorSpace(PdfColorSpace colorSpace, const PdfArray* indexedData = nullptr);

    /** Get the color space of the image
    *
    *  \returns the color space of the image
    */
    PdfColorSpace GetImageColorSpace();

    /** Set an ICC profile for this image.
     *
     *  \param stream an input stream from which the ICC profiles data can be read
     *  \param colorComponents the number of colorcomponents of the ICC profile
     *  \param alternateColorSpace an alternate colorspace to use if the ICC profile cannot be used
     *
     *  \see SetImageColorSpace to set an colorspace instead of an ICC profile for this image
     */
    void SetImageICCProfile(PdfInputStream& stream, unsigned colorComponents,
        PdfColorSpace alternateColorSpace = PdfColorSpace::DeviceRGB);

    //EPdfColorSpace GetImageColorSpace() const;

    /** Set a softmask for this image.
     *  \param pSoftmask a PdfImage pointer to the image, which is to be set as softmask, must be 8-Bit-Grayscale
     *
     */
    void SetImageSoftmask(const PdfImage& softmask);

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
     *  use the overload of SetImageData which takes a TVecFilters
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
    void SetImageData(PdfInputStream& stream, unsigned width, unsigned height,
        unsigned bitsPerComponent, bool writeRect = true);

    /** Set the actual image data from an input stream
     *
     *  \param stream stream suplying raw image data
     *  \param width width of the image in pixels
     *  \param height height of the image in pixels
     *  \param bitsPerComponent bits per color component of the image (depends on the image colorspace you have set
     *                           but is 8 in most cases)
     *  \param filters these filters will be applied to compress the image data
     */
    void SetImageData(PdfInputStream& stream, unsigned width, unsigned height,
                      unsigned bitsPerComponent, PdfFilterList& filters, bool writeRect = true);

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
    void SetImageDataRaw(PdfInputStream& stream, unsigned width, unsigned height,
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

#ifdef PODOFO_HAVE_JPEG_LIB
    /** Load the image data from a JPEG file
     *  \param filename
     */
    void LoadFromJpeg(const std::string_view& filename);

    /** Load the image data from JPEG bytes
     *  \param data JPEG bytes
     *  \param len number of bytes
     */
    void LoadFromJpegData(const unsigned char* data, size_t len);

#endif // PODOFO_HAVE_JPEG_LIB
#ifdef PODOFO_HAVE_TIFF_LIB
    /** Load the image data from a TIFF file
     *  \param filename
     */
    void LoadFromTiff(const std::string_view& filename);

    /** Load the image data from TIFF bytes
     *  \param data TIFF bytes
     *  \param len number of bytes
     */
    void LoadFromTiffData(const unsigned char* data, size_t len);

#endif // PODOFO_HAVE_TIFF_LIB
#ifdef PODOFO_HAVE_PNG_LIB
    /** Load the image data from a PNG file
     *  \param filename
     */
    void LoadFromPng(const std::string_view& filename);

    /** Load the image data from PNG bytes
     *  \param data PNG bytes
     *  \param len number of bytes
     */
    void LoadFromPngData(const unsigned char* data, size_t len);
#endif // PODOFO_HAVE_PNG_LIB

    /** Set an color/chroma-key mask on an image.
     *  The masked color will not be painted, i.e. masked as being transparent.
     *
     *  \param r red RGB value of color that should be masked
     *  \param g green RGB value of color that should be masked
     *  \param b blue RGB value of color that should be masked
     *  \param threshold colors are masked that are in the range [(r-threshold, r+threshold),(g-threshold, g+threshold),(b-threshold, b+threshold)]
     */
    void SetImageChromaKeyMask(int64_t r, int64_t g, int64_t b, int64_t threshold = 0);

    /**
     * Apply an interpolation to the image if the source resolution
     * is lower than the resolution of the output device.
     * Default is false.
     * \param value whether the image should be interpolated
     */
    void SetInterpolate(bool value);

private:

    /** Converts a EPdfColorSpace enum to a name key which can be used in a
     *  PDF dictionary.
     *  \param colorSpace a valid colorspace
     *  \returns a valid key for this colorspace.
     */
    static PdfName ColorspaceToName(PdfColorSpace colorSpace);

#ifdef PODOFO_HAVE_JPEG_LIB
    void LoadFromJpegHandle(FILE* stream, const std::string_view& filename);
#endif // PODOFO_HAVE_JPEG_LIB
#ifdef PODOFO_HAVE_TIFF_LIB
    void LoadFromTiffHandle(void* handle);
#endif // PODOFO_HAVE_TIFF_LIB
#ifdef PODOFO_HAVE_PNG_LIB
    void LoadFromPngHandle(FILE* stream);
#endif // PODOFO_HAVE_PNG_LIB

    unsigned m_width;
    unsigned m_height;
};

};

#endif // PDF_IMAGE_H
