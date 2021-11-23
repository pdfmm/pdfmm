/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncodingFactory.h"
#include "PdfFont.h"
#include "PdfFontObject.h"
#include "PdfFontCIDTrueType.h"
#include "PdfFontMetrics.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontMetricsObject.h"
#include "PdfFontType1.h"
#include "PdfFontType3.h"
#include "PdfFontTrueType.h"

using namespace std;
using namespace mm;

unique_ptr<PdfFont> PdfFont::Create(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding, PdfFontInitOptions flags)
{
    PdfFontFileType type = metrics->GetFontFileType();
    bool embeddingEnabled = (flags & PdfFontInitOptions::Embed) != PdfFontInitOptions::None;
    bool subsettingEnabled = (flags & PdfFontInitOptions::Subset) != PdfFontInitOptions::None;
    auto font(createFontForType(doc, metrics, encoding, type));
    if (font != nullptr)
        font->InitImported(embeddingEnabled, subsettingEnabled);

    return font;
}

unique_ptr<PdfFont> PdfFont::createFontForType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding, PdfFontFileType type)
{
    PdfFont* font = nullptr;
    if (encoding.IsCMapEncoding())
    {
        switch (type)
        {
            case PdfFontFileType::TrueType:
            case PdfFontFileType::OpenType:
                font = new PdfFontCIDTrueType(doc, metrics, encoding);
                break;
            case PdfFontFileType::Type1:
                // TODO
                //font = new PdfFontCIDType1(doc, metrics, encoding);
                //break;
            case PdfFontFileType::Type1CCF:
            case PdfFontFileType::CIDType1CCF:
            case PdfFontFileType::Type3:
            default:
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Unsupported font at this context");
        }
    }
    else
    {
        switch (type)
        {
            case PdfFontFileType::TrueType:
            case PdfFontFileType::OpenType:
                font = new PdfFontTrueType(doc, metrics, encoding);
                break;
            case PdfFontFileType::Type1:
            case PdfFontFileType::Type1CCF:
                font = new PdfFontType1(doc, metrics, encoding);
                break;
            case PdfFontFileType::Type3:
                font = new PdfFontType3(doc, metrics, encoding);
                break;
            case PdfFontFileType::CIDType1CCF:
            case PdfFontFileType::Unknown:
            default:
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Unsupported font at this context");
        }
    }

    return unique_ptr<PdfFont>(font);
}

bool PdfFont::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfFont>& font)
{
    auto& dict = obj.GetDictionary();
    PdfObject* objTypeKey = dict.FindKey(PdfName::KeyType);
    if (objTypeKey == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Font: No Type");

    if (objTypeKey->GetName() != "Font")
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);

    auto subTypeKey = dict.FindKey(PdfName::KeySubtype);
    if (subTypeKey == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Font: No SubType");

    auto& subType = subTypeKey->GetName();
    if (subType == "Type0")
    {
        // TABLE 5.18 Entries in a Type 0 font dictionary

        // The PDF reference states that DescendantFonts must be an array,
        // some applications (e.g. MS Word) put the array into an indirect object though.
        auto descendantObj = dict.FindKey("DescendantFonts");
        if (descendantObj == nullptr)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Type0 Font: No DescendantFonts");

        auto& descendants = descendantObj->GetArray();
        PdfObject* objFont = nullptr;
        PdfObject* objDescriptor = nullptr;
        if (descendants.size() != 0)
        {
            objFont = &descendants.FindAt(0);
            objDescriptor = objFont->GetDictionary().FindKey("FontDescriptor");
        }

        if (objFont != nullptr)
        {
            PdfFontMetricsConstPtr metrics = PdfFontMetricsObject::Create(*objFont, objDescriptor);
            auto encoding = PdfEncodingFactory::CreateEncoding(obj, *metrics);
            if (encoding.IsNull())
                goto Exit;

            font.reset(new PdfFontObject(obj, metrics, encoding));
            return true;
        }
    }
    else if (subType == "Type1")
    {
        auto objDescriptor = dict.FindKey("FontDescriptor");

        // Handle missing FontDescriptor for the 14 standard fonts
        if (objDescriptor == nullptr)
        {
            // Check if it's a PdfFontStandard14
            auto baseFont = dict.FindKey("BaseFont");
            PdfStandard14FontType stdFontType;
            if (baseFont == nullptr
                || !PdfFont::IsStandard14Font(baseFont->GetName().GetString(), stdFontType))
            {
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "No known /BaseFont found");
            }

            PdfFontMetricsConstPtr metrics = PdfFontMetricsStandard14::Create(stdFontType, obj);
            PdfEncoding encoding = PdfEncodingFactory::CreateEncoding(obj, *metrics);
            if (encoding.IsNull())
                goto Exit;

            font.reset(new PdfFontObject(obj, metrics, encoding));
            return true;
        }

        PdfFontMetricsConstPtr metrics = PdfFontMetricsObject::Create(obj, objDescriptor);
        auto encoding = PdfEncodingFactory::CreateEncoding(obj, *metrics);
        if (encoding.IsNull())
            goto Exit;

        font.reset(new PdfFontObject(obj, metrics, encoding));
        return true;
    }
    else if (subType == "Type3")
    {
        auto objDescriptor = dict.FindKey("FontDescriptor");
        PdfFontMetricsConstPtr metrics = PdfFontMetricsObject::Create(obj, objDescriptor);
        auto encoding = PdfEncodingFactory::CreateEncoding(obj, *metrics);
        if (encoding.IsNull())
            goto Exit;

        font.reset(new PdfFontObject(obj, metrics, encoding));
        return true;
    }
    else if (subType == "TrueType")
    {
        auto objDescriptor = dict.FindKey("FontDescriptor");
        PdfFontMetricsConstPtr metrics = PdfFontMetricsObject::Create(obj, objDescriptor);
        auto encoding = PdfEncodingFactory::CreateEncoding(obj, *metrics);
        if (encoding.IsNull())
            goto Exit;

        font.reset(new PdfFontObject(obj, metrics, encoding));
        return true;
    }

Exit:
    font.reset();
    return false;
}

unique_ptr<PdfFont> PdfFont::CreateStandard14(PdfDocument& doc, PdfStandard14FontType std14Font,
    const PdfEncoding& encoding, PdfFontInitOptions flags)
{
    (void)flags;
    bool embeddingEnabled = (flags & PdfFontInitOptions::Embed) != PdfFontInitOptions::None;
    //bool subsettingEnabled = (flags & PdfFontInitOptions::Subset) != PdfFontInitOptions::None;
    bool subsettingEnabled = false; // TODO
    PdfFontMetricsConstPtr metrics = PdfFontMetricsStandard14::Create(std14Font);
    unique_ptr<PdfFont> font(new PdfFontType1(doc, metrics, encoding));
    if (font != nullptr)
        font->InitImported(embeddingEnabled, subsettingEnabled);

    return font;
}
