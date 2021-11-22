/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontStandard14.h"

#include <pdfmm/private/PdfStandard14FontsData.h>

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfArray.h"
#include "PdfFontMetricsStandard14.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace mm;

PdfFontStandard14::PdfFontStandard14(PdfDocument& doc,
        PdfStandard14FontType fontType,
        const PdfEncoding& encoding) :
    PdfFontSimple(doc, PdfFontMetricsStandard14::GetInstance(fontType), encoding),
    m_FontType(fontType)
{
}

PdfFontStandard14::PdfFontStandard14(PdfObject& obj,
        PdfStandard14FontType fontType,
        const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding)
    : PdfFontSimple(obj, metrics, encoding),
    m_FontType(fontType)
{
}

string_view PdfFontStandard14::GetStandard14FontName(PdfStandard14FontType stdFont)
{
    return ::GetStandard14FontName(stdFont);
}

bool PdfFontStandard14::IsStandard14Font(const string_view& fontName, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, true, stdFont);
}

bool PdfFontStandard14::IsStandard14Font(const string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, useAltNames, stdFont);
}

bool PdfFontStandard14::TryGetStandard14Font(const string_view& baseFontName, bool bold, bool italic,
    PdfStandard14FontType& stdFont)
{
    return TryGetStandard14Font(baseFontName, bold, italic, false, stdFont);
}

bool PdfFontStandard14::TryGetStandard14Font(const string_view& baseFontName, bool bold, bool italic,
    bool useAltNames, PdfStandard14FontType& stdFont)
{
    return ::TryGetStandard14Font(baseFontName, bold, italic, useAltNames, stdFont);
}

PdfFontType PdfFontStandard14::GetType() const
{
    return PdfFontType::Standard14;
}

PdfEncodingMapConstPtr PdfFontStandard14::GetStandard14FontEncodingMap(PdfStandard14FontType stdFont)
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
            return PdfEncodingMapFactory::StandardEncodingInstance();
        case PdfStandard14FontType::Symbol:
            return PdfEncodingMapFactory::SymbolEncodingInstance();
        case PdfStandard14FontType::ZapfDingbats:
            return PdfEncodingMapFactory::ZapfDingbatsEncodingInstance();
        case PdfStandard14FontType::Unknown:
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Invalid Standard14 font type");
    }
}

void PdfFontStandard14::initImported()
{
    Init(!IsEmbeddingEnabled());
}

bool PdfFontStandard14::TryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    // All Std14 fonts use a charset which maps 1:1 to unicode codepoints
    // The only ligatures supported are just the ones that are also
    // Unicode code points. So, to map the CID to the gid, we just find the
    // the accordingly glyph id
    Std14CPToGIDMap::const_iterator found;
    auto& map = GetStd14CPToGIDMap(m_FontType);
    // NOTE: In standard 14 fonts CID are equivalent to char codes
    char32_t mappedCodePoint = GetEncoding().GetCodePoint(cid);
    if (mappedCodePoint == U'\0'
        || mappedCodePoint >= 0xFFFF
        || (found = map.find((unsigned short)mappedCodePoint)) == map.end())
    {
        gid = { };
        return false;
    }

    gid = found->second;
    return true;
}

bool PdfFontStandard14::TryMapGIDToCID(unsigned gid, unsigned& cid) const
{
    // Lookup the GID in the Standard14 fonts data, then encode back
    // the found code point to a CID
    unsigned size;
    auto data = GetStd14FontChars(m_FontType, size);
    PdfCID fullCid;
    if (gid >= size
        || !GetEncoding().TryGetCID((char32_t)data[gid].CodePoint, fullCid))
    {
        gid = { };
        return false;
    }

    cid = fullCid.Id;
    return true;
}
