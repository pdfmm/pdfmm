/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfFontTrueType.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfStream.h"

using namespace PoDoFo;

PdfFontTrueType::PdfFontTrueType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding) :
    PdfFontSimple(doc, metrics, encoding)
{
}

PdfFontType PdfFontTrueType::GetType() const
{
    return PdfFontType::TrueType;
}

void PdfFontTrueType::initImported()
{
    this->Init("TrueType");
}

void PdfFontTrueType::embedFontFile(PdfObject& descriptor)
{
    PdfObject* pContents;
    size_t size = 0;

    pContents = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
    descriptor.GetDictionary().AddKey("FontFile2", pContents->GetIndirectReference());

    // if the data was loaded from memory - use it from there
    // otherwise, load from disk
    if (m_Metrics->GetFontData().empty())
    {
        size = io::FileSize(m_Metrics->GetFilename());
        PdfFileInputStream stream(m_Metrics->GetFilename());

        // NOTE: Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow
        // adding keys to an object after a stream was written
        pContents->GetDictionary().AddKey("Length1", PdfVariant(static_cast<int64_t>(size)));
        pContents->GetOrCreateStream().Set(stream);
    }
    else
    {
        const char* buffer = m_Metrics->GetFontData().data();
        size = m_Metrics->GetFontData().size();

        // Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow
        // adding keys to an object after a stream was written
        pContents->GetDictionary().AddKey("Length1", PdfVariant(static_cast<int64_t>(size)));
        pContents->GetOrCreateStream().Set(buffer, size);
    }
}
