/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfObjectStreamParser.h"

#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfIndirectObjectList.h"

#include <algorithm>

#ifndef VERBOSE_DEBUG_DISABLED
#include <iostream>
#endif

using namespace std;
using namespace mm;

PdfObjectStreamParser::PdfObjectStreamParser(PdfParserObject& parser,
        PdfIndirectObjectList& objects, const shared_ptr<chars>& buffer)
    : m_Parser(&parser), m_Objects(&objects), m_buffer(buffer)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

void PdfObjectStreamParser::Parse(ObjectIdList const& list)
{
    int64_t num = m_Parser->GetDictionary().FindKeyAs<int64_t>("N", 0);
    int64_t first = m_Parser->GetDictionary().FindKeyAs<int64_t>("First", 0);

    unique_ptr<char> buffer;
    size_t bufferLen;
    m_Parser->GetOrCreateStream().GetFilteredCopy(buffer, bufferLen);

    this->ReadObjectsFromStream(buffer.get(), bufferLen, num, first, list);
    m_Parser = nullptr;
}

void PdfObjectStreamParser::ReadObjectsFromStream(char* buffer, size_t bufferLen, int64_t num, int64_t first, ObjectIdList const& list)
{
    PdfInputDevice device(buffer, bufferLen);
    PdfTokenizer tokenizer(m_buffer);
    PdfVariant var;
    int i = 0;

    while (static_cast<int64_t>(i) < num)
    {
        const int64_t objNo = tokenizer.ReadNextNumber(device);
        const int64_t offset = tokenizer.ReadNextNumber(device);
        size_t pos = device.Tell();

        if (first >= std::numeric_limits<int64_t>::max() - offset)
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object position out of max limit");
        }

        // move to the position of the object in the stream
        device.Seek(static_cast<std::streamoff>(first + offset));

        // use a second tokenizer here so that anything that gets dequeued isn't left in the tokenizer that reads the offsets and lengths
        PdfTokenizer variantTokenizer(m_buffer);
        variantTokenizer.ReadNextVariant(device, var); // NOTE: The stream is already decrypted

        bool shouldRead = std::find(list.begin(), list.end(), objNo) != list.end();
#ifndef VERBOSE_DEBUG_DISABLED
        std::cerr << "ReadObjectsFromStream STREAM=" << m_Parser->GetIndirectReference().ToString() <<
            ", OBJ=" << objNo <<
            ", " << (shouldRead ? "read" : "skipped") << std::endl;
#endif
        if (shouldRead)
        {
            // The generation number of an object stream and of any
            // compressed object is implicitly zero
            PdfReference reference(static_cast<uint32_t>(objNo), 0);
            auto obj = new PdfObject(var);
            m_Objects->PushObject(reference, obj);
        }

        // move back to the position inside of the table of contents
        device.Seek(pos);

        i++;
    }
}
