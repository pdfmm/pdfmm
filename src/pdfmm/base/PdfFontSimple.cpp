/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontSimple.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfFilter.h"
#include "PdfName.h"
#include "PdfObjectStream.h"

using namespace std;
using namespace mm;

PdfFontSimple::PdfFontSimple(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding)
    : PdfFont(doc, metrics, encoding), m_Descriptor(nullptr)
{
}

PdfFontSimple::PdfFontSimple(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding)
    : PdfFont(obj, metrics, encoding), m_Descriptor(nullptr)
{
}

void PdfFontSimple::getWidthsArray(PdfArray& arr) const
{
    vector<double> widths;
    for (unsigned code = GetEncoding().GetFirstChar().Code; code <= GetEncoding().GetLastChar().Code; code++)
    {
        // NOTE: In non CID-keyed fonts char codes are equivalent to CID
        widths.push_back(GetCIDWidthRaw(code));
    }

    arr.Clear();
    arr.resize(widths.size());

    auto matrix = m_Metrics->GetMatrix();
    for (unsigned i = 0; i < widths.size(); i++)
        arr.Add(PdfObject(static_cast<int64_t>(std::round(widths[i] / matrix[0]))));
}

void PdfFontSimple::getFontMatrixArray(PdfArray& fontMatrix) const
{
    fontMatrix.Clear();
    fontMatrix.Reserve(6);

    auto matrix = m_Metrics->GetMatrix();
    for (unsigned i = 0; i < 6; i++)
        fontMatrix.Add(PdfObject(matrix[i]));
}

void PdfFontSimple::Init()
{
    PdfName subType;
    switch (GetType())
    {
        case PdfFontType::Type1:
            subType = PdfName("Type1");
            break;
        case PdfFontType::TrueType:
            subType = PdfName("TrueType");
            break;
        case PdfFontType::Type3:
            subType = PdfName("Type3");
            break;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(subType));
    this->GetObject().GetDictionary().AddKey("BaseFont", PdfName(GetName()));
    m_Encoding->ExportToFont(*this);

    if (!GetMetrics().IsStandard14FontMetrics() || IsEmbeddingEnabled())
    {
        // Standard 14 fonts don't need any metrics descriptor
        // if the font is not embedded
        this->GetObject().GetDictionary().AddKey("FirstChar", PdfVariant(static_cast<int64_t>(m_Encoding->GetFirstChar().Code)));
        this->GetObject().GetDictionary().AddKey("LastChar", PdfVariant(static_cast<int64_t>(m_Encoding->GetLastChar().Code)));

        PdfArray arr;
        this->getWidthsArray(arr);

        auto widthsObj = GetDocument().GetObjects().CreateObject(arr);
        this->GetObject().GetDictionary().AddKeyIndirect("Widths", widthsObj);

        if (GetType() == PdfFontType::Type3)
        {
            getFontMatrixArray(arr);
            GetObject().GetDictionary().AddKey("FontMatrix", arr);

            GetBoundingBox(arr);
            GetObject().GetDictionary().AddKey("FontBBox", arr);
        }

        auto descriptorObj = GetDocument().GetObjects().CreateDictionaryObject("FontDescriptor");
        this->GetObject().GetDictionary().AddKeyIndirect("FontDescriptor", descriptorObj);
        FillDescriptor(descriptorObj->GetDictionary());
        m_Descriptor = descriptorObj;
    }
}

void PdfFontSimple::embedFont()
{
    if (m_Descriptor == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Descriptor must be initialized");

    embedFontFile(*m_Descriptor);
}

void PdfFontSimple::initImported()
{
    this->Init();
}

void PdfFontSimple::embedFontFile(PdfObject& descriptor)
{
    auto fontdata = m_Metrics->GetFontFileData();
    if (fontdata.empty())
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);

    PdfName fontFileName;
    nullable<unsigned> length1;
    PdfName subtype;
    switch (m_Metrics->GetFontFileType())
    {
        case PdfFontFileType::TrueType:
            fontFileName = PdfName("FontFile2");
            length1 = (unsigned)fontdata.size();
            break;
        case PdfFontFileType::OpenType:
            fontFileName = PdfName("FontFile3");
            subtype = PdfName("OpenType");
            break;
        case PdfFontFileType::Type1CCF:
            fontFileName = PdfName("FontFile3");
            subtype = PdfName("Type1C");
            break;
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported font type embedding");
    }

    auto contents = GetDocument().GetObjects().CreateDictionaryObject();
    descriptor.GetDictionary().AddKeyIndirect(fontFileName, contents);

    // NOTE: Set lengths before creating the stream as
    // PdfStreamedDocument does not allow adding keys
    // to an object after a stream was written
    if (length1 != nullptr)
        contents->GetDictionary().AddKey("Length1", PdfObject(static_cast<int64_t>(*length1)));

    if (!subtype.IsNull())
        contents->GetDictionary().AddKey(PdfName::KeySubtype, subtype);

    contents->GetOrCreateStream().Set(fontdata);
}
