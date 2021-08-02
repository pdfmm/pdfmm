/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfFontType1Base14.h"

#include "PdfDocument.h"
#include "base/PdfDictionary.h"
#include "base/PdfArray.h"
#include "PdfFontMetricsBase14.h"
#include "PdfFontFactoryBase14Data.h"

using namespace std;
using namespace PoDoFo;

PdfFontType1Base14::PdfFontType1Base14(PdfDocument& doc,
        PdfStd14FontType fontType,
        const PdfEncoding& encoding) :
    PdfFont(doc, PdfFontMetricsBase14::GetInstance(fontType), encoding),
    m_FontType(fontType)
{
}

PdfFontType1Base14::PdfFontType1Base14(PdfObject& obj,
        PdfStd14FontType fontType,
        const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding)
    : PdfFont(obj, metrics, encoding),
    m_FontType(fontType)
{
}

string_view PdfFontType1Base14::GetStandard14FontName(PdfStd14FontType stdFont)
{
    return ::GetStandard14FontName(stdFont);
}

bool PdfFontType1Base14::IsStandard14Font(const string_view& fontName, PdfStd14FontType& stdFont)
{
    return ::IsStandard14Font(fontName, stdFont);
}

PdfFontType PdfFontType1Base14::GetType() const
{
    return PdfFontType::Type1;
}

void PdfFontType1Base14::initImported()
{
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName("Type1"));
    this->GetObject().GetDictionary().AddKey("BaseFont", PdfName(GetBaseFont()));
    m_Encoding->ExportToDictionary(this->GetObject().GetDictionary());
}

bool PdfFontType1Base14::TryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    // All Std14 fonts use a charset which maps 1:1 to unicode codepoints
    // The only ligatures supported are just the ones that are also
    // Unicode code points. So, to map the CID to the gid, we just find the
    // the accordingly glyph id
    Std14CPToGIDMap::const_iterator found;
    auto& map = GetStd14CPToGIDMap(m_FontType);
    // NOTE: In base 14 fonts CID are equivalent to char codes
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

bool PdfFontType1Base14::TryMapGIDToCID(unsigned gid, unsigned& cid) const
{
    // Lookup the GID in the Standard14 fonts data, then encode back
    // the found code point to a CID
    unsigned size;
    auto data = GetStd14FontData(m_FontType, size);
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
