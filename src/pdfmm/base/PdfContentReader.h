/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CONTENT_READER_H
#define PDF_CONTENT_READER_H

#include "PdfCanvas.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfContentsTokenizer.h"

namespace mm {

enum class PdfContentsType2
{
    Unknown = 0,
    Keyword,         ///< The token is a PDF keyword.
    ImageDictionary, ///> Inline image dictionary
    ImageData        ///< The "token" is raw inline image data found between ID and EI tags (see PDF ref section 4.8.6)
};

struct PdfContentData
{
    PdfContentData();

    PdfContentsType2 Type = PdfContentsType2::Unknown;
    PdfContentOperator Operator = PdfContentOperator::Unknown;
    std::string_view Keyword;
    std::vector<PdfVariant> Stack;
    PdfDictionary InlineImageDictionary;
    PdfData InlineImageData;
};

enum class PdfContentReaderFlags
{
    None = 0,
    IgnoreInvalid = 1,
    DontFollowXObjects = 2,
};

typedef std::function<bool(const PdfContentData& content)> PdfContentHandler;
typedef std::function<bool(const PdfDictionary& dict, PdfInputDevice& device)> PdfInlineImageHandler;

struct PdfContentReaderArgs
{
    PdfContentReaderFlags Flags = PdfContentReaderFlags::None;
    PdfInlineImageHandler InlineImageHandler;
};

class PdfContentReader
{
public:
    static void Visit(const PdfCanvas& canvas, const PdfContentHandler& contentHandler, nullable<const PdfContentReaderArgs&> args = { });

    static void Visit(PdfInputDevice& device, const PdfContentHandler& contentHandler, nullable<const PdfContentReaderArgs&> args = { });

private:
    PdfContentReader(PdfInputDevice& device, const PdfContentHandler& handler, const PdfContentReaderArgs& args);

    void Visit();

private:
    void visit(PdfContentData& content);

    void handleContent(PdfContentData& content);

    bool tryReadNextContent(PdfContentData& content);

    bool tryHandleOperator(PdfContentData& content, bool& handled);

    bool tryFollowXObject(PdfContentData& content);

    bool tryReadInlineImgDict(PdfDictionary& dict);

    bool tryReadInlineImgData(PdfData& data);

    void handleUnsupportedPostScriptToken();

private:
    struct Storage
    {
        PdfPostScriptTokenType PsType;
        std::string_view Keyword;
        PdfVariant Variant;
        PdfName Name;
    };

private:
    PdfInputDevice* m_device;
    PdfContentHandler m_handler;
    PdfContentReaderArgs m_args;
    std::shared_ptr<chars> m_buffer;
    PdfPostScriptTokenizer m_tokenizer;
    bool m_readingInlineImgData;  // A state of reading inline image data

    // Temp storage
    Storage m_temp;
};

ENABLE_BITMASK_OPERATORS(mm::PdfContentReaderFlags);

};

#endif // PDF_CONTENT_READER_H
