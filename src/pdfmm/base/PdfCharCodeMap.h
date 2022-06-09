/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CHAR_CODE_MAP_H
#define PDF_CHAR_CODE_MAP_H

#include "PdfDeclarations.h"
#include "PdfEncodingCommon.h"

namespace mm
{
    /** A conventient typedef for an unspecified codepoint
     * The underlying type is convenientely char32_t so
     * it's a 32 bit fixed sized type that is also compatible
     * with unicode code points
     */
    using codepoint = char32_t;
    using codepointview = cmn::cspan<codepoint>;

    /**
     * A bidirectional map from character code units to unspecified code points
     *
     * \remarks The actual code point nature is unspecified, but
     * it can either be unicode code points or CID(s) as used
     * in CID keyed fonts. For generic terminology see
     * https://en.wikipedia.org/wiki/Character_encoding#Terminology
     * See also 5014.CIDFont_Spec, 2.1 Terminology
     */
    class PDFMM_API PdfCharCodeMap
    {
    public:
        PdfCharCodeMap();

        PdfCharCodeMap(PdfCharCodeMap&& map) noexcept;

        ~PdfCharCodeMap();

        /** Method to push a mapping.
         * Given string can be a ligature, es "ffi"
         */
        void PushMapping(const PdfCharCode& codeUnit, const codepointview& codePoints);

        /** Convenience method to push a single code point mapping
         */
        void PushMapping(const PdfCharCode& codeUnit, codepoint codePoint);

        /** Returns false when no mapped identifiers are not found in the map
         */
        bool TryGetCodePoints(const PdfCharCode& codeUnit, std::vector<codepoint>& codePoints) const;

        /** Try get char code from utf8 encoded range
         * \remarks It assumes it != and it will consumes the interator
         * also when returning false
         */
        bool TryGetNextCharCode(std::string_view::iterator& it,
            const std::string_view::iterator& end, PdfCharCode& code) const;

        /** Try get char code from unicode code points
         * \param codePoints sequence of unicode code points. All the sequence must match
         */
        bool TryGetCharCode(const codepointview& codePoints, PdfCharCode& code) const;

        /** Try get char code from unicode code point
         */
        bool TryGetCharCode(codepoint codePoint, PdfCharCode& code) const;

        PdfCharCodeMap& operator=(PdfCharCodeMap&& map) noexcept;

        unsigned GetSize() const;

        const PdfEncodingLimits& GetLimits() const;

    private:
        void move(PdfCharCodeMap& map) noexcept;
        void pushMapping(const PdfCharCode& codeUnit, std::vector<codepoint>&& codePoints);

        struct HashCharCode
        {
            size_t operator()(const PdfCharCode& code) const;
        };

        struct EqualCharCode
        {
            bool operator()(const PdfCharCode& lhs, const PdfCharCode& rhs) const;
        };

        // Map code units -> code point(s)
        // pp. 474-475 of PdfReference 1.7 "The value of dstString can be a string of up to 512 bytes"
        typedef std::unordered_map<PdfCharCode, std::vector<codepoint>, HashCharCode, EqualCharCode> CUMap;

        // Map code point(s) -> code units
        struct CPMapNode
        {
            codepoint CodePoint;
            PdfCharCode CodeUnit;
            CPMapNode* Ligatures;
            CPMapNode* Left;
            CPMapNode* Right;
        };

    private:
        PdfCharCodeMap(const PdfCharCodeMap&) = delete;
        PdfCharCodeMap& operator=(const PdfCharCodeMap&) = delete;

    private:
        void reviseCPMap();
        static bool tryFindNextCharacterId(const CPMapNode* node, std::string_view::iterator &it,
            const std::string_view::iterator& end, PdfCharCode& cid);
        static const CPMapNode* findNode(const CPMapNode* node, codepoint codePoint);
        static void deleteNode(CPMapNode* node);
        static CPMapNode* findOrAddNode(CPMapNode*& node, codepoint codePoint);

    public:
        typedef CUMap::const_iterator iterator;

    public:
        iterator begin() const;
        iterator end() const;

    private:
        PdfEncodingLimits m_Limits;
        CUMap m_cuMap;
        bool m_MapDirty;
        CPMapNode* m_cpMapHead;           // Head of a BST to lookup code points
        int m_depth;
    };
}

#endif // PDF_CHAR_CODE_MAP_H
