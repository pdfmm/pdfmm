/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfObjectStreamParser.h"

#include <algorithm>

#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfParserObject.h"
#include "PdfObjectStream.h"
#include "PdfIndirectObjectList.h"
#include "PdfStreamDevice.h"

using namespace std;
using namespace mm;

PdfObjectStreamParser::PdfObjectStreamParser(PdfParserObject& parser,
        PdfIndirectObjectList& objects, const shared_ptr<charbuff>& buffer)
    : m_Parser(&parser), m_Objects(&objects), m_buffer(buffer)
{
    if (buffer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
}

void PdfObjectStreamParser::Parse(const ObjectIdList& list)
{
    int64_t num = m_Parser->GetDictionary().FindKeyAs<int64_t>("N", 0);
    int64_t first = m_Parser->GetDictionary().FindKeyAs<int64_t>("First", 0);

    charbuff buffer;
    m_Parser->GetOrCreateStream().CopyTo(buffer);

    this->ReadObjectsFromStream(buffer.data(), buffer.size(), num, first, list);
    m_Parser = nullptr;
}

void PdfObjectStreamParser::ReadObjectsFromStream(char* buffer, size_t bufferLen, int64_t num, int64_t first, ObjectIdList const& list)
{
    SpanStreamDevice device(buffer, bufferLen);
    PdfTokenizer tokenizer(m_buffer);
    PdfVariant var;
    int i = 0;

    while (i < num)
    {
        int64_t objNo = tokenizer.ReadNextNumber(device);
        int64_t offset = tokenizer.ReadNextNumber(device);
        size_t pos = device.GetPosition();

        if (first >= std::numeric_limits<int64_t>::max() - offset)
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::BrokenFile,
                "Object position out of max limit");
        }

        // move to the position of the object in the stream
        device.Seek(static_cast<size_t>(first + offset));

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
            auto obj = new PdfObject(std::move(var));
            obj->SetIndirectReference(reference);
            m_Objects->PushObject(obj);
        }

        // move back to the position inside of the table of contents
        device.Seek(pos);

        i++;
    }
}
