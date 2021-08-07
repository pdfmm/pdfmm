/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CONTENTS_TOKENIZER_H
#define PDF_CONTENTS_TOKENIZER_H

#include "PdfPostScriptTokenizer.h"

namespace mm {

class PdfDocument;
class PdfCanvas;
class PdfObject;

/** An enum describing the type of a read token
 */
enum class PdfContentsType
{
    Unknown = 0,
    Keyword, ///< The token is a PDF keyword.
    Variant, ///< The token is a PDF variant. A variant is usually a parameter to a keyword
    ImageDictionary, ///> Inline image dictionary
    ImageData ///< The "token" is raw inline image data found between ID and EI tags (see PDF ref section 4.8.6)
};

/** This class is a parser for content streams in PDF documents.
 *
 *  The parsed content stream can be used and modified in various ways.
 *
 *  This class is currently work in progress and subject to change!
 */
class PDFMM_API PdfContentsTokenizer
{
public:
    /** Construct a PdfContentsTokenizer from an existing device
     */
    PdfContentsTokenizer(const std::shared_ptr<PdfInputDevice>& device);

    /** Construct a PdfContentsTokenizer from a PdfCanvas
     *  (i.e. PdfPage or a PdfXObject).
     *
     *  This is more convenient as you do not have
     *  to care about buffers yourself.
     *
     *  \param canvas an object that hold a PDF contents stream
     */
    PdfContentsTokenizer(PdfCanvas& canvas);

    /** Read the next keyword or variant, returning true and setting reType if something was read.
     *  Either keyword or variant, but never both, have defined and usable values on
     *  true return, with which being controlled by the value of contentsType.
     *
     *  If EOF is encountered, returns false and leaves contentsType, keyword and
     *  variant undefined.
     *
     *  As a special case, reType may be set to EPdfContentsType::ImageData. In
     *  this case keyword is undefined, and variant contains a PdfData
     *  variant containing the byte sequence between the ID and BI keywords
     *  sans the one byte of leading- and trailing- white space. No filter
     *  decoding is performed.
     *
     *  \param[out] contentsType will be set to either keyword or variant if true is returned. Undefined
     *              if false is returned.
     *
     *  \param[out] keyword if pType is set to EPdfContentsType::Keyword this will point to the keyword,
     *              otherwise the value is undefined. If set, the value points to memory owned by the
     *              PdfContentsTokenizer and must not be freed. The value is invalidated when TryReadNext
     *              is next called or when the PdfContentsTokenizer is destroyed.
     *
     *  \param[out] variant if pType is set to EPdfContentsType::Variant or EPdfContentsType::ImageData
     *              this will be set to the read variant, otherwise the value is undefined.
     *
     */
    bool TryReadNext(PdfContentsType& contentsType, std::string_view& keyword, PdfVariant& variant);

    void ReadNextVariant(PdfVariant& variant);
    bool TryReadNextVariant(PdfVariant& variant);

private:
    bool tryReadNext(PdfContentsType& type, std::string_view& keyword, PdfVariant& variant);
    bool tryReadInlineImgDict(PdfDictionary& dict);
    bool tryReadInlineImgData(PdfData& data);

private:
    PdfRefCountedBuffer m_buffer;
    PdfPostScriptTokenizer m_tokenizer;
    std::shared_ptr<PdfInputDevice> m_device;
    bool m_readingInlineImgData;  // A state of reading inline image data
};

};

#endif // PDF_CONTENTS_TOKENIZER_H
