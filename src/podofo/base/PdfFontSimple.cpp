/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfFontSimple.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfName.h"
#include "PdfStream.h"

using namespace std;
using namespace PoDoFo;

PdfFontSimple::PdfFontSimple(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding)
    : PdfFont(doc, metrics, encoding), m_Descriptor(nullptr)
{
}

void PdfFontSimple::getWidthsArray(PdfArray& arr) const
{
    vector<double> widths;
    for (unsigned code = GetEncoding().GetFirstChar().Code; code <= GetEncoding().GetLastChar().Code; code++)
    {
        // NOTE: In non CID-keyed fonts char codes are equivalent to CID
        widths.push_back(GetCIDWidthRaw({ code }));
    }

    arr.clear();
    arr.reserve(widths.size());
    // TODO: Handle custom measure units, like for /Type3 fonts
    for (unsigned i = 0; i < widths.size(); i++)
        arr.Add(PdfObject(static_cast<int64_t>(std::round(widths[i] * 1000))));
}

void PdfFontSimple::Init(const string_view& subType, bool skipMetricsDescriptors)
{
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(subType));
    this->GetObject().GetDictionary().AddKey("BaseFont", PdfName(GetBaseFont()));
    m_Encoding->ExportToDictionary(this->GetObject().GetDictionary());

    if (!skipMetricsDescriptors)
    {
        // Standard 14 fonts don't need any metrics descriptor
        this->GetObject().GetDictionary().AddKey("FirstChar", PdfVariant(static_cast<int64_t>(m_Encoding->GetFirstChar().Code)));
        this->GetObject().GetDictionary().AddKey("LastChar", PdfVariant(static_cast<int64_t>(m_Encoding->GetLastChar().Code)));

        PdfArray widths;
        this->getWidthsArray(widths);

        auto widthsObj = this->GetObject().GetDocument()->GetObjects().CreateObject(widths);
        this->GetObject().GetDictionary().AddKeyIndirect("Widths", widthsObj);

        auto descriptorObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("FontDescriptor");
        this->GetObject().GetDictionary().AddKeyIndirect("FontDescriptor", descriptorObj);
        FillDescriptor(descriptorObj->GetDictionary());
        m_Descriptor = descriptorObj;
    }
}

void PdfFontSimple::embedFont()
{
    this->embedFontFile(*m_Descriptor);
}
