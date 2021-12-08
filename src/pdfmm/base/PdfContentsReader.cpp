#include "PdfContentsReader.h"

#include "PdfOperatorUtils.h"
#include "PdfCanvasInputDevice.h"
#include "PdfData.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfContentsReader::PdfContentsReader(const PdfCanvas& canvas, nullable<const PdfContentReaderArgs&> args)
    : PdfContentsReader(new PdfCanvasInputDevice(canvas), true, args) { }

PdfContentsReader::PdfContentsReader(PdfInputDevice& device, nullable<const PdfContentReaderArgs&> args)
    : PdfContentsReader(&device, false, args) { }

PdfContentsReader::PdfContentsReader(PdfInputDevice* device, bool ownDevice, nullable<const PdfContentReaderArgs&> args) :
    m_ownDevice(ownDevice),
    m_device(device),
    m_args(args.has_value() ? *args : PdfContentReaderArgs()),
    m_buffer(std::make_shared<chars>(PdfTokenizer::BufferSize)),
    m_tokenizer(m_buffer),
    m_readingInlineImgData(false),
    m_temp{ }
{
}

PdfContentsReader::~PdfContentsReader()
{
    if (m_ownDevice)
        delete m_device;
}

bool PdfContentsReader::TryReadNext(PdfContent& content)
{
    // Reset the stack and warnings before reading more content
    resetContent(content);

    if (m_readingInlineImgData)
    {
        if (m_args.InlineImageHandler == nullptr)
        {
            if (!tryReadInlineImgData(content.InlineImageData))
                return false;

            content.Type = PdfContentType::ImageData;
            m_readingInlineImgData = false;
            cleanContent(content);
            return true;
        }
        else
        {
            if (!m_args.InlineImageHandler(content.InlineImageDictionary, *m_device))
                return false;

            // Consume the EI end image operator
            (void)tryReadNextContent(content);
            if (content.Operator != PdfOperator::EI)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Missing end of inline image EI operator");

            m_readingInlineImgData = false;
        }
    }

    if (!tryReadNextContent(content))
        return false;

    cleanContent(content);
    handleWarnings();
    return true;
}

// Returns false in case of EOF
bool PdfContentsReader::tryReadNextContent(PdfContent& content)
{
    while (true)
    {
        bool gotToken = m_tokenizer.TryReadNext(*m_device, m_temp.PsType, content.Keyword, m_temp.Variant);
        if (!gotToken)
        {
            content.Type = PdfContentType::Unknown;
            return false;
        }

        switch (m_temp.PsType)
        {
            case PdfPostScriptTokenType::Keyword:
            {
                content.Type = PdfContentType::Operator;
                if (!TryGetPdfOperator(content.Keyword, content.Operator))
                {
                    content.Warnings |= PdfContentWarnings::InvalidOperator;
                    return true;
                }

                int operandCount = mm::GetOperandCount(content.Operator);
                if (operandCount != -1 && content.Stack.GetSize() != (unsigned)operandCount)
                {
                    if (content.Stack.GetSize() < (unsigned)operandCount)
                        content.Warnings |= PdfContentWarnings::InvalidOperator;
                    else // Stack.GetSize() > operandCount
                        content.Warnings |= PdfContentWarnings::InvalidSpuriousOperands;
                    return true;
                }

                bool handled;
                if (!tryHandleOperator(content, handled))
                    return false;

                if (handled)
                {
                    // Reset the stack and warnings before
                    // reading more content
                    resetContent(content);
                    continue;
                }

                return true;
            }
            case PdfPostScriptTokenType::Variant:
            {
                content.Stack.Push(m_temp.Variant);
                continue;
            }
            default:
            {
                content.Warnings |= PdfContentWarnings::InvalidPostScriptContent;
                continue;
            }
        }
    }
}

void PdfContentsReader::resetContent(PdfContent& content)
{
    content.Stack.Clear();
    content.Warnings = PdfContentWarnings::None;
}

void PdfContentsReader::cleanContent(PdfContent& content)
{
    // Do some cleaning
    switch (content.Type)
    {
        case PdfContentType::Operator:
        {
            content.InlineImageData = PdfData();
            content.InlineImageDictionary = PdfDictionary();
            break;
        }
        case PdfContentType::ImageDictionary:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageData = PdfData();
            break;
        }
        case PdfContentType::ImageData:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary = PdfDictionary();
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported flow");
        }
    }
}

// Returns false in case of EOF
bool PdfContentsReader::tryHandleOperator(PdfContent& content, bool& handled)
{
    // By default it's not handled
    handled = false;
    switch (content.Operator)
    {
        case PdfOperator::Do:
        {
            if ((m_args.Flags & PdfContentReaderFlags::DontFollowXObjects)
                != PdfContentReaderFlags::None)
            {
                // Don't follow XObject
                return true;
            }

            if (!tryFollowXObject(content))
                content.Warnings |= PdfContentWarnings::InvalidXObject;

            handled = true;
            return true;
        }
        case PdfOperator::BI:
        {
            if (!tryReadInlineImgDict(content))
                return false;

            content.Type = PdfContentType::ImageDictionary;
            m_readingInlineImgData = true;
            handled = true;
            return true;
        }
        default:
        {
            // Not handled operator
            return true;
        }
    }
}

// Returns false in case of EOF
bool PdfContentsReader::tryReadInlineImgDict(PdfContent& content)
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
                content.Warnings |= PdfContentWarnings::InvalidPostScriptContent;
                break;
            }
        }

        if (m_tokenizer.TryReadNextVariant(*m_device, m_temp.Variant))
            content.InlineImageDictionary.AddKey(m_temp.Name, m_temp.Variant);
        else
            return false;
    }
}

// Returns false in case of errors
bool PdfContentsReader::tryFollowXObject(PdfContent& content)
{
    (void)content;
    return true;
}

// Returns false in case of EOF
bool PdfContentsReader::tryReadInlineImgData(PdfData& data)
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

void PdfContentsReader::handleWarnings()
{
    if ((m_args.Flags & PdfContentReaderFlags::ThrowOnWarnings) != PdfContentReaderFlags::None)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidContentStream, "Unsupported PostScript content");
}
