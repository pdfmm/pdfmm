/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfEncodingShim.h"
#include "PdfEncodingMapFactory.h"

using namespace std;
using namespace PoDoFo;

PdfEncodingShim::PdfEncodingShim(const PdfEncoding& encoding, PdfFont& font)
    : PdfEncoding(encoding), m_Font(&font)
{
}

PdfFont& PdfEncodingShim::GetFont() const
{
    return *m_Font;
}

// For the actual implementation, create the encoding map using the supplied char code map
PdfDynamicEncoding::PdfDynamicEncoding(const shared_ptr<PdfCharCodeMap>& map, PdfFont& font)
    : PdfEncoding(shared_ptr<PdfEncodingMap>(new PdfEncodingMapBase(map)), nullptr), m_font(&font)
{
}

PdfFont& PdfDynamicEncoding::GetFont() const
{
    PODOFO_ASSERT(m_font != nullptr);
    return *m_font;
}
