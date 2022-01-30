/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingFactory.h"

#include <pdfmm/private/PdfEncodingPrivate.h>

#include "PdfObject.h"
#include "PdfDictionary.h"
#include "PdfEncodingMapFactory.h"
#include "PdfIdentityEncoding.h"
#include "PdfDifferenceEncoding.h"
#include "PdfFontType1Encoding.h"
#include "PdfCMapEncoding.h"
#include "PdfEncodingShim.h"
#include "PdfFontMetrics.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace mm;

PdfEncoding PdfEncodingFactory::CreateEncoding(const PdfObject& fontObj, const PdfFontMetrics& metrics)
{
    // The /Encoding entry can be a predefined encoding, a CMap
    PdfEncodingMapConstPtr encoding;

    auto encodingObj = fontObj.GetDictionary().FindKey("Encoding");
    if (encodingObj != nullptr)
        encoding = createEncodingMap(*encodingObj, metrics);

    if (encoding == nullptr && metrics.GetFontFileType() == PdfFontFileType::Type1)
    {
        auto fontFileObj = metrics.GetFontFileObject();
        if (fontFileObj == nullptr)
        {
            // encoding may be undefined, found a valid pdf with
            //   20 0 obj
            //   <<
            //   /Type /Font
            //   /BaseFont /ZapfDingbats
            //   /Subtype /Type1
            //   >>
            //   endobj 
            // If encoding is null then
            // use StandardEncoding for Times, Helvetica, Courier font families
            // special encodings for Symbol and ZapfDingbats
            PdfStandard14FontType std14Font;
            if (metrics.IsStandard14FontMetrics(std14Font))
                encoding = PdfEncodingMapFactory::GetStandard14FontEncodingMap(std14Font);
            else if (metrics.IsType1Kind() && metrics.IsPdfNonSymbolic())
                encoding = PdfEncodingMapFactory::StandardEncodingInstance();
        }
        else
        {
            encoding = PdfFontType1Encoding::Create(*fontFileObj);
        }
    }

    // TODO: Implement full text extraction, including search in predefined
    // CMap(s) as described in Pdf Reference and here https://stackoverflow.com/a/26910569/213871

    // The /ToUnicode CMap is the main entry to search
    // for text extraction
    PdfEncodingMapConstPtr toUnicode;
    auto toUnicodeObj = fontObj.GetDictionary().FindKey("ToUnicode");
    if (toUnicodeObj != nullptr)
        toUnicode = createEncodingMap(*toUnicodeObj, metrics);

    if (encoding == nullptr)
    {
        if (toUnicode == nullptr)
        {
            // We don't have enough info to create an encoding and
            // we don't know how to read an built-in font encoding
            return PdfEncoding();
        }
        else
        {
            // As a fallback, create an identity encoding of the size size of the /ToUnicode mapping
            encoding = std::make_shared<PdfIdentityEncoding>(toUnicode->GetLimits().MaxCodeSize);
        }
    }

    return PdfEncoding(fontObj, encoding, toUnicode);
}

PdfEncodingMapConstPtr PdfEncodingFactory::createEncodingMap(
    const PdfObject& obj, const PdfFontMetrics& metrics)
{
    if (obj.IsName())
    {
        auto& name = obj.GetName();
        if (name == "WinAnsiEncoding")
            return PdfEncodingMapFactory::WinAnsiEncodingInstance();
        else if (name == "MacRomanEncoding")
            return PdfEncodingMapFactory::MacRomanEncodingInstance();
        else if (name == "MacExpertEncoding")
            return PdfEncodingMapFactory::MacExpertEncodingInstance();
        // CHECK-ME The following StandardEncoding, SymbolEncoding,
        // SymbolSetEncoding, ZapfDingbatsEncoding are not built-in
        // encodings in Pdf, or they are not mentioned at all
        else if (name == "StandardEncoding")
            return PdfEncodingMapFactory::StandardEncodingInstance();
        else if (name == "SymbolEncoding")
            return PdfEncodingMapFactory::SymbolEncodingInstance();
        else if (name == "SymbolSetEncoding")
            return PdfEncodingMapFactory::SymbolEncodingInstance();
        else if (name == "ZapfDingbatsEncoding")
            return PdfEncodingMapFactory::ZapfDingbatsEncodingInstance();
        // TABLE 5.15 Predefined CJK CMap names: the generip H-V identifies
        // are mappings for 2-byte CID. "It maps 2-byte character codes ranging
        // from 0 to 65,535 to the same 2 - byte CID value, interpreted high
        // order byte first"
        else if (name == "Identity-H")
            return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();
        else if (name == "Identity-V")
            return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
    }
    else if (obj.IsDictionary())
    {
        auto& dict = obj.GetDictionary();
        auto* cmapName = dict.FindKey("CMapName");
        if (cmapName != nullptr)
        {
            if (cmapName->GetName() == "Identity-H")
                return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();

            if (cmapName->GetName() == "Identity-V")
                return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
        }

        // CHECK-ME: should we better verify if it's a CMap first?
        if (obj.HasStream())
            return PdfCMapEncoding::Create(obj);

        // CHECK-ME: should we verify if it's a reference by searching /Differences?
        return PdfDifferenceEncoding::Create(obj, metrics);
    }

    return nullptr;
}

PdfEncoding PdfEncodingFactory::CreateWinAnsiEncoding()
{
    return PdfEncoding(WinAnsiEncodingId, PdfEncodingMapFactory::WinAnsiEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateMacRomanEncoding()
{
    return PdfEncoding(MacRomanEncodingId, PdfEncodingMapFactory::MacRomanEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateMacExpertEncoding()
{
    return PdfEncoding(MacExpertEncodingId, PdfEncodingMapFactory::MacExpertEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateStandardEncoding()
{
    return PdfEncoding(StandardEncodingId, PdfEncodingMapFactory::StandardEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreatePdfDocEncoding()
{
    return PdfEncoding(PdfDocEncodingId, PdfEncodingMapFactory::PdfDocEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateSymbolEncoding()
{
    return PdfEncoding(SymbolEncodingId, PdfEncodingMapFactory::SymbolEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateZapfDingbatsEncoding()
{
    return PdfEncoding(ZapfDingbatsEncodingId, PdfEncodingMapFactory::ZapfDingbatsEncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateWin1250Encoding()
{
    return PdfEncoding(WinAnsiEncodingId, PdfEncodingMapFactory::Win1250EncodingInstance());
}

PdfEncoding PdfEncodingFactory::CreateIso88592Encoding()
{
    return PdfEncoding(Iso88592EncodingId, PdfEncodingMapFactory::Iso88592EncodingInstance());
}
