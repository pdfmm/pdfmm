/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfIdentityEncoding.h"

#include <sstream>
#include <stdexcept>

#include <utfcpp/utf8.h>

#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfFont.h"

using namespace std;
using namespace PoDoFo;

static PdfEncodingLimits getLimits(unsigned char codeSpaceSize);
static string getSuffix(unsigned char codeSpaceSize);

PdfIdentityEncoding::PdfIdentityEncoding(unsigned char codeSpaceSize, PdfIdentityOrientation orientation)
    : PdfEncodingMap(getLimits(codeSpaceSize)), m_orientation(orientation)
{
}

bool PdfIdentityEncoding::tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const
{
    auto& limits = GetLimits();
    PODOFO_INVARIANT(limits.MinCodeSize == limits.MaxCodeSize);
    if (usr::GetCharCodeSize(codePoint) > limits.MaxCodeSize)
    {
        codeUnit = { };
        return false;
    }

    codeUnit = { (unsigned)codePoint, limits.MaxCodeSize };
    return true;
}

bool PdfIdentityEncoding::tryGetCodePoints(const PdfCharCode& codeUnit, vector<char32_t>& codePoints) const
{
    codePoints.push_back((char32_t)codeUnit.Code);
    return true;
}

void PdfIdentityEncoding::getExportObject(PdfVecObjects& objects, PdfName& name, PdfObject*& obj) const
{
    (void)objects;
    (void)obj;

    if (GetLimits().MinCodeSize != GetLimits().MaxCodeSize
        || GetLimits().MinCodeSize != 2)
    {
        // Default identities are 2 bytes only
        // TODO: Implement a CMap for other identities
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEnumValue, "Unsupported");
    }

    switch (m_orientation)
    {
        case PdfIdentityOrientation::Horizontal:
            name = PdfName("Identity-H");
            break;
        case PdfIdentityOrientation::Vertical:
            name = PdfName("Identity-V");
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEnumValue, "Unsupported");
    }
}

void PdfIdentityEncoding::appendBaseFontEntries(PdfStream& stream) const
{
    // Very easy, just do a single bfrange
    // Use PdfEncodingMap::AppendUTF16CodeTo
    PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "TODO");
}

PdfEncodingLimits getLimits(unsigned char codeSpaceSize)
{
    if (codeSpaceSize == 0 || codeSpaceSize > 4)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Code space size can't be zero or bigger than 4");

    return { codeSpaceSize, codeSpaceSize, PdfCharCode(0, codeSpaceSize),
        PdfCharCode((unsigned)std::pow(2, codeSpaceSize * CHAR_BIT), codeSpaceSize) };
}

string getSuffix(unsigned char codeSpaceSize)
{
    switch (codeSpaceSize)
    {
        case 1:
            return "1Byte";
        case 2:
            return "2Bytes";
        case 3:
            return "3Bytes";
        case 4:
            return "4Bytes";
        default:
            throw runtime_error("Unsupported");
    }
}
