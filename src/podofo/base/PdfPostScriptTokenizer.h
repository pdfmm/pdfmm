/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_POSTSCRIPT_TOKENIZER_H
#define PDF_POSTSCRIPT_TOKENIZER_H

#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfInputDevice.h"

namespace PoDoFo {

class PdfDocument;
class PdfCanvas;
class PdfObject;


/** An enum describing the type of a read token
 */
enum class EPdfPostScriptTokenType
{
    Unknown = 0,
    Keyword, ///< The token is a PDF keyword.
    Variant, ///< The token is a PDF variant. A variant is usually a parameter to a keyword
    ProcedureEnter, ///< Procedure enter delimiter
    ProcedureExit, ///< Procedure enter delimiter
};

/** This class is a parser for general PostScript content in PDF documents.
 */
class PODOFO_API PdfPostScriptTokenizer final : private PdfTokenizer
{
public:
    PdfPostScriptTokenizer();
    PdfPostScriptTokenizer(const PdfRefCountedBuffer& buffer);
public:
    bool TryReadNext(PdfInputDevice& device, EPdfPostScriptTokenType& tokenType, std::string_view& keyword, PdfVariant& variant);
    void ReadNextVariant(PdfInputDevice& device, PdfVariant& variant);
    bool TryReadNextVariant(PdfInputDevice& device, PdfVariant& variant);
};

};

#endif // PDF_POSTSCRIPT_TOKENIZER_H
