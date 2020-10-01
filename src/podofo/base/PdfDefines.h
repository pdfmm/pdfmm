/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_DEFINES_H_
#define _PDF_DEFINES_H_

/** \file PdfDefines.h
 *        This file should be included as the FIRST file in every header of
 *        PoDoFo lib. It includes all standard files, defines some useful
 *        macros, some datatypes and all important enumeration types. On
 *        supporting platforms it will be precompiled to speed compilation.
 */ 

#include "PdfCompilerCompat.h"

/**
 * PoDoFo version - 24-bit integer representation.
 * Version is 0xMMmmpp  where M is major, m is minor and p is patch
 * eg 0.7.0  is represented as 0x000700
 * eg 0.7.99 is represented as 0x000763
 *
 * Note that the PoDoFo version is available in parts as individual 8-bit
 * integer literals in PODOFO_VERSION_MAJOR, PODOFO_VERSION_MINOR and
 * PODOFO_VERSION_PATCH .
 */
#define PODOFO_MAKE_VERSION_REAL(M,m,p) ( (M<<16)+(m<<8)+(p) )
#define PODOFO_MAKE_VERSION(M,m,p) PODOFO_MAKE_VERSION_REAL(M,m,p)
#define PODOFO_VERSION PODOFO_MAKE_VERSION(PODOFO_VERSION_MAJOR, PODOFO_VERSION_MINOR, PODOFO_VERSION_PATCH)

/**
 * PoDoFo version represented as a string literal, eg '0.7.99'
 */
// The \0 is from Win32 example resources and the other values in PoDoFo's one
#define PODOFO_MAKE_VERSION_STR_REAL(M,m,p) M ## . ## m ## . ## p
#define PODOFO_STR(x) #x "\0"
#define PODOFO_XSTR(x) PODOFO_STR(x)
#define PODOFO_MAKE_VERSION_STR(M,m,p) PODOFO_XSTR(PODOFO_MAKE_VERSION_STR_REAL(M,m,p))
#define PODOFO_VERSION_STR PODOFO_MAKE_VERSION_STR(PODOFO_VERSION_MAJOR, PODOFO_VERSION_MINOR, PODOFO_VERSION_PATCH)

#ifndef PODOFO_COMPILE_RC

#ifndef PODOFO_UNUSED_PARAM
#define PODOFO_UNUSED_PARAM(x)
#endif

// Include common system files
// (most are now pulled in my PdfCompilerCompat.h)
#include <cwchar>
#include <cstdint>

// Include common STL files
#include <memory>
#include <string>
#include <vector>

/** \def VERBOSE_DEBUG_DISABLED
 *  Debug define. Enable it, if you need
 *  more debuf output to the commandline from PoDoFo
 *
 *  Setting VERBOSE_DEBUG_DISABLED will make PoDoFo
 *  EXTREMELY slow and verbose, so it's not practical
 *  even for regular debuggin.
 */
#define VERBOSE_DEBUG_DISABLED

// Should we do lots of extra (expensive) sanity checking?  You should not
// define this on production builds because of the runtime cost and because it
// might cause the library to abort() if it notices something nasty.
// It may also change the size of some objects, and is thus not binary
// compatible.
//
// If you don't know you need this, avoid it.
//
#define EXTRA_CHECKS_DISABLED

// Error Handling Defines
#include "PdfError.h"

// Memory management
#include "PdfMemoryManagement.h"

// Include API macro definitions
#include "podofoapi.h"

#ifdef DEBUG
#include <cassert>
#define PODOFO_ASSERT( x ) assert( x );
#else
#define PODOFO_ASSERT( x ) do { if (!(x)) PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, #x); } while (false)
#endif // DEBUG

// By default, PoDoFo will use C++ locale support to ensure that
// it doesn't write bad PDF data - particularly floating point numbers.
// If your standard library does not support locales this won't work, but
// your STL probably writes all data in a POSIX-like way irrespective of
// locale. If you set this to 0, you MUST use some other method to ensure
// that streams used by PoDoFo will write data in a POSIX locale like manner.
#ifndef USE_CXX_LOCALE
#define USE_CXX_LOCALE 1
#endif

/**
 * \namespace PoDoFo
 * 
 * All classes, functions, types and enums of PoDoFo
 * are members of these namespace.
 *
 * If you use PoDoFo, you might want to add the line:
 *       using namespace PoDoFo;
 * to your application.
 */ 
namespace PoDoFo {

/* Explicitly big-endian short, suitable for unicode text */
typedef uint16_t     pdf_utf16be;
/* Typedef to indicate utf-8 encoded data */
typedef unsigned char  pdf_utf8;

// Enums

/**
 * Enum to identify diferent versions of the PDF file format
 */
enum class EPdfVersion
{
    V1_0 = 0,       /**< PDF 1.0 */
    V1_1,           /**< PDF 1.1 */
    V1_2,           /**< PDF 1.2 */  
    V1_3,           /**< PDF 1.3 */ 
    V1_4,           /**< PDF 1.4 */
    V1_5,           /**< PDF 1.5 */
    V1_6,           /**< PDF 1.6 */ 
    V1_7,           /**< PDF 1.7 */
    V2_0,           /**< PDF 1.7 */
};

/** The default PDF Version used by new PDF documents
 *  in PoDoFo. 
 */
constexpr EPdfVersion PdfVersionDefault = EPdfVersion::V1_3;

/**
 * Specify additional options for writing the PDF.
 */
enum class EPdfWriteMode
{
    Compact = 0x01, ///< Try to write the PDF as compact as possible (Default)
    Clean = 0x02,   ///< Create a PDF that is readable in a text editor, i.e. insert spaces and linebreaks between tokens
};

const EPdfWriteMode PdfWriteModeDefault = EPdfWriteMode::Compact;

/**
 * Every PDF datatype that can occur in a PDF file
 * is referenced by an own enum (e.g. Bool or String).
 *
 * \see PdfVariant
 *
 * Remember to update PdfVariant::GetDataTypeString() when adding members here.
 */
enum class EPdfDataType : uint8_t
{
   Unknown = 0,         /**< The Datatype is unknown. The value is chosen to enable value storage in 8-bit unsigned integer. */
   Bool,                  /**< Boolean datatype: Accepts the values "true" and "false" */
   Number,                /**< Number datatype for integer values */
   Real,                  /**< Real datatype for floating point numbers */
   String,                /**< String datatype in PDF file. Strings have the form (Hallo World!) in PDF files. \see PdfString */
   Name,                  /**< Name datatype. Names are used as keys in dictionary to reference values. \see PdfName */
   Array,                 /**< An array of other PDF data types. */
   Dictionary,            /**< A dictionary associates keys with values. A key can have another dictionary as value. */
   Null,                  /**< The null datatype is always null. */
   Reference,             /**< The reference datatype contains references to PDF objects in the PDF file of the form 4 0 R. \see PdfObject */
   RawData,               /**< Raw PDF data */
};

/**
 * Every filter that can be used to encode a stream 
 * in a PDF file is referenced by an own enum value.
 * Common filters are EPdfFilter::FlateDecode (i.e. Zip) or
 * EPdfFilter::ASCIIHexDecode
 */
enum class EPdfFilter
{
    None = -1,                 /**< Do not use any filtering */
    ASCIIHexDecode,            /**< Converts data from and to hexadecimal. Increases size of the data by a factor of 2! \see PdfHexFilter */
    ASCII85Decode,             /**< Converts to and from Ascii85 encoding. \see PdfAscii85Filter */
    LZWDecode,                 
    FlateDecode,               /**< Compress data using the Flate algorithm of ZLib. This filter is recommended to be used always. \see PdfFlateFilter */
    RunLengthDecode,           /**< Run length decode data. \see PdfRLEFilter */
    CCITTFaxDecode,
    JBIG2Decode,
    DCTDecode,
    JPXDecode,
    Crypt
};


/**
 * Enum for the different font formats supported by PoDoFo
 */
enum class EPdfFontType
{
    TrueType,
    Type1Pfa,
    Type1Pfb,
    Type1Base14,
    Type3,
    Unknown = 0xff
};

/** 
 * Enum for the colorspaces supported
 * by PDF.
 */
enum class EPdfColorSpace
{
    DeviceGray,        /**< Gray */
    DeviceRGB,         /**< RGB  */
    DeviceCMYK,        /**< CMYK */
    Separation,        /**< Separation */
    CieLab,            /**< CIE-Lab */
    Indexed,           /**< Indexed */
    Unknown = 0xff
};

/**
 * Enum for text rendering mode (Tr)
 */
enum class EPdfTextRenderingMode
{
    Fill = 0,                 /**< Default mode, fill text */
    Stroke,                   /**< Stroke text */
    FillAndStroke,            /**< Fill, then stroke text */
    Invisible,                /**< Neither fill nor stroke text (invisible) */
    FillToClipPath,           /**< Fill text and add to path for clipping */
    StrokeToClipPath,         /**< Stroke text and add to path for clipping */
    FillAndStrokeToClipPath,  /**< Fill, then stroke text and add to path for clipping */
    ToClipPath,               /**< Add text to path for clipping */
    Unknown = 0xff
};

/**
 * Enum for the different stroke styles that can be set
 * when drawing to a PDF file (mostly for line drawing).
 */
enum class EPdfStrokeStyle
{
    Solid,
    Dash,
    Dot,
    DashDot,
    DashDotDot,
    Custom 
};

/**
 * Enum to specifiy the initial information of the
 * info dictionary.
 */
enum class EPdfInfoInitial
{
    WriteCreationTime = 0x01, ///< Write the creation time (current time). Default for new documents.
    WriteModificationTime = 0x02, ///< Write the modification time (current time). Default for loaded documents.
    WriteProducer = 0x04  ///< Write producer key. Default for new documents.
};

/**
 * Enum for predefined tiling patterns.
 */
enum class EPdfTilingPatternType
{
    BDiagonal = 1,
    Cross,
    DiagCross,
    FDiagonal,
    Horizontal,
    Vertical,
    Image
};

/**
 * Enum for line cap styles when drawing.
 */
enum class EPdfLineCapStyle
{
    Butt    = 0,
    Round   = 1,
    Square  = 2
};

/**
 * Enum for line join styles when drawing.
 */
enum class EPdfLineJoinStyle
{
    Miter   = 0,
    Round   = 1,
    Bevel   = 2
};

/**
 * Enum for vertical text alignment
 */
enum class EPdfVerticalAlignment
{
    Top    = 0,
    Center = 1,
    Bottom  = 2
};

/**
 * Enum for text alignment
 */
enum class EPdfAlignment
{
    Left    = 0,
    Center  = 1,
    Right   = 2
};

enum class PdfSaveOptions
{
    None,
};

/**
 * List of defined Rendering intents
 */
#define ePdfRenderingIntent_AbsoluteColorimetric	"AbsoluteColorimetric"
#define ePdfRenderingIntent_RelativeColorimetric	"RelativeColorimetric"
#define ePdfRenderingIntent_Perceptual			"Perceptual"
#define ePdfRenderingIntent_Saturation			"Saturation"

/**
 * List of defined transparency blending modes
 */
#define ePdfBlendMode_Normal		"Normal"
#define ePdfBlendMode_Multiply		"Multiply"
#define ePdfBlendMode_Screen		"Screen"
#define ePdfBlendMode_Overlay		"Overlay"
#define ePdfBlendMode_Darken		"Darken"
#define ePdfBlendMode_Lighten		"Lighten"
#define ePdfBlendMode_ColorDodge	"ColorDodge"
#define ePdfBlendMode_ColorBurn		"ColorBurn"
#define ePdfBlendMode_HardLight		"HardLight"
#define ePdfBlendMode_SoftLight		"SoftLight"
#define ePdfBlendMode_Difference	"Difference"
#define ePdfBlendMode_Exclusion		"Exclusion"
#define ePdfBlendMode_Hue		"Hue"
#define ePdfBlendMode_Saturation	"Saturation"
#define ePdfBlendMode_Color		"Color"
#define ePdfBlendMode_Luminosity	"Luminosity"

/**
 * Enum holding the supported page sizes by PoDoFo.
 * Can be used to construct a PdfRect structure with 
 * measurements of a page object.
 *
 * \see PdfPage
 */
enum class EPdfPageSize
{
    A0,              /**< DIN A0  */
    A1,              /**< DIN A1  */
    A2,              /**< DIN A2  */
    A3,              /**< DIN A3  */
    A4,              /**< DIN A4  */
    A5,              /**< DIN A5  */
    A6,              /**< DIN A6  */
    Letter,          /**< Letter  */
    Legal,           /**< Legal   */
    Tabloid,         /**< Tabloid */

    Unknown = 0xff
};

/**
 * Enum holding the supported of types of "PageModes"
 * that define which (if any) of the "panels" are opened
 * in Acrobat when the document is opened.
 *
 * \see PdfDocument
 */
enum class EPdfPageMode
{
    DontCare,
    UseNone,
    UseThumbs,
    UseBookmarks,
    FullScreen,
    UseOC,
    UseAttachments
};

/**
 * Enum holding the supported of types of "PageLayouts"
 * that define how Acrobat will display the pages in
 * relation to each other
 *
 * \see PdfDocument
 */
enum class EPdfPageLayout
{
    Ignore,
    Default,
    SinglePage,
    OneColumn,
    TwoColumnLeft,
    TwoColumnRight,
    TwoPageLeft,
    TwoPageRight
};

/**
 */
const bool ePdfCreateObject = true;
const bool ePdfDontCreateObject = false;

// character constants
#define MAX_PDF_VERSION_STRING_INDEX  8

// We use fixed bounds two dimensional arrays here so that
// they go into the const data section of the library.
static const char s_szPdfVersions[][9] = {
    "%PDF-1.0",
    "%PDF-1.1",
    "%PDF-1.2",
    "%PDF-1.3",
    "%PDF-1.4",
    "%PDF-1.5",
    "%PDF-1.6",
    "%PDF-1.7",
    "%PDF-2.0",
};

static const char s_szPdfVersionNums[][4] = {
    "1.0",
    "1.1",
    "1.2",
    "1.3",
    "1.4",
    "1.5",
    "1.6",
    "1.7",
    "2.0",
};

/// PDF Reference, Section 3.1.1, Table 3.1, White-space characters
const int s_nNumWhiteSpaces = 6;
const char s_cWhiteSpaces[] = {
    0x00, // NULL
    0x09, // TAB
    0x0A, // Line Feed
    0x0C, // Form Feed
    0x0D, // Carriage Return
    0x20, // White Space
    0x00  // end marker
};

/// PDF Reference, Section 3.1.1, Character Set
static const int s_nNumDelimiters = 10;
static const char s_cDelimiters[] = {
    '(',
    ')',
    '<',
    '>',
    '[',
    ']',
    '{',
    '}',
    '/',
    '%',
    '\0' // end marker
};

#ifndef PODOFO_CONVERSION_CONSTANT
#define PODOFO_CONVERSION_CONSTANT 0.002834645669291339
#endif // PODOFO_CONVERSION_CONSTANT

}; // end namespace PoDoFo

ENABLE_BITMASK_OPERATORS(PoDoFo::EPdfWriteMode);
ENABLE_BITMASK_OPERATORS(PoDoFo::EPdfInfoInitial);

/**
 * \mainpage
 *
 * <b>PoDoFo</b> is a library to work with the PDF file format and includes also a few
 * tools. The name comes from the first letter of PDF (Portable Document
 * Format). 
 * 
 * The <b>PoDoFo</b> library is a free portable C++ library which includes
 * classes to parse a PDF file and modify its contents into memory. The changes
 * can be written back to disk easily. The parser could also be used to write a
 * PDF viewer. Besides parsing PoDoFo includes also very simple classes to create
 * your own PDF files. All classes are documented so it is easy to start writing
 * your own application using PoDoFo.
 * 
 * The <b>PoDoFo</b> tools are simple tools build around the <b>PoDoFo</b> library. These tools
 * are first of all examples on how to use the <b>PoDoFo</b> library in your own
 * projects. But secondly they offer also features for working with PDF
 * files. More tools will come with future release and the existing tools will
 * gain more features. Currently there are two tools: podofoimgextract (which
 * extracts all jpeg images from a given PDF file) and podofouncompress (which
 * removes all compression filters from a PDF file - this is useful for debugging
 * existing PDF files).
 * 
 * Additionally there is the external tool <b>PoDoFoBrowser</b> which is not included in
 * this package, but can be downloaded from the <b>PoDoFo</b> webpage. <b>PoDoFoBrowser</b> is
 * a Qt application for browsing the objects in a PDF file and modifying their
 * keys easily. It is very useful if you want to look on the internal structure
 * of PDF files.
 * 
 * As of now <b>PoDoFo</b> is available for Unix, Mac OS X and Windows platforms. 
 *
 * More information can be found at: http://podofo.sourceforge.net
 *
 * <b>PoDoFo</b> is created by Dominik Seichter <domseichter@web.de>, 
 * Leonard Rosenthol <leonardr@pdfsages.com> and Craig Ringer <craig@postnewspapers.com.au>
 *
 * \page Codingstyle (Codingstyle)
 * \verbinclude CODINGSTYLE.txt
 *
 */

#endif // !PODOFO_COMPILE_RC

#endif // _PDF_DEFINES_H_
