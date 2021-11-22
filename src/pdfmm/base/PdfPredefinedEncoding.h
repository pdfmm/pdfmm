/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_SIMPLE_ENCODING_H
#define PDF_SIMPLE_ENCODING_H

#include "PdfEncodingMap.h"

namespace mm
{
    /**
     * A common base class for Pdf defined predefined encodings which are
     * known by name.
     *
     *  - WinAnsiEncoding
     *  - MacRomanEncoding
     *  - MacExpertEncoding
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     */
    class PDFMM_API PdfPredefinedEncoding : public PdfBuiltInEncoding
    {
    protected:
        PdfPredefinedEncoding(const PdfName& name);

    public:
        bool IsSimpleEncoding() const override;

    protected:
        void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;
    };

    /**
     * The WinAnsiEncoding is the default encoding in pdfmm for
     * contents on PDF pages.
     *
     * It is also called CP-1252 encoding.
     * This class may be used as base for derived encodings.
     *
     * \see PdfWin1250Encoding
     *
     * \see PdfFont::WinAnsiEncoding
     */
    class PDFMM_API PdfWinAnsiEncoding : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;
        friend class PdfWin1250Encoding;
        friend class PdfIso88592Encoding;

    private:
        PdfWinAnsiEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from WinAnsiEncoding to UTF16

    };

    /**
     * MacRomanEncoding 
     */
    class PDFMM_API PdfMacRomanEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfMacRomanEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from MacRomanEncoding to UTF16
    };

    /**
     * MacExpertEncoding
     */
    class PDFMM_API PdfMacExpertEncoding final : public PdfPredefinedEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfMacExpertEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from MacExpertEncoding to UTF16
    };

    /**
     * The PdfDocEncoding is the default encoding for
     * all strings in pdfmm which are data in the PDF
     * file.
     *
     * \see PdfFont::DocEncoding
     */
    class PDFMM_API PdfDocEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfDocEncoding();

    public:
        /** Check if the chars in the given utf-8 view are elegible for PdfDocEncofing conversion
         *
         * /param isPdfDocEncoding the given utf-8 string is coincident in PdfDocEncoding representation
         */
        static bool CheckValidUTF8ToPdfDocEcondingChars(const std::string_view& view, bool& isPdfDocEncodingEqual);
        static bool IsPdfDocEncodingCoincidentToUTF8(const std::string_view& view);
        static bool TryConvertUTF8ToPdfDocEncoding(const std::string_view& view, std::string& pdfdocencstr);
        static std::string ConvertUTF8ToPdfDocEncoding(const std::string_view& view);
        static std::string ConvertPdfDocEncodingToUTF8(const std::string_view& view, bool& isUTF8Equal);
        static void ConvertPdfDocEncodingToUTF8(const std::string_view& view, std::string& u8str, bool& isUTF8Equal);

    public:
        static const std::unordered_map<char32_t, char>& GetUTF8ToPdfEncodingMap();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from DocEncoding to unicode
    };

    /**
     * StandardEncoding
     */
    class PDFMM_API PdfStandardEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfStandardEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from StandardEncoding to UTF16
    };

    /**
     * Symbol Encoding
     */
    class PDFMM_API PdfSymbolEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfSymbolEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from SymbolEncoding to UTF16
    };

    /**
     * ZapfDingbats encoding
     */
    class PDFMM_API PdfZapfDingbatsEncoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfZapfDingbatsEncoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from ZapfDingbatsEncoding to UTF16
    };

    /**
     * WINDOWS-1250 encoding
     */
    class PDFMM_API PdfWin1250Encoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfWin1250Encoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from Win1250Encoding to UTF16
    };

    /**
     * ISO-8859-2 encoding
     */
    class PDFMM_API PdfIso88592Encoding final : public PdfBuiltInEncoding
    {
        friend class PdfEncodingMapFactory;

    private:
        PdfIso88592Encoding();

    protected:
        const char32_t* GetToUnicodeTable() const override;

    private:
        static const char32_t s_cEncoding[256]; // conversion table from Iso88592Encoding to UTF16
    };
}

#endif // PDF_SIMPLE_ENCODING_H
