/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfIdentityEncoding.h"

#include <sstream>
#include <stdexcept>

#include <utfcpp/utf8.h>

#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfFont.h"

using namespace std;
using namespace mm;

static PdfEncodingLimits getLimits(unsigned char codeSpaceSize);

PdfIdentityEncoding::PdfIdentityEncoding(unsigned char codeSpaceSize)
    : PdfIdentityEncoding(codeSpaceSize, PdfIdentityOrientation::Unkwnown) { }

PdfIdentityEncoding::PdfIdentityEncoding(unsigned char codeSpaceSize, PdfIdentityOrientation orientation)
    : m_Limits(getLimits(codeSpaceSize)), m_orientation(orientation) { }

PdfIdentityEncoding::PdfIdentityEncoding(PdfIdentityOrientation orientation)
    : PdfIdentityEncoding(2, orientation) { }

bool PdfIdentityEncoding::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    PDFMM_INVARIANT(m_Limits.MinCodeSize == m_Limits.MaxCodeSize);
    if (utls::GetCharCodeSize(codePoint) > m_Limits.MaxCodeSize)
    {
        codeUnit = { };
        return false;
    }

    codeUnit = { (unsigned)codePoint, m_Limits.MaxCodeSize };
    return true;
}

bool PdfIdentityEncoding::tryGetCodePoints(const PdfCharCode& codeUnit, vector<char32_t>& codePoints) const
{
    codePoints.push_back((char32_t)codeUnit.Code);
    return true;
}

void PdfIdentityEncoding::getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const
{
    (void)objects;
    (void)obj;

    switch (m_orientation)
    {
        case PdfIdentityOrientation::Horizontal:
            name = PdfName("Identity-H");
            break;
        case PdfIdentityOrientation::Vertical:
            name = PdfName("Identity-V");
            break;
        default:
            // TODO: Implement a custom CMap that has the correct
            // range for other identities
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported");
    }
}

void PdfIdentityEncoding::AppendToUnicodeEntries(PdfObjectStream& stream) const
{
    // Very easy, just do a single bfrange
    // Use PdfEncodingMap::AppendUTF16CodeTo
    (void)stream;
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NotImplemented, "TODO");
}

void PdfIdentityEncoding::AppendCIDMappingEntries(PdfObjectStream& stream, const PdfFont& font) const
{
    (void)stream;
    (void)font;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}

PdfEncodingMapType PdfIdentityEncoding::GetType() const
{
    // PdfIdentityEncoding represents either Identity-H/Identity-V
    // predefined CMap names or exports a custom CMap
    return PdfEncodingMapType::CMap;
}

const PdfEncodingLimits& PdfIdentityEncoding::GetLimits() const
{
    return m_Limits;
}

PdfEncodingLimits getLimits(unsigned char codeSpaceSize)
{
    if (codeSpaceSize == 0 || codeSpaceSize > 4)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Code space size can't be zero or bigger than 4");

    return { codeSpaceSize, codeSpaceSize, PdfCharCode(0, codeSpaceSize),
        PdfCharCode((unsigned)std::pow(2, codeSpaceSize * CHAR_BIT), codeSpaceSize) };
}
