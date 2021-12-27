/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfXRefStreamParserObject.h"

#include "PdfParser.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObjectStream.h"
#include "PdfVariant.h"

using namespace std;
using namespace mm;

PdfXRefStreamParserObject::PdfXRefStreamParserObject(PdfDocument& document, PdfInputDevice& device,
    PdfXRefEntries& entries)
    : PdfParserObject(document, device), m_NextOffset(-1), m_entries(&entries)
{
}

void PdfXRefStreamParserObject::Parse()
{
    // Ignore the encryption in the XREF as the XREF stream must no be encrypted (see PDF Reference 3.4.7)
    this->ParseFile(nullptr);

    // Do some very basic error checking
    if (!this->GetDictionary().HasKey(PdfName::KeyType))
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
    }

    auto& obj = this->GetDictionary().MustFindKey(PdfName::KeyType);
    if (!obj.IsName() || obj.GetName() != "XRef")
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
    }

    if (!this->GetDictionary().HasKey(PdfName::KeySize)
        || !this->GetDictionary().HasKey("W"))
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
    }

    if (!this->HasStreamToParse())
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    if (this->GetDictionary().HasKey("Prev"))
    {
        m_NextOffset = static_cast<ssize_t>(this->GetDictionary().FindKeyAs<double>("Prev", 0));
    }
}

void PdfXRefStreamParserObject::ReadXRefTable()
{
    int64_t size = this->GetDictionary().FindKeyAs<int64_t>(PdfName::KeySize, 0);
    auto& arr = this->GetDictionary().MustFindKey("W");

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integers
    if (!arr.IsArray() || arr.GetArray().size() != 3)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
    }

    int64_t wArray[W_ARRAY_SIZE] = { 0, 0, 0 };
    for (unsigned i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (!arr.GetArray()[i].IsNumber())
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }

        wArray[i] = static_cast<int64_t>(arr.GetArray()[i].GetNumber());
    }

    vector<int64_t> indices;
    getIndices(indices, static_cast<int64_t>(size));

    parseStream(wArray, indices);
}

void PdfXRefStreamParserObject::parseStream(const int64_t wArray[W_ARRAY_SIZE], const std::vector<int64_t>& indices)
{
    for (int64_t lengthSum = 0, i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (wArray[i] < 0)
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoXRef,
                "Negative field length in XRef stream");
        }
        if (std::numeric_limits<int64_t>::max() - lengthSum < wArray[i])
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoXRef,
                "Invalid entry length in XRef stream");
        }
        else
        {
            lengthSum += wArray[i];
        }
    }

    const size_t entryLen = static_cast<size_t>(wArray[0] + wArray[1] + wArray[2]);

    unique_ptr<char[]> buffer;
    size_t bufferLen;
    this->GetOrCreateStream().GetFilteredCopy(buffer, bufferLen);

    std::vector<int64_t>::const_iterator it = indices.begin();
    char* cursor = buffer.get();
    while (it != indices.end())
    {
        int64_t firstObj = *it++;
        int64_t count = *it++;

        while (count > 0)
        {
            if ((size_t)(cursor - buffer.get()) >= bufferLen)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoXRef, "Invalid count in XRef stream");

            auto& entry = (*m_entries)[static_cast<unsigned>(firstObj)];
            if (firstObj >= 0 && firstObj < static_cast<int64_t>(m_entries->size())
                && !entry.Parsed)
            {
                readXRefStreamEntry(entry, cursor, wArray);
            }

            firstObj++;
            cursor += entryLen;
            count--;
        }
    }
}

void PdfXRefStreamParserObject::getIndices(std::vector<int64_t>& indices, int64_t size)
{
    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if (this->GetDictionary().HasKey("Index"))
    {
        auto& arr = *(this->GetDictionary().GetKey("Index"));
        if (!arr.IsArray())
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }

        for (auto index : arr.GetArray())
            indices.push_back(index.GetNumber());
    }
    else
    {
        // Default
        indices.push_back(static_cast<int64_t>(0));
        indices.push_back(size);
    }

    // indices must be a multiple of 2
    if (indices.size() % 2 != 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
}

void PdfXRefStreamParserObject::readXRefStreamEntry(PdfXRefEntry& entry, char* buffer, const int64_t wArray[W_ARRAY_SIZE])
{
    uint64_t entryRaw[W_ARRAY_SIZE];
    for (unsigned i = 0; i < W_ARRAY_SIZE; i++)
    {
        if (wArray[i] > W_MAX_BYTES)
        {
            PdfError::LogMessage(PdfLogSeverity::Error,
                "The XRef stream dictionary has an entry in /W of size {}. The maximum supported value is {}",
                wArray[i], W_MAX_BYTES);

            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidXRefStream);
        }

        entryRaw[i] = 0;
        for (int64_t z = W_MAX_BYTES - wArray[i]; z < W_MAX_BYTES; z++)
        {
            entryRaw[i] = (entryRaw[i] << 8) + static_cast<unsigned char>(*buffer);
            buffer++;
        }
    }

    entry.Parsed = true;

    // TABLE 3.15 Additional entries specific to a cross - reference stream dictionary
    // /W array: "If the first element is zero, the type field is not present, and it defaults to type 1"
    uint64_t type;
    if (wArray[0] == 0)
        type = 1;
    else
        type = entryRaw[0]; // nData[0] contains the type information of this entry

    switch (type)
    {
        // TABLE 3.16 Entries in a cross-reference stream
        case 0:
            // a free object
            entry.ObjectNumber = entryRaw[1];
            entry.Generation = (uint32_t)entryRaw[2];
            entry.Type = XRefEntryType::Free;
            break;
        case 1:
            // normal uncompressed object
            entry.Offset = entryRaw[1];
            entry.Generation = (uint32_t)entryRaw[2];
            entry.Type = XRefEntryType::InUse;
            break;
        case 2:
            // object that is part of an object stream
            entry.ObjectNumber = entryRaw[1]; // object number of the stream
            entry.Index = (uint32_t)entryRaw[2]; // index in the object stream
            entry.Type = XRefEntryType::Compressed;
            break;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidXRefType);
    }
}

bool PdfXRefStreamParserObject::TryGetPreviousOffset(size_t& previousOffset) const
{
    bool ret = m_NextOffset != -1;
    previousOffset = ret ? (size_t)m_NextOffset : 0;
    return ret;
}
