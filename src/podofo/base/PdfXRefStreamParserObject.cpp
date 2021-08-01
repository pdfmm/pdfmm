/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfXRefStreamParserObject.h"

#include "PdfParser.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfStream.h"
#include "PdfVariant.h"

using namespace std;
using namespace PoDoFo;

PdfXRefStreamParserObject::PdfXRefStreamParserObject(PdfDocument& document, const PdfRefCountedInputDevice& rDevice,
    const PdfRefCountedBuffer& rBuffer, TVecEntries& entries)
    : PdfParserObject(document, rDevice, rBuffer), m_lNextOffset(-1), m_entries(&entries)
{
}

void PdfXRefStreamParserObject::Parse()
{
    // Ignore the encryption in the XREF as the XREF stream must no be encrypted (see PDF Reference 3.4.7)
    this->ParseFile(nullptr);

    // Do some very basic error checking
    if (!this->GetDictionary().HasKey(PdfName::KeyType))
    {
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }

    PdfObject* pObj = this->GetDictionary().GetKey(PdfName::KeyType);
    if (!pObj->IsName() || (pObj->GetName() != "XRef"))
    {
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }

    if (!this->GetDictionary().HasKey(PdfName::KeySize)
        || !this->GetDictionary().HasKey("W"))
    {
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }

    if (!this->HasStreamToParse())
    {
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }

    if (this->GetDictionary().HasKey("Prev"))
    {
        m_lNextOffset = static_cast<ssize_t>(this->GetDictionary().FindKeyAs<double>("Prev", 0));
    }
}

void PdfXRefStreamParserObject::ReadXRefTable()
{
    int64_t lSize = this->GetDictionary().FindKeyAs<int64_t>(PdfName::KeySize, 0);
    auto& arr = *(this->GetDictionary().GetKey("W"));

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integers
    if (!arr.IsArray() || arr.GetArray().size() != 3)
    {
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }

    int64_t nW[W_ARRAY_SIZE] = { 0, 0, 0 };
    for (unsigned i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (!arr.GetArray()[i].IsNumber())
        {
            PODOFO_RAISE_ERROR(EPdfError::NoXRef);
        }

        nW[i] = static_cast<int64_t>(arr.GetArray()[i].GetNumber());
    }

    vector<int64_t> vecIndeces;
    GetIndeces(vecIndeces, static_cast<int64_t>(lSize));

    ParseStream(nW, vecIndeces);
}

void PdfXRefStreamParserObject::ParseStream(const int64_t nW[W_ARRAY_SIZE], const std::vector<int64_t>& rvecIndeces)
{
    for (int64_t nLengthSum = 0, i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (nW[i] < 0)
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::NoXRef,
                "Negative field length in XRef stream");
        }
        if (std::numeric_limits<int64_t>::max() - nLengthSum < nW[i])
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::NoXRef,
                "Invalid entry length in XRef stream");
        }
        else
        {
            nLengthSum += nW[i];
        }
    }

    const size_t entryLen = static_cast<size_t>(nW[0] + nW[1] + nW[2]);

    unique_ptr<char> buffer;
    size_t lBufferLen;
    this->GetOrCreateStream().GetFilteredCopy(buffer, lBufferLen);

    std::vector<int64_t>::const_iterator it = rvecIndeces.begin();
    char* pBuffer = buffer.get();
    while (it != rvecIndeces.end())
    {
        int64_t nFirstObj = *it++;
        int64_t nCount = *it++;

        while (nCount > 0)
        {
            if ((size_t)(pBuffer - buffer.get()) >= lBufferLen)
                PODOFO_RAISE_ERROR_INFO(EPdfError::NoXRef, "Invalid count in XRef stream");

            if (nFirstObj >= 0 && nFirstObj < static_cast<int64_t>(m_entries->size())
                && !(*m_entries)[static_cast<int>(nFirstObj)].Parsed)
            {
                ReadXRefStreamEntry(pBuffer, lBufferLen, nW, static_cast<int>(nFirstObj));
            }

            nFirstObj++;
            pBuffer += entryLen;
            --nCount;
        }
    }
}

void PdfXRefStreamParserObject::GetIndeces(std::vector<int64_t>& rvecIndeces, int64_t size)
{
    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if (this->GetDictionary().HasKey("Index"))
    {
        auto& arr = *(this->GetDictionary().GetKey("Index"));
        if (!arr.IsArray())
        {
            PODOFO_RAISE_ERROR(EPdfError::NoXRef);
        }

        for (auto index : arr.GetArray())
            rvecIndeces.push_back(index.GetNumber());
    }
    else
    {
        // Default
        rvecIndeces.push_back(static_cast<int64_t>(0));
        rvecIndeces.push_back(size);
    }

    // vecIndeces must be a multiple of 2
    if (rvecIndeces.size() % 2 != 0)
    {
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }
}

void PdfXRefStreamParserObject::ReadXRefStreamEntry(char* pBuffer, size_t, const int64_t lW[W_ARRAY_SIZE], int nObjNo)
{
    uint64_t nData[W_ARRAY_SIZE];
    for (unsigned i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (lW[i] > W_MAX_BYTES)
        {
            PdfError::LogMessage(LogSeverity::Error,
                "The XRef stream dictionary has an entry in /W of size %i.\nThe maximum supported value is %i.",
                lW[i], W_MAX_BYTES);

            PODOFO_RAISE_ERROR(EPdfError::InvalidXRefStream);
        }

        nData[i] = 0;
        for (int64_t z = W_MAX_BYTES - lW[i]; z < W_MAX_BYTES; z++)
        {
            nData[i] = (nData[i] << 8) + static_cast<unsigned char>(*pBuffer);
            ++pBuffer;
        }
    }

    PdfXRefEntry& entry = (*m_entries)[nObjNo];
    entry.Parsed = true;

    // TABLE 3.15 Additional entries specific to a cross - reference stream dictionary
    // /W array: "If the first element is zero, the type field is not present, and it defaults to type 1"
    uint64_t type;
    if (lW[0] == 0)
        type = 1;
    else
        type = nData[0]; // nData[0] contains the type information of this entry

    switch (type)
    {
        // TABLE 3.16 Entries in a cross-reference stream
        case 0:
            // a free object
            entry.ObjectNumber = nData[1];
            entry.Generation = (uint32_t)nData[2];
            entry.Type = EXRefEntryType::Free;
            break;
        case 1:
            // normal uncompressed object
            entry.Offset = nData[1];
            entry.Generation = (uint32_t)nData[2];
            entry.Type = EXRefEntryType::InUse;
            break;
        case 2:
            // object that is part of an object stream
            entry.ObjectNumber = nData[1]; // object number of the stream
            entry.Index = (uint32_t)nData[2]; // index in the object stream
            entry.Type = EXRefEntryType::Compressed;
            break;
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidXRefType);
    }
}

bool PdfXRefStreamParserObject::TryGetPreviousOffset(size_t &previousOffset) const
{
    bool ret = m_lNextOffset != -1;
    previousOffset = ret ? (size_t)m_lNextOffset : 0;
    return ret;
}
