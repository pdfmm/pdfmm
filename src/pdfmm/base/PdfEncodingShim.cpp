/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfEncodingShim.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace mm;

namespace mm
{
    class PdfDynamicEncodingMap : public PdfEncodingMapBase
    {
    public:
        PdfDynamicEncodingMap(const shared_ptr<PdfCharCodeMap>& map);
    };
}

PdfEncodingShim::PdfEncodingShim(const PdfEncoding& encoding, PdfFont& font)
    : PdfEncoding(encoding), m_Font(&font)
{
}

PdfFont& PdfEncodingShim::GetFont() const
{
    return *m_Font;
}

// For the actual implementation, create the encoding map using the supplied char code map
PdfDynamicEncoding::PdfDynamicEncoding(const shared_ptr<PdfCharCodeMap>& cidMap,
    const shared_ptr<PdfCharCodeMap>& toUnicodeMap, PdfFont& font)
    : PdfEncoding(GetNextId(), PdfEncodingMapConstPtr(new PdfDynamicEncodingMap(cidMap)),
        PdfEncodingMapConstPtr(new PdfDynamicEncodingMap(toUnicodeMap))), m_font(&font)
{
}

bool PdfDynamicEncoding::IsDynamicEncoding() const
{
    return true;
}

PdfFont& PdfDynamicEncoding::GetFont() const
{
    PDFMM_ASSERT(m_font != nullptr);
    return *m_font;
}

PdfDynamicEncodingMap::PdfDynamicEncodingMap(const shared_ptr<PdfCharCodeMap>& map)
    : PdfEncodingMapBase(map, PdfEncodingMapType::CMap) { }
