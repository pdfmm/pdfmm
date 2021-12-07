#include "PdfContentReader.h"

#include <pdfmm/private/OperatorUtils.h>

#include "PdfCanvasInputDevice.h"
#include "PdfData.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfContentReader::PdfContentReader(PdfInputDevice& device, const PdfContentHandler& handler, const PdfContentReaderArgs& args) :
    m_device(&device),
    m_handler(handler),
    m_args(args),
    m_buffer(std::make_shared<chars>(PdfTokenizer::BufferSize)),
    m_tokenizer(m_buffer),
    m_readingInlineImgData(false),
    m_temp{ }
{
}

void PdfContentReader::Visit(const PdfCanvas& canvas, const PdfContentHandler& handler, nullable<const PdfContentReaderArgs&> args)
{
    PdfCanvasInputDevice device(canvas);
    Visit(device, handler, args);
}

void PdfContentReader::Visit(PdfInputDevice& device, const PdfContentHandler& handler, nullable<const PdfContentReaderArgs&> args)
{
    PdfContentReaderArgs actualArgs;
    if (args.has_value())
        actualArgs = *args;

    PdfContentReader reader(device, handler, actualArgs);
    reader.Visit();
}

void PdfContentReader::Visit()
{
    PdfContentData content;
    visit(content);
}

void PdfContentReader::visit(PdfContentData& content)
{
    while (true)
    {
        if (m_readingInlineImgData)
        {
            if (m_args.InlineImageHandler == nullptr)
            {
                if (!tryReadInlineImgData(content.InlineImageData))
                    return;

                content.Type = PdfContentsType2::ImageData;
                handleContent(content);
                m_readingInlineImgData = false;
                continue;
            }
            else
            {
                if (!m_args.InlineImageHandler(content.InlineImageDictionary, *m_device))
                    return;

                // Consume the EI end image operator
                (void)tryReadNextContent(content);
                if (content.Operator != PdfContentOperator::EI)
                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Missing end of inline image EI operator");

                m_readingInlineImgData = false;
            }
        }

        if (tryReadNextContent(content))
            return;

        handleContent(content);
    }
}

void PdfContentReader::handleContent(PdfContentData& content)
{
    // Do some cleaning
    switch (content.Type)
    {
        case PdfContentsType2::Keyword:
        {
            content.InlineImageData = PdfData();
            content.InlineImageDictionary = PdfDictionary();
            break;
        }
        case PdfContentsType2::ImageDictionary:
        {
            content.Operator = PdfContentOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageData = PdfData();
            break;
        }
        case PdfContentsType2::ImageData:
        {
            content.Operator = PdfContentOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary = PdfDictionary();
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported flow");
        }
    }

    // Call the registered handler
    if (!m_handler(content))
        return;

    // Reset the stack
    content.Stack.resize(1);
    content.Stack[0] = PdfVariant();
}

// Returns false in case of EOF
bool PdfContentReader::tryReadNextContent(PdfContentData& content)
{
    while (true)
    {
        bool gotToken = m_tokenizer.TryReadNext(*m_device, m_temp.PsType, content.Keyword, content.Stack.back());
        if (!gotToken)
        {
            content.Type = PdfContentsType2::Unknown;
            return false;
        }

        switch (m_temp.PsType)
        {
            case PdfPostScriptTokenType::Keyword:
            {
                (void)TryGetPDFOperator(content.Keyword, content.Operator);
                bool handled;
                if (!tryHandleOperator(content, handled))
                    return false;

                if (handled)
                    continue;

                return true;
            }
            case PdfPostScriptTokenType::Variant:
            {
                content.Stack.resize(content.Stack.size() + 1);
                continue;
            }
            default:
            {
                handleUnsupportedPostScriptToken();
                continue;
            }
        }
    }
}

// Returns false in case of EOF
bool PdfContentReader::tryHandleOperator(PdfContentData& content, bool& handled)
{
    // By default it's not handled
    handled = false;
    switch (content.Operator)
    {
        case PdfContentOperator::Do:
        {
            if ((m_args.Flags & PdfContentReaderFlags::DontFollowXObjects)
                != PdfContentReaderFlags::None)
            {
                // Don't follow XObject
                return true;
            }

            if (!tryFollowXObject(content))
                return false;

            handled = true;
            return true;
        }
        case PdfContentOperator::BI:
        {
            if (!tryReadInlineImgDict(content.InlineImageDictionary))
                return false;

            content.Type = PdfContentsType2::ImageDictionary;
            m_readingInlineImgData = true;
            handled = true;
            return true;
        }
        default:
        {
            // Not handled
            return true;
        }
    }
}

// Returns false in case of EOF
bool PdfContentReader::tryReadInlineImgDict(PdfDictionary& dict)
{
    while (true)
    {
        if (!m_tokenizer.TryReadNext(*m_device, m_temp.PsType, m_temp.Keyword, m_temp.Variant))
            return false;

        switch (m_temp.PsType)
        {
            case PdfPostScriptTokenType::Keyword:
            {
                // Try to find end of dictionary
                if (m_temp.Keyword == "ID")
                    return true;
                else
                    return false;
            }
            case PdfPostScriptTokenType::Variant:
            {
                if (m_temp.Variant.TryGetName(m_temp.Name))
                    break;
                else
                    return false;
            }
            default:
            {
                handleUnsupportedPostScriptToken();
                break;
            }
        }

        if (m_tokenizer.TryReadNextVariant(*m_device, m_temp.Variant))
            dict.AddKey(m_temp.Name, m_temp.Variant);
        else
            return false;
    }
}

// Returns false in case of EOF
bool PdfContentReader::tryFollowXObject(PdfContentData& content)
{
    (void)content;
    return true;
}

// Returns false in case of EOF
bool PdfContentReader::tryReadInlineImgData(PdfData& data)
{
    // Consume one whitespace between ID and data
    char ch;
    if (!m_device->TryGetChar(ch))
        return false;

    // Read "EI"
    enum class ReadEIStatus
    {
        ReadE,
        ReadI,
        ReadWhiteSpace
    };

    // NOTE: This is a better version of the previous approach
    // and still is wrong since the Pdf specification is broken
    // with this regard. The dictionary should have a /Length
    // key with the length of the data, and it's a requirement
    // in Pdf 2.0 specification (ISO 32000-2). To handle better
    // the situation the only approach would be to use more
    // comprehensive heuristic, similarly to what pdf.js does
    ReadEIStatus status = ReadEIStatus::ReadE;
    unsigned readCount = 0;
    while (m_device->TryGetChar(ch))
    {
        switch (status)
        {
            case ReadEIStatus::ReadE:
            {
                if (ch == 'E')
                    status = ReadEIStatus::ReadI;

                break;
            }
            case ReadEIStatus::ReadI:
            {
                if (ch == 'I')
                    status = ReadEIStatus::ReadWhiteSpace;
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
            case ReadEIStatus::ReadWhiteSpace:
            {
                if (PdfTokenizer::IsWhitespace(ch))
                {
                    data = cspan<char>(m_buffer->data(), readCount - 2);
                    return true;
                }
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
        }

        if (m_buffer->size() == readCount)
        {
            // image is larger than buffer => resize buffer
            m_buffer->resize(m_buffer->size() * 2);
        }

        m_buffer->data()[readCount] = ch;
        readCount++;
    }

    return false;
}

void PdfContentReader::handleUnsupportedPostScriptToken()
{
    if ((m_args.Flags & PdfContentReaderFlags::IgnoreInvalid) != PdfContentReaderFlags::None)
        return;

    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidContentStream, "Unsupported PostScript content");
}

PdfContentData::PdfContentData() :
    Type(PdfContentsType2::Unknown),
    Operator(PdfContentOperator::Unknown),
    Stack(1) { }
