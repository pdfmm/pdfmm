/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FILTERS_PRIVATE_H
#define PDF_FILTERS_PRIVATE_H

/**
 * \file PdfFiltersPrivate.h
 *
 * Provides implementations of various PDF stream filters.
 *
 * This is an internal header. It should not be included in podofo.h, and
 * should not be included directly by client applications. These filters should
 * only be accessed through the factory interface in PdfFilters.h .
 */

#include "PdfFilter.h"
#include "PdfRefCountedBuffer.h"

#include <zlib.h>

#ifdef PODOFO_HAVE_JPEG_LIB
extern "C" {
#ifdef WIN32		// Collision between Win32 and libjpeg headers
#define XMD_H
#undef FAR

#ifndef HAVE_BOOLEAN
#define HAVE_BOOLEAN
#define PODOFO_JPEG_HAVE_BOOLEAN // not to be defined in the build system
#endif

#endif
#include "jpeglib.h"

#ifdef PODOFO_JPEG_HAVE_BOOLEAN
#undef HAVE_BOOLEAN
#endif
}
#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB
extern "C" {
#include "tiffio.h"
#ifdef WIN32		// Collision between tiff and jpeg-headers
#define XMD_H
#undef FAR
#endif
}
#endif // PODOFO_HAVE_TIFF_LIB


namespace PoDoFo {

#define PODOFO_FILTER_INTERNAL_BUFFER_SIZE 4096

class PdfPredictorDecoder;
class PdfOutputDevice;

/** The ascii hex filter.
 */
class PdfHexFilter : public PdfFilter
{
public:
    PdfHexFilter();

    inline bool CanEncode() const override { return true; }

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::ASCIIHexDecode; }

private:
    char m_cDecodedByte;
    bool m_bLow;
};

/** The Ascii85 filter.
 */
class PdfAscii85Filter : public PdfFilter
{
public:
    PdfAscii85Filter();

    inline bool CanEncode() const override { return true; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::ASCII85Decode; }

private:
    void EncodeTuple(unsigned tuple, int bytes);
    void WidePut(unsigned tuple, int bytes) const;

private:
    int m_count;
    unsigned m_tuple;
};

/** The Flate filter.
 */
class PdfFlateFilter : public PdfFilter
{
public:
    PdfFlateFilter();

    virtual ~PdfFlateFilter();

    inline bool CanEncode() const override { return true; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary* pDecodeParms) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::FlateDecode; }

private:
    void EncodeBlockInternal(const char* pBuffer, size_t lLen, int nMode);

private:
    unsigned char m_buffer[PODOFO_FILTER_INTERNAL_BUFFER_SIZE];

    z_stream m_stream;
    PdfPredictorDecoder* m_pPredictor;
};

/** The RLE filter.
 */
class PdfRLEFilter : public PdfFilter
{
public:
    PdfRLEFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    inline PdfFilterType GetType() const override { return PdfFilterType::RunLengthDecode; }

private:
    int m_nCodeLen;
};

/** The LZW filter.
 */
class PdfLZWFilter : public PdfFilter
{
    struct TLzwItem
    {
        std::vector<unsigned char> value;
    };

    typedef std::vector<TLzwItem> TLzwTable;
    typedef TLzwTable::iterator TILzwTable;
    typedef TLzwTable::const_iterator TCILzwTable;

public:
    PdfLZWFilter();

    virtual ~PdfLZWFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::LZWDecode; }

private:
    void InitTable();

private:
    static const unsigned short s_masks[4];
    static const unsigned short s_clear;
    static const unsigned short s_eod;

    TLzwTable m_table;

    unsigned m_mask;
    unsigned m_code_len;
    unsigned char m_character;

    bool m_bFirst;

    PdfPredictorDecoder* m_pPredictor;
};

#ifdef PODOFO_HAVE_JPEG_LIB

void PODOFO_API jpeg_memory_src(j_decompress_ptr cinfo, const JOCTET* buffer, size_t bufsize);

extern "C"
{
    void JPegErrorExit(j_common_ptr cinfo);

    void JPegErrorOutput(j_common_ptr, int);
};

/** The DCT filter can decoded JPEG compressed data.
 *
 *  This filter requires JPEG lib to be available
 */
class PdfDCTFilter : public PdfFilter
{
public:
    PdfDCTFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::DCTDecode; }

private:
    struct jpeg_decompress_struct m_cinfo;
    struct jpeg_error_mgr m_jerr;

    PdfRefCountedBuffer m_buffer;
    PdfOutputDevice* m_pDevice;
};

#endif // PODOFO_HAVE_JPEG_LIB

#ifdef PODOFO_HAVE_TIFF_LIB

/** The CCITT filter can decoded CCITTFaxDecode compressed data.
 *
 *  This filter requires TIFFlib to be available
 */
class PdfCCITTFilter : public PdfFilter
{
public:
    PdfCCITTFilter();

    inline bool CanEncode() const override { return false; }

    void BeginEncodeImpl() override;

    void EncodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndEncodeImpl() override;

    inline bool CanDecode() const override { return true; }

    void BeginDecodeImpl(const PdfDictionary*) override;

    void DecodeBlockImpl(const char* pBuffer, size_t lLen) override;

    void EndDecodeImpl() override;

    inline PdfFilterType GetType() const override { return PdfFilterType::CCITTFaxDecode; }

private:
    TIFF* m_tiff;
};

#endif // PODOFO_HAVE_TIFF_LIB

};


#endif // PDF_FILTERS_PRIVATE_H
