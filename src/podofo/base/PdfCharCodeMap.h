/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CHAR_CODE_MAP_H
#define PDF_CHAR_CODE_MAP_H

#include "PdfDefines.h"
#include <unordered_map>

namespace PoDoFo
{
    /** A character code unit
     *
     * For generic terminology see https://en.wikipedia.org/wiki/Character_encoding#Terminology
     * See also 5014.CIDFont_Spec, 2.1 Terminology
     */
    struct PODOFO_API PdfCharCode
    {
        unsigned Code;

        // RangeSize example <cd> -> 1, <00cd> -> 2
        unsigned char CodeSpaceSize;

        PdfCharCode();

        /** Create a code of minimum size
         */
        explicit PdfCharCode(unsigned code);

        PdfCharCode(unsigned code, unsigned char codeSpaceSize);

    public:
        void AppendTo(std::string& str, bool clear = false) const;
        void WriteHexTo(std::string& str, bool wrap = true) const;
    };

    /**
     * A bidirectional map from character code units to generic code points
     *
     * \remarks Code points actual encoding is unspecified, but it can be unicode code points
     * or CID(s) as for CID keyed fonts
     * For generic terminology see https://en.wikipedia.org/wiki/Character_encoding#Terminology
     * See also 5014.CIDFont_Spec, 2.1 Terminology
     */
    class PODOFO_API PdfCharCodeMap
    {
    public:
        PdfCharCodeMap();

        PdfCharCodeMap(PdfCharCodeMap&& map) noexcept;

        ~PdfCharCodeMap();

        /** Method to push a mapping.
         * Given string can be a ligature, es "ffi"
         */
        void PushMapping(const PdfCharCode& codeUnit, const cspan<char32_t>& codePoints);

        /** Convenience method to push a single code point mapping
         */
        void PushMapping(const PdfCharCode& codeUnit, char32_t codePoint);

        /** Returns false when no mapped identifiers are not found in the map
         */
        bool TryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

        /** Try get char code from utf8 encoded range
         * \remarks It assumes it != and it will consumes the interator
         * also when returning false
         */
        bool TryGetNextCharCode(std::string_view::iterator& it,
            const std::string_view::iterator& end, PdfCharCode& code) const;

        /** Try get char code from unicode code points
         * \param codePoints sequence of unicode code points. All the sequence must match
         */
        bool TryGetCharCode(const cspan<char32_t>& codePoints, PdfCharCode& code) const;

        /** Try get char code from unicode code point
         */
        bool TryGetCharCode(char32_t codePoint, PdfCharCode& code) const;

        PdfCharCodeMap& operator=(PdfCharCodeMap&& map) noexcept;

    private:
        void move(PdfCharCodeMap& map) noexcept;
        void pushMapping(const PdfCharCode& codeUnit, std::vector<char32_t>&& codePoints);

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
        typedef std::unordered_map<PdfCharCode, std::vector<char32_t>, HashCharCode, EqualCharCode> CUMap;

        // Map code point(s) -> code units
        struct CPMapNode
        {
            char32_t CodePoint;
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
        static const CPMapNode* findNode(const CPMapNode* node, char32_t codePoint);
        static void deleteNode(CPMapNode* node);
        static CPMapNode* findOrAddNode(CPMapNode*& node, char32_t codePoint);

    public:
        typedef CUMap::const_iterator iterator;

    public:
        iterator begin() const;
        iterator end() const;

    private:
        CUMap m_cuMap;
        unsigned char m_maxCodeSpaceSize;
        bool m_MapDirty;
        CPMapNode* m_cpMapHead;           // Head of a BST to lookup code points
        int m_depth;
    };
}

#endif // PDF_CHAR_CODE_MAP_H
