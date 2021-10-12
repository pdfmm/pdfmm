/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontFactory.h"

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncodingFactory.h"
#include "PdfFontObject.h"
#include "PdfFontCIDTrueType.h"
#include "PdfFontMetrics.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfFontMetricsObject.h"
#include "PdfFontType1.h"
#include "PdfFontType3.h"
#include "PdfFontStandard14.h"
#include "PdfFontTrueType.h"

using namespace std;
using namespace mm;

PdfFont* PdfFontFactory::CreateFontObject(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding, const PdfFontInitParams& params)
{
    PdfFontMetricsType type = metrics->GetType();
    auto font = createFontForType(doc, metrics, encoding, type, params.Subsetting);
    if (font != nullptr)
        font->InitImported(params.Embed, params.Subsetting);

    return font;
}

PdfFont* PdfFontFactory::createFontForType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
    const PdfEncoding& encoding, PdfFontMetricsType type, bool subsetting)
{
    PdfFont* font = nullptr;
    if (subsetting || encoding.HasCIDMapping())
    {
        switch (type)
        {
            case PdfFontMetricsType::TrueType:
                font = new PdfFontCIDTrueType(doc, metrics, encoding);
                break;
            case PdfFontMetricsType::Type1Pfa:
            case PdfFontMetricsType::Type1Pfb:
                // TODO
                //font = new PdfFontCIDType1(doc, metrics, encoding);
                //break;
            case PdfFontMetricsType::Type3:
            case PdfFontMetricsType::Unknown:
            case PdfFontMetricsType::Type1Standard14:
            default:
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Unsupported font at this context");
        }
    }
    else
    {
        switch (type)
        {
            case PdfFontMetricsType::TrueType:
                font = new PdfFontTrueType(doc, metrics, encoding);
                break;
            case PdfFontMetricsType::Type1Pfa:
            case PdfFontMetricsType::Type1Pfb:
                font = new PdfFontType1(doc, metrics, encoding);
                break;
            case PdfFontMetricsType::Type3:
                font = new PdfFontType3(doc, metrics, encoding);
                break;
            case PdfFontMetricsType::Type1Standard14:
            case PdfFontMetricsType::Unknown:
            default:
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Unsupported font at this context");
        }
    }

    return font;
}

PdfFont* PdfFontFactory::CreateFont(PdfObject& obj)
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
        PdfObject* pDescendantObj = dict.FindKey("DescendantFonts");

        if (pDescendantObj == nullptr)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Type0 Font: No DescendantFonts");

        PdfArray& descendants = pDescendantObj->GetArray();
        PdfObject* objFont = nullptr;
        PdfObject* objDescriptor = nullptr;
        if (descendants.size() != 0)
        {
            objFont = &descendants.FindAt(0);
            objDescriptor = objFont->GetDictionary().FindKey("FontDescriptor");
        }

        if (objFont != nullptr)
        {
            auto encoding = PdfEncodingFactory::CreateEncoding(obj);
            if (!encoding.IsNull())
            {
                auto metrics = shared_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(*objFont, objDescriptor));
                return new PdfFontObject(obj, metrics, encoding);
            }
        }
    }
    else if (subType == "Type1")
    {
        // TODO: Old documents do not have a FontDescriptor for 
        //       the 14 standard fonts. This suggestions is 
        //       deprecated now, but give us problems with old documents.
        auto objDescriptor = dict.FindKey("FontDescriptor");

        // Handle missing FontDescriptor for the 14 standard fonts
        if (objDescriptor == nullptr)
        {
            // Check if it's a PdfFontStandard14
            auto baseFont = obj.GetDictionary().FindKey("BaseFont");
            PdfStandard14FontType baseFontType;
            if (baseFont == nullptr
                || !PdfFontStandard14::IsStandard14Font(baseFont->GetName().GetString(), baseFontType))
            {
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "No BaseFont object found "
                    "by reference in given object");
            }

            shared_ptr<const PdfFontMetrics> metrics;
            if (dict.HasKey("Widths"))
                metrics = shared_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(obj));
            else
                metrics = PdfFontMetricsStandard14::GetInstance(baseFontType);

            if (metrics != nullptr)
            {
                // pEncoding may be undefined, found a valid pdf with
                //   20 0 obj
                //   <<
                //   /Type /Font
                //   /BaseFont /ZapfDingbats
                //   /Subtype /Type1
                //   >>
                //   endobj 
                // If pEncoding is null then
                // use StandardEncoding for Courier, Times, Helvetica font families
                // and special encodings for Symbol and ZapfDingbats
                PdfEncoding encoding = PdfEncodingFactory::CreateEncoding(obj);
                if (encoding.IsNull() && metrics->IsSymbol())
                {
                    if (baseFontType == PdfStandard14FontType::Symbol)
                        encoding = PdfEncodingFactory::CreateSymbolEncoding();
                    else if (baseFontType == PdfStandard14FontType::ZapfDingbats)
                        encoding = PdfEncodingFactory::CreateZapfDingbatsEncoding();
                    else
                        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Uncoregnized symbol encoding");
                }
                else
                {
                    encoding = PdfEncodingFactory::CreateStandardEncoding();
                }

                return new PdfFontStandard14(obj, baseFontType, metrics, encoding);
            }
        }

        auto encoding = PdfEncodingFactory::CreateEncoding(obj);
        if (!encoding.IsNull())
        {
            auto metrics = shared_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(obj, objDescriptor));
            return new PdfFontObject(obj, metrics, encoding);
        }
    }
    else if (subType == "Type3")
    {
        auto objDescriptor = obj.GetDictionary().FindKey("FontDescriptor");
        auto encoding = PdfEncodingFactory::CreateEncoding(obj);
        if (!encoding.IsNull())
        {
            auto metrics = shared_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(obj, objDescriptor));
            return new PdfFontObject(obj, metrics, encoding);
        }
    }
    else if (subType == "TrueType")
    {
        auto objDescriptor = obj.GetDictionary().FindKey("FontDescriptor");
        auto encoding = PdfEncodingFactory::CreateEncoding(obj);
        if (!encoding.IsNull())
        {
            auto metrics = shared_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(obj, objDescriptor));
            return new PdfFontObject(obj, metrics, encoding);
        }
    }

    return nullptr;
}

PdfFont* PdfFontFactory::CreateStandard14Font(PdfDocument& doc, PdfStandard14FontType baseFont,
    const PdfEncoding& encoding, const PdfFontInitParams& params)
{
    PdfFont* font = new PdfFontStandard14(doc, baseFont, encoding);
    if (font != nullptr)
        font->InitImported(params.Embed, params.Subsetting);

    return font;
}
