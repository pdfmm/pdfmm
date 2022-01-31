/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_PRIVATE_H
#define PDF_ENCODING_PRIVATE_H

namespace mm
{
    // Known encoding IDs
    constexpr size_t NullEncodingId             = 0;
    constexpr size_t WinAnsiEncodingId          = 11;
    constexpr size_t MacRomanEncodingId         = 12;
    constexpr size_t MacExpertEncodingId        = 13;
    constexpr size_t StandardEncodingId         = 21;
    constexpr size_t SymbolEncodingId           = 22;
    constexpr size_t ZapfDingbatsEncodingId     = 23;
    constexpr size_t CustomEncodingStartId      = 101;

    /** Check if the chars in the given utf-8 view are elegible for PdfDocEncofing conversion
     *
     * /param isPdfDocEncoding the given utf-8 string is coincident in PdfDocEncoding representation
     */
    bool CheckValidUTF8ToPdfDocEcondingChars(const std::string_view& view, bool& isPdfDocEncodingEqual);
    bool IsPdfDocEncodingCoincidentToUTF8(const std::string_view& view);
    bool TryConvertUTF8ToPdfDocEncoding(const std::string_view& view, std::string& pdfdocencstr);
    std::string ConvertUTF8ToPdfDocEncoding(const std::string_view& view);
    std::string ConvertPdfDocEncodingToUTF8(const std::string_view& view, bool& isUTF8Equal);
    void ConvertPdfDocEncodingToUTF8(const std::string_view& view, std::string& u8str, bool& isUTF8Equal);
}

#endif // PDF_ENCODING_PRIVATE_H
