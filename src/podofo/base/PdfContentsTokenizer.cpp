/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
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

#include "PdfContentsTokenizer.h"

#include "PdfCanvas.h"
#include "PdfInputDevice.h"
#include "PdfOutputStream.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"
#include "PdfData.h"
#include "PdfDictionary.h"
#include "PdfDefinesPrivate.h"

#include <iostream>
#include <list>

using namespace std;
using namespace PoDoFo;

/// <summary>
/// We found Pdfs spanning delimiters or begin/end tags into
/// streams. Let's create a device correctly spanning I/O
/// reads into these
/// </summary>
class PdfCanvasInputDevice : public PdfInputDevice
{
public:
    PdfCanvasInputDevice(PdfCanvas& canvas);
public:
    bool TryGetChar(int& ch) override;
    size_t Tell() override;
    int Look() override;
    size_t Read(char* buffer, size_t size) override;
    bool Eof() const override { return m_eof; }
    bool IsSeekable() const override { return false; }
private:
    bool tryGetNextDevice(PdfInputDevice*& device);
    void popNextDevice();
private:
    bool m_eof;
    std::list<PdfObject*> m_lstContents;
    std::unique_ptr<PdfInputDevice> m_device;
};

PdfContentsTokenizer::PdfContentsTokenizer(const std::shared_ptr<PdfInputDevice>& device)
    : m_device(device), m_readingInlineImgData(false)
{
}

PdfContentsTokenizer::PdfContentsTokenizer(PdfCanvas& canvas)
    : m_device(new PdfCanvasInputDevice(canvas)), m_readingInlineImgData(false)
{
}

bool PdfContentsTokenizer::tryReadInlineImgDict(PdfDictionary& dict)
{
    EPdfContentsType type;
    string_view keyword;
    PdfVariant variant;
    while (true)
    {
        if (!tryReadNext(type, keyword, variant))
            return false;

        PdfName key;
        switch (type)
        {
            case EPdfContentsType::Keyword:
            {
                // Try to find end of dictionary
                if (keyword == "ID")
                    return true;
                else
                    return false;
            }
            case EPdfContentsType::Variant:
            {
                const PdfName* name;
                if (variant.TryGetName(name))
                {
                    key = *name;
                    break;
                }
                else
                {
                    return false;
                }
            }
            case EPdfContentsType::Unknown:
            {
                return false;
            }
            case EPdfContentsType::ImageDictionary:
            case EPdfContentsType::ImageData:
            {
                throw runtime_error("Unsupported flow");
            }
        }

        if (TryReadNextVariant(variant))
            dict.AddKey(key, PdfObject(variant));
        else
            return false;
    }
}

void PdfContentsTokenizer::ReadNextVariant(PdfVariant& rVariant)
{
    if (!TryReadNextVariant(rVariant))
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected variant");
}

bool PdfContentsTokenizer::TryReadNextVariant(PdfVariant& rVariant)
{
    EPdfTokenType eTokenType;
    string_view pszToken;
    if (!PdfTokenizer::TryReadNextToken(*m_device, pszToken, &eTokenType))
        return false;

    return PdfTokenizer::TryReadNextVariant(*m_device, pszToken, eTokenType, rVariant, nullptr);
}

bool PdfContentsTokenizer::TryReadNext(EPdfContentsType& reType, string_view& rpszKeyword, PdfVariant& rVariant)
{
    if (m_readingInlineImgData)
    {
        rpszKeyword = { };
        PdfData data;
        if (tryReadInlineImgData(data))
        {
            rVariant = data;
            reType = EPdfContentsType::ImageData;
            m_readingInlineImgData = false;
            return true;
        }
        else
        {
            reType = EPdfContentsType::Unknown;
            m_readingInlineImgData = false;
            return false;
        }
    }

    if (!tryReadNext(reType, rpszKeyword, rVariant))
    {
        reType = EPdfContentsType::Unknown;
        return false;
    }

    if (reType == EPdfContentsType::Keyword && rpszKeyword == "BI")
    {
        PdfDictionary dict;
        if (tryReadInlineImgDict(dict))
        {
            rVariant = dict;
            reType = EPdfContentsType::ImageDictionary;
            m_readingInlineImgData = true;
            return true;
        }
        else
        {
            reType = EPdfContentsType::Unknown;
            return false;
        }
    }

    return true;
}

bool PdfContentsTokenizer::tryReadNext(EPdfContentsType& type, string_view& keyword, PdfVariant& variant)
{
    EPdfTokenType eTokenType;
    string_view pszToken;
    bool gotToken = PdfTokenizer::TryReadNextToken(*m_device, pszToken, &eTokenType);
    if (!gotToken)
    {
        type = EPdfContentsType::Unknown;
        return false;
    }

    EPdfLiteralDataType eDataType = DetermineDataType(*m_device, pszToken, eTokenType, variant);

    // asume we read a variant unless we discover otherwise later.
    type = EPdfContentsType::Variant;

    switch (eDataType)
    {
        case EPdfLiteralDataType::Null:
        case EPdfLiteralDataType::Bool:
        case EPdfLiteralDataType::Number:
        case EPdfLiteralDataType::Real:
            // the data was already read into rVariant by the DetermineDataType function
            break;

        case EPdfLiteralDataType::Reference:
        {
            // references are invalid in content streams
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "references are invalid in content streams");
            break;
        }

        case EPdfLiteralDataType::Dictionary:
            this->ReadDictionary(*m_device, variant, nullptr);
            break;
        case EPdfLiteralDataType::Array:
            this->ReadArray(*m_device, variant, nullptr);
            break;
        case EPdfLiteralDataType::String:
            this->ReadString(*m_device, variant, nullptr);
            break;
        case EPdfLiteralDataType::HexString:
            this->ReadHexString(*m_device, variant, nullptr);
            break;
        case EPdfLiteralDataType::Name:
            this->ReadName(*m_device, variant);
            break;
        default:
            // Assume we have a keyword
            type = EPdfContentsType::Keyword;
            keyword = pszToken;
            break;
    }

    return true;
}

bool PdfContentsTokenizer::tryReadInlineImgData(PdfData& data)
{
    auto& buffer = GetBuffer();
    if (buffer.GetBuffer() == nullptr || buffer.GetSize() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    int ch;

    // Consume one whitespace between ID and data
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
                if (IsWhitespace(ch))
                {
                    data = string_view(buffer.GetBuffer(), readCount - 2);
                    return true;
                }
                else
                    status = ReadEIStatus::ReadE;

                break;
            }
        }

        if (readCount == buffer.GetSize())
        {
            // image is larger than buffer => resize buffer
            buffer.Resize(buffer.GetSize() * 2);
        }

        buffer.GetBuffer()[readCount] = ch;
        readCount++;
    }

    return false;
}

PdfCanvasInputDevice::PdfCanvasInputDevice(PdfCanvas& canvas)
{
    PdfObject* contents = canvas.GetContents();
    if (contents == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "/Contents handle is null");

    if (contents->IsArray())
    {
        PdfArray& contentsArr = contents->GetArray();
        for (size_t i = 0; i < contentsArr.GetSize(); i++)
        {
            auto& streamObj = contentsArr.FindAt(i);
            m_lstContents.push_back(&streamObj);
        }
    }
    else if (contents->IsDictionary())
    {
        // NOTE: Pages are allowed to be empty
        if (contents->HasStream())
            m_lstContents.push_back(contents);
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Page /Contents not stream or array of streams");
    }

    if (m_lstContents.size() == 0)
    {
        m_eof = true;
    }
    else
    {
        popNextDevice();
        m_eof = m_device->Eof();
    }
}

bool PdfCanvasInputDevice::TryGetChar(int& ch)
{
    if (m_eof)
    {
        ch = 0;
        return false;
    }

    PdfInputDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            m_eof = true;
            return false;
        }

        if (device->TryGetChar(ch))
            return true;
    }
}

int PdfCanvasInputDevice::Look()
{
    if (m_eof)
        return EOF;

    PdfInputDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            m_eof = true;
            return EOF;
        }

        int ret = device->Look();
        if (ret != EOF)
            return ret;
    }
}

size_t PdfCanvasInputDevice::Read(char* buffer, size_t size)
{
    if (size == 0 || m_eof)
        return 0;

    size_t readCount = 0;
    PdfInputDevice* device = nullptr;
    while (true)
    {
        if (!tryGetNextDevice(device))
        {
            m_eof = true;
            return readCount;
        }

        // Span reads into multple input devices
        size_t readLocal = device->Read(buffer + readCount, size);
        size -= readLocal;
        readCount += readLocal;

        if (size == 0)
            return readCount;
    }
}

size_t PdfCanvasInputDevice::Tell()
{
    throw runtime_error("Unsupported");
}

bool PdfCanvasInputDevice::tryGetNextDevice(PdfInputDevice*& device)
{
    PODOFO_ASSERT(m_device != nullptr);
    if (device == nullptr)
    {
        device = m_device.get();
        return true;
    }

    if (m_lstContents.size() == 0)
    {
        device = nullptr;
        return false;
    }

    popNextDevice();
    device = m_device.get();
    return true;
}

void PdfCanvasInputDevice::popNextDevice()
{
    PdfStream& pStream = m_lstContents.front()->GetOrCreateStream();
    PdfRefCountedBuffer buffer;
    PdfBufferOutputStream stream(&buffer);
    pStream.GetFilteredCopy(&stream);

    // TODO: Optimize me, the following copy the buffer
    m_device = std::make_unique<PdfInputDevice>(buffer.GetBuffer(), buffer.GetSize());
    m_lstContents.pop_front();
}
