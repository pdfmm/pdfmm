/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfEncodingMapFactory.h"
#include "PdfPredefinedEncoding.h"
#include "PdfIdentityEncoding.h"
#include "PdfDifferenceEncoding.h"
#include "PdfFontType1Encoding.h"

using namespace std;
using namespace mm;

PdfEncodingMapConstPtr PdfEncodingMapFactory::PdfDocEncodingInstance()
{
    static shared_ptr<PdfDocEncoding> s_istance(new PdfDocEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::WinAnsiEncodingInstance()
{
    static shared_ptr<PdfWinAnsiEncoding> s_istance(new PdfWinAnsiEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::MacRomanEncodingInstance()
{
    static shared_ptr<PdfMacRomanEncoding> s_istance(new PdfMacRomanEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::StandardEncodingInstance()
{
    static shared_ptr<PdfStandardEncoding> s_istance(new PdfStandardEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::MacExpertEncodingInstance()
{
    static shared_ptr<PdfMacExpertEncoding> s_istance(new PdfMacExpertEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::SymbolEncodingInstance()
{
    static shared_ptr<PdfSymbolEncoding> s_istance(new PdfSymbolEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::ZapfDingbatsEncodingInstance()
{
    static shared_ptr<PdfZapfDingbatsEncoding> s_istance(new PdfZapfDingbatsEncoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::TwoBytesHorizontalIdentityEncodingInstance()
{
    static shared_ptr<PdfIdentityEncoding> s_istance(new PdfIdentityEncoding(PdfIdentityOrientation::Horizontal));
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::TwoBytesVerticalIdentityEncodingInstance()
{
    static shared_ptr<PdfIdentityEncoding> s_istance(new PdfIdentityEncoding(PdfIdentityOrientation::Vertical));
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::Win1250EncodingInstance()
{
    static shared_ptr<PdfWin1250Encoding> s_istance(new PdfWin1250Encoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::Iso88592EncodingInstance()
{
    static shared_ptr<PdfIso88592Encoding> s_istance(new PdfIso88592Encoding());
    return s_istance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetDummyEncodingMap()
{
    static PdfEncodingMapConstPtr s_instance(new PdfDummyEncodingMap());
    return s_instance;
}

PdfEncodingMapConstPtr PdfEncodingMapFactory::GetStandard14FontEncodingMap(PdfStandard14FontType stdFont)
{
    switch (stdFont)
    {
        case PdfStandard14FontType::TimesRoman:
        case PdfStandard14FontType::TimesItalic:
        case PdfStandard14FontType::TimesBold:
        case PdfStandard14FontType::TimesBoldItalic:
        case PdfStandard14FontType::Helvetica:
        case PdfStandard14FontType::HelveticaOblique:
        case PdfStandard14FontType::HelveticaBold:
        case PdfStandard14FontType::HelveticaBoldOblique:
        case PdfStandard14FontType::Courier:
        case PdfStandard14FontType::CourierOblique:
        case PdfStandard14FontType::CourierBold:
        case PdfStandard14FontType::CourierBoldOblique:
            return StandardEncodingInstance();
        case PdfStandard14FontType::Symbol:
            return SymbolEncodingInstance();
        case PdfStandard14FontType::ZapfDingbats:
            return ZapfDingbatsEncodingInstance();
        case PdfStandard14FontType::Unknown:
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Invalid Standard14 font type");
    }
}
