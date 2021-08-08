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
     * A common base class for built-in encodings which are
     * known by name.
     *
     *  - WinAnsiEncoding
     *  - MacRomanEncoding
     *  - MacExpertEncoding
     *  - StandardEncoding
     *  - SymbolEncoding
     *  - ZapfDingbatsEncoding
     *  - PdfDocEncoding (only use this for strings which are not printed
     *                    in the document. This is for meta data in the PDF).
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     *..\see PdfStandardEncoding
     *  \see PdfSymbolEncoding
     *  \see PdfZapfDingbatsEncoding
     *
     */
    // TODO: Move StandardEncoding, SymbolEncoding, ZapfDingbatsEncoding
    // PdfDocEncoding away from the hierarchy. They are not true PDF
    // built-in encodings
    class PDFMM_API PdfPredefinedEncoding : public PdfEncodingMapSimple
    {
    protected:
        /**
         *  Create a new simple PdfEncodingMap which uses 1 byte.
         *
         *  \param name the name of a standard PdfEncoding
         *
         *  As of now possible values are:
         *  - MacRomanEncoding
         *  - WinAnsiEncoding
         *  - MacExpertEncoding
         *
         *  \see PdfWinAnsiEncoding
         *  \see PdfMacRomanEncoding
         *  \see PdfMacExportEncoding
         */
        PdfPredefinedEncoding(const PdfName& name);

        /** Get the name of this encoding.
         *
         *  \returns the name of this encoding.
         */
        inline const PdfName& GetName() const { return m_name; }

    protected:
        void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;
        bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;
        bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

        /** Gets a table of 256 short values which are the
         *  big endian Unicode code points that are assigned
         *  to the 256 values of this encoding.
         *
         *  This table is used internally to convert an encoded
         *  string of this encoding to and from Unicode.
         *
         *  \returns an array of 256 big endian Unicode code points
         */
        virtual const char32_t* GetToUnicodeTable() const = 0;

    private:
        /** Initialize the internal table of mappings from Unicode code points
         *  to encoded byte values.
         */
        void InitEncodingTable();

    private:
        PdfName m_name;         // The name of the encoding
        std::unordered_map<char32_t, char> m_EncodingTable; // The helper table for conversions into this encoding
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
        void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;
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
     // FIX-ME: This shouldn't be a PdfPredefinedEncoding
    class PDFMM_API PdfDocEncoding final : public PdfPredefinedEncoding
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
     // FIX-ME: This shouldn't be a PdfPredefinedEncoding
    class PDFMM_API PdfStandardEncoding final : public PdfPredefinedEncoding
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
     // FIX-ME: This shouldn't be a PdfPredefinedEncoding
    class PDFMM_API PdfSymbolEncoding final : public PdfPredefinedEncoding
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
     // FIX-ME: This shouldn't be a PdfPredefinedEncoding
    class PDFMM_API PdfZapfDingbatsEncoding final : public PdfPredefinedEncoding
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
    // FIX-ME: This shouldn't be a PdfPredefinedEncoding
    class PDFMM_API PdfWin1250Encoding final : public PdfWinAnsiEncoding
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
     // FIX-ME: This shouldn't be a PdfPredefinedEncoding
    class PDFMM_API PdfIso88592Encoding final : public PdfWinAnsiEncoding
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
