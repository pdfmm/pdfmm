/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef PDF_CONTENTS_TOKENIZER_H
#define PDF_CONTENTS_TOKENIZER_H

#include "PdfDefines.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfInputDevice.h"

#include <list>

namespace PoDoFo {

class PdfDocument;
class PdfCanvas;
class PdfObject;

/** An enum describing the type of a read token
 */
enum class EPdfContentsType
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
class PODOFO_API PdfContentsTokenizer : private PdfTokenizer
{
public:
    /** Construct a PdfContentsTokenizer from an existing device
     *
     *  \param pBuffer pointer to a buffer
     *  \param lLen length of the buffer
     */
    PdfContentsTokenizer(const std::shared_ptr<PdfInputDevice>& device);

    /** Construct a PdfContentsTokenizer from a PdfCanvas
     *  (i.e. PdfPage or a PdfXObject).
     *
     *  This is more convinient as you do not have
     *  to care about buffers yourself.
     *
     *  \param pCanvas an object that hold a PDF contents stream
     */
    PdfContentsTokenizer(PdfCanvas& canvas);

    /** Read the next keyword or variant, returning true and setting reType if something was read.
     *  Either rpszKeyword or rVariant, but never both, have defined and usable values on
     *  true return, with which being controlled by the value of eType.
     *
     *  If EOF is encountered, returns false and leaves eType, pszKeyword and
     *  rVariant undefined.
     *
     *  As a special case, reType may be set to EPdfContentsType::ImageData. In
     *  this case rpszzKeyword is undefined, and rVariant contains a PdfData
     *  variant containing the byte sequence between the ID and BI keywords
     *  sans the one byte of leading- and trailing- white space. No filter
     *  decoding is performed.
     *
     *  \param[out] reType will be set to either keyword or variant if true is returned. Undefined
     *              if false is returned.
     *
     *  \param[out] rpszKeyword if pType is set to EPdfContentsType::Keyword this will point to the keyword,
     *              otherwise the value is undefined. If set, the value points to memory owned by the
     *              PdfContentsTokenizer and must not be freed. The value is invalidated when TryReadNext
     *              is next called or when the PdfContentsTokenizer is destroyed.
     *
     *  \param[out] rVariant if pType is set to EPdfContentsType::Variant or EPdfContentsType::ImageData
     *              this will be set to the read variant, otherwise the value is undefined.
     *
     */
    bool TryReadNext(EPdfContentsType& reType, std::string_view& rpszKeyword, PdfVariant & rVariant);

    void ReadNextVariant(PdfVariant& rVariant);
    bool TryReadNextVariant(PdfVariant& rVariant);

private:
    bool tryReadNext(EPdfContentsType& type, std::string_view& keyword, PdfVariant& variant);
    bool tryReadInlineImgDict(PdfDictionary& dict);
    bool tryReadInlineImgData(PdfData& data);

private:
    std::shared_ptr<PdfInputDevice> m_device;
    bool m_readingInlineImgData;  // A state of reading inline image data
};

};

#endif // PDF_CONTENTS_TOKENIZER_H
