/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfParserObject.h"

#include <doc/PdfDocument.h>
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfInputStream.h"
#include "PdfParser.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <iostream>
#include <sstream>

using namespace PoDoFo;
using namespace std;

PdfParserObject::PdfParserObject(PdfDocument& document, const PdfRefCountedInputDevice& device,
    const PdfRefCountedBuffer& buffer, ssize_t lOffset)
    : PdfObject(PdfVariant::NullValue, true), m_device(device), m_buffer(buffer), m_pEncrypt(nullptr)
{
    // Parsed objects by definition are initially not dirty
    resetDirty();
    SetDocument(document);
    InitPdfParserObject();
    m_lOffset = lOffset < 0 ? m_device.Device()->Tell() : lOffset;
}

PdfParserObject::PdfParserObject(const PdfRefCountedBuffer& buffer)
    : PdfObject(PdfVariant::NullValue, true), m_buffer(buffer), m_pEncrypt(nullptr)
{
    InitPdfParserObject();
    m_lOffset = -1;
}

void PdfParserObject::InitPdfParserObject()
{
    m_bIsTrailer = false;

    // Whether or not demand loading is disabled we still don't load
    // anything in the ctor. This just controls whether ::ParseFile(...)
    // forces an immediate demand load, or lets it genuinely happen
    // on demand.
    m_bLoadOnDemand = false;

    // We rely heavily on the demand loading infrastructure whether or not
    // we *actually* delay loading.
    EnableDelayedLoading();
    EnableDelayedLoadingStream();

    m_bStream = false;
    m_lStreamOffset = 0;
}

void PdfParserObject::ReadObjectNumber()
{
    PdfReference ref;
    try
    {
        int64_t obj = m_tokenizer.ReadNextNumber(*m_device.Device());
        int64_t gen = m_tokenizer.ReadNextNumber(*m_device.Device());

        ref = PdfReference(static_cast<uint32_t>(obj), static_cast<uint16_t>(gen));
        SetIndirectReference(ref);
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "Object and generation number cannot be read.");
        throw e;
    }

    if (!m_tokenizer.IsNextToken(*m_device.Device(), "obj"))
    {
        std::ostringstream oss;
        oss << "Error while reading object " << ref.ObjectNumber() << " "
            << ref.GenerationNumber() << ": Next token is not 'obj'." << std::endl;
        PODOFO_RAISE_ERROR_INFO(EPdfError::NoObject, oss.str().c_str());
    }
}

void PdfParserObject::ParseFile(PdfEncrypt* pEncrypt, bool bIsTrailer)
{
    if (!m_device.Device())
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    if (m_lOffset >= 0)
        m_device.Device()->Seek(m_lOffset);

    if (!bIsTrailer)
        ReadObjectNumber();

#ifndef VERBOSE_DEBUG_DISABLED
    std::cerr << "Parsing object number: " << m_reference.ObjectNumber()
              << " " << m_reference.GenerationNumber() << " obj"
              << " " << m_lOffset << " offset"
              << " (DL: " << (m_bLoadOnDemand ? "on" : "off") << ")"
              << endl;
#endif // VERBOSE_DEBUG_DISABLED

    m_lOffset = m_device.Device()->Tell();
    m_pEncrypt = pEncrypt;
    m_bIsTrailer = bIsTrailer;

    if (!m_bLoadOnDemand)
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
void PdfParserObject::ParseFileComplete(bool bIsTrailer)
{
    m_device.Device()->Seek(m_lOffset);
    if (m_pEncrypt)
        m_pEncrypt->SetCurrentReference(GetIndirectReference());

    // Do not call ReadNextVariant directly,
    // but TryReadNextToken, to handle empty objects like:
    // 13 0 obj
    // endobj

    EPdfTokenType eTokenType;
    string_view pszToken;
    bool gotToken = m_tokenizer.TryReadNextToken(*m_device.Device(), pszToken, eTokenType);
    if (!gotToken)
        PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected variant.");

    // Check if we have an empty object or data
    if (pszToken != "endobj")
    {
        m_tokenizer.ReadNextVariant(*m_device.Device(), pszToken, eTokenType, m_Variant, m_pEncrypt);

        if (!bIsTrailer)
        {
            bool gotToken = m_tokenizer.TryReadNextToken(*m_device.Device(), pszToken);
            if (!gotToken)
            {
                PODOFO_RAISE_ERROR_INFO(EPdfError::UnexpectedEOF, "Expected 'endobj' or (if dict) 'stream', got EOF.");
            }
            if (pszToken == "endobj")
            {
                // nothing to do, just validate that the PDF is correct
                // If it's a dictionary, it might have a stream, so check for that
            }
            else if (m_Variant.IsDictionary() && pszToken == "stream")
            {
                m_bStream = true;
                m_lStreamOffset = m_device.Device()->Tell(); // NOTE: whitespace after "stream" handle in stream parser!
            }
            else
            {
                PODOFO_RAISE_ERROR_INFO(EPdfError::NoObject, pszToken.data());
            }
        }
    }
}


// Only called during delayed loading. Must be careful to avoid
// triggering recursive delay loading due to use of accessors of
// PdfVariant or PdfObject.
void PdfParserObject::ParseStream()
{
    PODOFO_ASSERT(DelayedLoadDone());

    int64_t len = -1;
    int c;

    if (m_device.Device() == nullptr || GetDocument() == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    auto& lengthObj = this->m_Variant.GetDictionary().MustFindKey(PdfName::KeyLength);
    if (!lengthObj.TryGetNumber(len))
        PODOFO_RAISE_ERROR(EPdfError::InvalidStreamLength);

    m_device.Device()->Seek(m_lStreamOffset);

    streamoff streamOffset;
    while (true)
    {
        c = m_device.Device()->Look();
        switch (c)
        {
            // Skip spaces between the stream keyword and the carriage return/line
            // feed or line feed. Actually, this is not required by PDF Reference,
            // but certain PDFs have additionals whitespaces
            case ' ':
            case '\t':
                (void)m_device.Device()->GetChar();
                break;
            // From PDF 32000:2008 7.3.8.1 General
            // "The keyword stream that follows the stream dictionary shall be
            // followed by an end-of-line marker consisting of either a CARRIAGE
            // RETURN and a LINE FEED or just a LINE FEED, and not by a CARRIAGE
            // RETURN alone"
            case '\r':
                streamOffset = m_device.Device()->Tell();
                (void)m_device.Device()->GetChar();
                c = m_device.Device()->Look();
                if (c == '\n')
                {
                    (void)m_device.Device()->GetChar();
                    streamOffset = m_device.Device()->Tell();
                }
                goto ReadStream;
            case '\n':
                (void)m_device.Device()->GetChar();
                streamOffset = m_device.Device()->Tell();
                goto ReadStream;
            // Assume malformed PDF with no whitespaces after the stream keyword
            default:
                streamOffset = m_device.Device()->Tell();
                goto ReadStream;
        }
    }

ReadStream:
    m_device.Device()->Seek(streamOffset);	// reset it before reading!
    PdfDeviceInputStream reader(m_device.Device());

    if (m_pEncrypt && !m_pEncrypt->IsMetadataEncrypted())
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
                    m_pEncrypt = 0;
            }
        }
    }

    // Set stream raw data without marking the object dirty
    if (m_pEncrypt != nullptr)
    {
        m_pEncrypt->SetCurrentReference(GetIndirectReference());
        auto pInput = m_pEncrypt->CreateEncryptionInputStream(reader, static_cast<size_t>(len));
        getOrCreateStream().SetRawData(*pInput, static_cast<ssize_t>(len), false);
    }
    else
    {
        getOrCreateStream().SetRawData(reader, static_cast<ssize_t>(len), false);
    }
}

void PdfParserObject::DelayedLoadImpl()
{
    ParseFileComplete(m_bIsTrailer);
}

void PdfParserObject::DelayedLoadStreamImpl()
{
    PODOFO_ASSERT(getStream() == nullptr);

    // Note: we can't use HasStream() here because it'll call DelayedLoad()
    if (HasStreamToParse())
    {
        try
        {
            ParseStream();
        }
        catch (PdfError& e)
        {
            std::ostringstream s;
            s << "Unable to parse the stream for object " << GetIndirectReference().ObjectNumber() << ' '
                << GetIndirectReference().GenerationNumber() << " obj .";
            e.AddToCallstack(__FILE__, __LINE__, s.str().c_str());
            throw;
        }
    }
}

void PdfParserObject::FreeObjectMemory(bool bForce)
{
    if (this->IsLoadOnDemand() && (bForce || !this->IsDirty()))
    {
        Clear();
        FreeStream();
        EnableDelayedLoading();
        EnableDelayedLoadingStream();
    }
}
