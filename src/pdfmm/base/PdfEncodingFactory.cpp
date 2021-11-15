/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
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

using namespace std;
using namespace mm;

PdfEncoding PdfEncodingFactory::CreateEncoding(PdfObject& fontObj)
{
    auto subTypeKey = fontObj.GetDictionary().GetKey(PdfName::KeySubtype);
    if (subTypeKey == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "Font: No SubType");

    bool explicitNames = false;
    auto& subType = subTypeKey->GetName();
    if (subType == "Type3")
        explicitNames = true;

    // The /Encoding entry can be a predefined encoding, a CMap
    PdfEncodingMapConstPtr encoding;
    auto objEncoding = fontObj.GetDictionary().FindKey("Encoding");
    if (objEncoding != nullptr)
        encoding = createEncodingMap(*objEncoding, explicitNames);;

    if (encoding == nullptr)
    {
        PdfObject* objDescriptor;
        if ((objDescriptor = fontObj.GetDictionary().FindKey("FontDescriptor")) != nullptr)
        {
            auto fontFileObj = objDescriptor->GetDictionary().FindKey("FontFile");
            if (fontFileObj == nullptr)
            {
                // Let's try to determine if its a symbolic font by reading the FontDescriptor Flags
                // Flags & 4 --> Symbolic, Flags & 32 --> Nonsymbolic
                int32_t flags = static_cast<int32_t>(objDescriptor->GetDictionary().FindKeyAs<double>("Flags", 0));
                if (flags & 32) // Nonsymbolic, otherwise pEncoding remains nullptr
                    encoding = PdfEncodingMapFactory::StandardEncodingInstance();
            }
            else
            {
                encoding.reset(new PdfFontType1Encoding(*fontFileObj));
            }
        }
    }

    // TODO: Implement full text extraction, including search in predefined
    // CMap(s) as described in Pdf Reference and here https://stackoverflow.com/a/26910569/213871

    // The /ToUnicode CMap is the main entry to search
    // for text extraction
    PdfEncodingMapConstPtr toUnicode;
    auto objToUnicode = fontObj.GetDictionary().FindKey("ToUnicode");
    if (objToUnicode != nullptr)
        toUnicode = createEncodingMap(*objToUnicode, explicitNames);

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

PdfEncodingMapConstPtr PdfEncodingFactory::createEncodingMap(PdfObject& obj, bool explicitNames)
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
        PdfDictionary& dict = obj.GetDictionary();
        PdfObject* cmapName = dict.FindKey("CMapName");
        if (cmapName != nullptr)
        {
            if (cmapName->GetName() == "Identity-H")
                return PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance();

            if (cmapName->GetName() == "Identity-V")
                return PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance();
        }

        // CHECK-ME: should we better verify if it's a CMap first?
        if (obj.HasStream())
            return std::make_shared<PdfCMapEncoding>(obj);

        // CHECK-ME: should we verify if it's a reference by searching /Differences?
        return std::make_shared<PdfDifferenceEncoding>(obj, explicitNames);
    }

    return nullptr;
}


PdfEncoding PdfEncodingFactory::CreateDynamicEncoding()
{
    return PdfEncoding(DynamicEncodingId, PdfEncodingMapFactory::GetDummyEncodingMap());
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
