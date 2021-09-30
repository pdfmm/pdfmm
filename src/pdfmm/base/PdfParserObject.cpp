/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfParserObject.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfInputStream.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <iostream>

using namespace mm;
using namespace std;

PdfParserObject::PdfParserObject(PdfDocument& document, const shared_ptr<PdfInputDevice>& device,
        ssize_t offset)
    : PdfObject(PdfVariant::NullValue, true), m_device(device), m_Encrypt(nullptr)
{
    if (device == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    // Parsed objects by definition are initially not dirty
    resetDirty();
    SetDocument(document);
    InitPdfParserObject();
    m_Offset = offset < 0 ? device->Tell() : offset;
}

PdfParserObject::PdfParserObject()
    : PdfObject(PdfVariant::NullValue, true), m_Encrypt(nullptr)
{
    InitPdfParserObject();
    m_Offset = -1;
}

void PdfParserObject::InitPdfParserObject()
{
    m_IsTrailer = false;

    // Whether or not demand loading is disabled we still don't load
    // anything in the ctor. This just controls whether ::ParseFile(...)
    // forces an immediate demand load, or lets it genuinely happen
    // on demand.
    m_LoadOnDemand = false;

    // We rely heavily on the demand loading infrastructure whether or not
    // we *actually* delay loading.
    EnableDelayedLoading();
    EnableDelayedLoadingStream();

    m_HasStream = false;
    m_StreamOffset = 0;
}

void PdfParserObject::ReadObjectNumber()
{
    PdfReference ref;
    try
    {
        int64_t obj = m_tokenizer.ReadNextNumber(*m_device);
        int64_t gen = m_tokenizer.ReadNextNumber(*m_device);

        ref = PdfReference(static_cast<uint32_t>(obj), static_cast<uint16_t>(gen));
        SetIndirectReference(ref);
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME_INFO(e, "Object and generation number cannot be read");
        throw e;
    }

    if (!m_tokenizer.IsNextToken(*m_device, "obj"))
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Error while reading object {} {} R: Next token is not 'obj'",
            ref.ObjectNumber(), ref.GenerationNumber());
    }
}

void PdfParserObject::ParseFile(PdfEncrypt* encrypt, bool isTrailer)
{
    if (m_Offset >= 0)
        m_device->Seek(m_Offset);

    if (!isTrailer)
        ReadObjectNumber();

#ifndef VERBOSE_DEBUG_DISABLED
    std::cerr << "Parsing object number: " << m_reference.ObjectNumber()
              << " " << m_reference.GenerationNumber() << " obj"
              << " " << m_Offset << " offset"
              << " (DL: " << (m_LoadOnDemand ? "on" : "off") << ")"
              << endl;
#endif // VERBOSE_DEBUG_DISABLED

    m_Offset = m_device->Tell();
    m_Encrypt = encrypt;
    m_IsTrailer = isTrailer;

    if (!m_LoadOnDemand)
    {
        // Force immediate loading of the object.  We need to do this through
        // the deferred loading machinery to avoid getting the object into an
        // inconsistent state.
        // We can't do a full DelayedStreamLoad() because the stream might use
        // an indirect /Length or /Length1 key that hasn't been read yet.
        DelayedLoad();

        // TODO: support immediate loading of the stream here too. For that, we need
        // to be able to trigger the reading of not-yet-parsed indirect objects
        // such as might appear in a /Length key with an indirect reference.
    }
}

void PdfParserObject::ForceStreamParse()
{
    // It's really just a call to DelayedLoad
    DelayedLoadStream();
}

// Only called via the demand loading mechanism
// Be very careful to avoid recursive demand loads via PdfVariant
// or PdfObject method calls here.
void PdfParserObject::ParseFileComplete(bool isTrailer)
{
    m_device->Seek(m_Offset);
    if (m_Encrypt != nullptr)
        m_Encrypt->SetCurrentReference(GetIndirectReference());

    // Do not call ReadNextVariant directly,
    // but TryReadNextToken, to handle empty objects like:
    // 13 0 obj
    // endobj

    PdfTokenType tokenType;
    string_view token;
    bool gotToken = m_tokenizer.TryReadNextToken(*m_device, token, tokenType);
    if (!gotToken)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected variant");

    // Check if we have an empty object or data
    if (token != "endobj")
    {
        m_tokenizer.ReadNextVariant(*m_device, token, tokenType, m_Variant, m_Encrypt);

        if (!isTrailer)
        {
            bool gotToken = m_tokenizer.TryReadNextToken(*m_device, token);
            if (!gotToken)
            {
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnexpectedEOF, "Expected 'endobj' or (if dict) 'stream', got EOF");
            }
            if (token == "endobj")
            {
                // nothing to do, just validate that the PDF is correct
                // If it's a dictionary, it might have a stream, so check for that
            }
            else if (m_Variant.IsDictionary() && token == "stream")
            {
                m_HasStream = true;
                m_StreamOffset = m_device->Tell(); // NOTE: whitespace after "stream" handle in stream parser!
            }
            else
            {
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, token);
            }
        }
    }
}


// Only called during delayed loading. Must be careful to avoid
// triggering recursive delay loading due to use of accessors of
// PdfVariant or PdfObject.
void PdfParserObject::ParseStream()
{
    PDFMM_ASSERT(DelayedLoadDone());

    int64_t len = -1;
    int c;

    if (GetDocument() == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto& lengthObj = this->m_Variant.GetDictionary().MustFindKey(PdfName::KeyLength);
    if (!lengthObj.TryGetNumber(len))
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidStreamLength);

    m_device->Seek(m_StreamOffset);

    streamoff streamOffset;
    while (true)
    {
        c = m_device->Look();
        switch (c)
        {
            // Skip spaces between the stream keyword and the carriage return/line
            // feed or line feed. Actually, this is not required by PDF Reference,
            // but certain PDFs have additionals whitespaces
            case ' ':
            case '\t':
                (void)m_device->GetChar();
                break;
            // From PDF 32000:2008 7.3.8.1 General
            // "The keyword stream that follows the stream dictionary shall be
            // followed by an end-of-line marker consisting of either a CARRIAGE
            // RETURN and a LINE FEED or just a LINE FEED, and not by a CARRIAGE
            // RETURN alone"
            case '\r':
                streamOffset = m_device->Tell();
                (void)m_device->GetChar();
                c = m_device->Look();
                if (c == '\n')
                {
                    (void)m_device->GetChar();
                    streamOffset = m_device->Tell();
                }
                goto ReadStream;
            case '\n':
                (void)m_device->GetChar();
                streamOffset = m_device->Tell();
                goto ReadStream;
            // Assume malformed PDF with no whitespaces after the stream keyword
            default:
                streamOffset = m_device->Tell();
                goto ReadStream;
        }
    }

ReadStream:
    m_device->Seek(streamOffset);	// reset it before reading!
    PdfDeviceInputStream reader(*m_device);

    if (m_Encrypt != nullptr && !m_Encrypt->IsMetadataEncrypted())
    {
        // If metadata is not encrypted the Filter is set to "Crypt"
        auto filterObj = this->m_Variant.GetDictionary().FindKey(PdfName::KeyFilter);
        if (filterObj != nullptr && filterObj->IsArray())
        {
            auto& filters = filterObj->GetArray();
            for (unsigned i = 0; i < filters.GetSize(); i++)
            {
                auto& obj = filters.FindAt(i);
                if (obj.IsName() && obj.GetName() == "Crypt")
                    m_Encrypt = nullptr;
            }
        }
    }

    // Set stream raw data without marking the object dirty
    if (m_Encrypt != nullptr)
    {
        m_Encrypt->SetCurrentReference(GetIndirectReference());
        auto input = m_Encrypt->CreateEncryptionInputStream(reader, static_cast<size_t>(len));
        getOrCreateStream().SetRawData(*input, static_cast<ssize_t>(len), false);
    }
    else
    {
        getOrCreateStream().SetRawData(reader, static_cast<ssize_t>(len), false);
    }
}

void PdfParserObject::DelayedLoadImpl()
{
    ParseFileComplete(m_IsTrailer);
}

void PdfParserObject::DelayedLoadStreamImpl()
{
    PDFMM_ASSERT(getStream() == nullptr);

    // Note: we can't use HasStream() here because it'll call DelayedLoad()
    if (HasStreamToParse())
    {
        try
        {
            ParseStream();
        }
        catch (PdfError& e)
        {
            PDFMM_PUSH_FRAME_INFO(e, "Unable to parse the stream for object {} {} R",
                GetIndirectReference().ObjectNumber(),
                GetIndirectReference().GenerationNumber());
            throw;
        }
    }
}

void PdfParserObject::FreeObjectMemory(bool force)
{
    if (this->IsLoadOnDemand() && (force || !this->IsDirty()))
    {
        Clear();
        FreeStream();
        EnableDelayedLoading();
        EnableDelayedLoadingStream();
    }
}
