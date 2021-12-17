#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfContentsReader.h"

#include "PdfXObjectForm.h"
#include "PdfOperatorUtils.h"
#include "PdfCanvasInputDevice.h"
#include "PdfData.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfContentsReader::PdfContentsReader(const PdfCanvas& canvas,
        nullable<const PdfContentReaderArgs&> args) :
    PdfContentsReader(std::make_shared<PdfCanvasInputDevice>(canvas),
        &canvas, args) { }

PdfContentsReader::PdfContentsReader(const shared_ptr<PdfInputDevice>& device,
        nullable<const PdfContentReaderArgs&> args) :
    PdfContentsReader(device, nullptr, args) { }

PdfContentsReader::PdfContentsReader(const shared_ptr<PdfInputDevice>& device,
    const PdfCanvas* canvas, nullable<const PdfContentReaderArgs&> args) :
    m_args(args.has_value() ? *args : PdfContentReaderArgs()),
    m_buffer(std::make_shared<chars>(PdfTokenizer::BufferSize)),
    m_tokenizer(m_buffer),
    m_readingInlineImgData(false),
    m_temp{ }
{
    if (device == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Device must be non null");

    m_inputs.push_back({ nullptr, device, canvas });
}

bool PdfContentsReader::TryReadNext(PdfContent& content)
{
    beforeReadReset(content);

    while (true)
    {
        if (m_inputs.size() == 0)
            goto Eof;

        if (m_readingInlineImgData)
        {
            if (m_args.InlineImageHandler == nullptr)
            {
                if (!tryReadInlineImgData(content.InlineImageData))
                    goto PopDevice;

                content.Type = PdfContentType::ImageData;
                m_readingInlineImgData = false;
                afterReadClear(content);
                return true;
            }
            else
            {
                bool eof = !m_args.InlineImageHandler(content.InlineImageDictionary, *m_inputs.back().Device);
                m_readingInlineImgData = false;

                // Try to consume the EI end image operator
                if (eof || !tryReadNextContent(content))
                {
                    content.Warnings = PdfContentWarnings::MissingEndImage;
                    goto PopDevice;
                }

                if (content.Operator != PdfOperator::EI)
                {
                    content.Warnings = PdfContentWarnings::MissingEndImage;
                    goto HandleContent;
                }

                beforeReadReset(content);
            }
        }

        if (!tryReadNextContent(content))
            goto PopDevice;

    HandleContent:
        afterReadClear(content);
        handleWarnings();
        return true;

    PopDevice:
        PDFMM_INVARIANT(m_inputs.size() != 0);
        m_inputs.pop_back();
        if (m_inputs.size() == 0)
            goto Eof;

        // Unless the device stack is empty, popping a devices
        // means that we finished processing an XObject form
        content.Type = PdfContentType::EndXObjectForm;
        if (content.Stack.GetSize() != 0)
            content.Warnings |= PdfContentWarnings::SpuriousStackContent;

        goto HandleContent;

    Eof:
        content.Type = PdfContentType::Unknown;
        afterReadClear(content);
        return false;
    }
}

// Returns false in case of EOF
bool PdfContentsReader::tryReadNextContent(PdfContent& content)
{
    while (true)
    {
        bool gotToken = m_tokenizer.TryReadNext(*m_inputs.back().Device, m_temp.PsType, content.Keyword, m_temp.Variant);
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
                        content.Warnings |= PdfContentWarnings::SpuriousStackContent;
                }

                if (!tryHandleOperator(content))
                    return false;

                return true;
            }
            case PdfPostScriptTokenType::Variant:
            {
                content.Stack.Push(std::move(m_temp.Variant));
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

void PdfContentsReader::beforeReadReset(PdfContent& content)
{
    content.Stack.Clear();
    content.Warnings = PdfContentWarnings::None;
}

void PdfContentsReader::afterReadClear(PdfContent& content)
{
    // Do some cleaning
    switch (content.Type)
    {
        case PdfContentType::Operator:
        {
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            break;
        }
        case PdfContentType::ImageDictionary:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            break;
        }
        case PdfContentType::ImageData:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary.Clear();
            content.XObject = nullptr;
            break;
        }
        case PdfContentType::DoXObject:
        {
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            break;
        }
        case PdfContentType::EndXObjectForm:
        case PdfContentType::Unknown:
        {
            // Full reset in case of unknown content.
            // Used when it is reached the EOF
            content.Operator = PdfOperator::Unknown;
            content.Keyword = string_view();
            content.InlineImageDictionary.Clear();
            content.InlineImageData.clear();
            content.XObject = nullptr;
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unsupported flow");
        }
    }
}

// Returns false in case of EOF
bool PdfContentsReader::tryHandleOperator(PdfContent& content)
{
    // By default it's not handled
    switch (content.Operator)
    {
        case PdfOperator::Do:
        {
            if (m_inputs.back().Canvas == nullptr || (m_args.Flags & PdfContentReaderFlags::DontFollowXObjects)
                != PdfContentReaderFlags::None)
            {
                // Don't follow XObject
                return true;
            }

            tryFollowXObject(content);
            return true;
        }
        case PdfOperator::BI:
        {
            if (!tryReadInlineImgDict(content))
                return false;

            content.Type = PdfContentType::ImageDictionary;
            m_readingInlineImgData = true;
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
        if (!m_tokenizer.TryReadNext(*m_inputs.back().Device, m_temp.PsType, m_temp.Keyword, m_temp.Variant))
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

        if (m_tokenizer.TryReadNextVariant(*m_inputs.back().Device, m_temp.Variant))
            content.InlineImageDictionary.AddKey(m_temp.Name, std::move(m_temp.Variant));
        else
            return false;
    }
}

// Returns false in case of errors
void PdfContentsReader::tryFollowXObject(PdfContent& content)
{
    PDFMM_ASSERT(m_inputs.back().Canvas != nullptr);
    PdfName xobjName;
    const PdfResources* resources;
    const PdfObject* xobjraw = nullptr;
    unique_ptr<PdfXObject> xobj;
    if (content.Stack.GetSize() != 1
        || !content.Stack[0].TryGetName(xobjName)
        || (resources = m_inputs.back().Canvas->GetResources()) == nullptr
        || (xobjraw = resources->GetFromResources("XObject", xobjName)) == nullptr
        || !PdfXObject::TryCreateFromObject(const_cast<PdfObject&>(*xobjraw), xobj))
    {
        content.Warnings |= PdfContentWarnings::InvalidXObject;
        return;
    }

    if (isCalledRecursively(xobjraw))
    {
        content.Warnings |= PdfContentWarnings::RecursiveXObject;
        return;
    }

    content.XObject.reset(xobj.release());
    content.Type = PdfContentType::DoXObject;

    if (content.XObject->GetType() == PdfXObjectType::Form)
    {
        m_inputs.push_back({
            content.XObject,
            std::make_shared<PdfCanvasInputDevice>(static_cast<const PdfXObjectForm&>(*content.XObject)),
            dynamic_cast<const PdfCanvas*>(content.XObject.get()) });
    }
}

// Returns false in case of EOF
bool PdfContentsReader::tryReadInlineImgData(chars& data)
{
    // Consume one whitespace between ID and data
    char ch;
    if (!m_inputs.back().Device->TryGetChar(ch))
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
    while (m_inputs.back().Device->TryGetChar(ch))
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

bool PdfContentsReader::isCalledRecursively(const PdfObject* xobj)
{
    // Determines if the given object is called recursively
    for (auto& input : m_inputs)
    {
        if (input.Canvas->GetContentsObject() == xobj)
            return true;
    }

    return false;
}
