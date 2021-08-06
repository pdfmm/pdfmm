/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfCharCodeMap.h"
#include <random>
#include <algorithm>
#include <utfcpp/utf8.h>

using namespace std;
using namespace PoDoFo;

PdfCharCodeMap::PdfCharCodeMap()
    : m_maxCodeSpaceSize(0), m_MapDirty(false), m_cpMapHead(nullptr), m_depth(0) { }

PdfCharCodeMap::PdfCharCodeMap(PdfCharCodeMap&& map) noexcept
{
    move(map);
}

PdfCharCodeMap::~PdfCharCodeMap()
{
    deleteNode(m_cpMapHead);
}

PdfCharCodeMap& PdfCharCodeMap::operator=(PdfCharCodeMap&& map) noexcept
{
    move(map);
    return *this;
}

void PdfCharCodeMap::move(PdfCharCodeMap& map) noexcept
{
    m_cuMap = std::move(map.m_cuMap);
    usr::move(map.m_maxCodeSpaceSize, m_maxCodeSpaceSize);
    usr::move(map.m_MapDirty, m_MapDirty);
    usr::move(map.m_cpMapHead, m_cpMapHead);
    usr::move(map.m_depth, m_depth);
}

void PdfCharCodeMap::PushMapping(const PdfCharCode& codeUnit, const cspan<char32_t>& codePoints)
{
    if (codePoints.size() == 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "CodePoints must not be empty");

    vector<char32_t> copy(codePoints.begin(), codePoints.end());
    pushMapping(codeUnit, std::move(copy));
}

void PdfCharCodeMap::PushMapping(const PdfCharCode& codeUnit, char32_t codePoint)
{
    vector<char32_t> codePoints = { codePoint };
    pushMapping(codeUnit, std::move(codePoints));
}

bool PdfCharCodeMap::TryGetCodePoints(const PdfCharCode& codeUnit, vector<char32_t>& codePoints) const
{
    auto found = m_cuMap.find(codeUnit);
    if (found == m_cuMap.end())
    {
        codePoints.clear();
        return false;
    }

    codePoints = found->second;
    return true;
}

bool PdfCharCodeMap::TryGetNextCharCode(string_view::iterator& it, const string_view::iterator& end, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCPMap();
    return tryFindNextCharacterId(m_cpMapHead, it, end, code);
}

bool PdfCharCodeMap::TryGetCharCode(const cspan<char32_t>& codePoints, PdfCharCode& codeUnit) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCPMap();
    auto it = codePoints.begin();
    auto end = codePoints.end();
    const CPMapNode* node = m_cpMapHead;
    if (it == end)
        goto NotFound;

    do
    {
        // All the sequence must match
        node = findNode(node, *it);
        if (node == nullptr)
            goto NotFound;

        node = node->Ligatures;
        it++;
    } while (it != end);

    if (node->CodeUnit.CodeSpaceSize == 0)
    {
        // Undefined char code
        goto NotFound;
    }
    else
    {
        codeUnit = node->CodeUnit;
        return true;
    }

NotFound:
    codeUnit = { };
    return false;
}

bool PdfCharCodeMap::TryGetCharCode(char32_t codePoint, PdfCharCode& code) const
{
    const_cast<PdfCharCodeMap&>(*this).reviseCPMap();
    auto node = findNode(m_cpMapHead, codePoint);
    if (node == nullptr)
    {
        code = { };
        return false;
    }

    code = node->CodeUnit;
    return true;
}

void PdfCharCodeMap::pushMapping(const PdfCharCode& codeUnit, vector<char32_t>&& codePoints)
{
    if (codeUnit.CodeSpaceSize == 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Code unit must be valid");

    m_cuMap[codeUnit] = std::move(codePoints);
    if (codeUnit.CodeSpaceSize > m_maxCodeSpaceSize)
        m_maxCodeSpaceSize = codeUnit.CodeSpaceSize;

    m_MapDirty = true;
}

bool PdfCharCodeMap::tryFindNextCharacterId(const CPMapNode* node, string_view::iterator& it,
    const string_view::iterator& end, PdfCharCode& codeUnit)
{
    PODOFO_INVARIANT(it != end);
    string_view::iterator curr;
    char32_t codepoint = (char32_t)utf8::next(it, end);
    node = findNode(node, codepoint);
    if (node == nullptr)
        goto NotFound;

    if (it != end)
    {
        // Try to find ligatures, save a temporary iterator
        // in case the search in unsuccessful
        curr = it;
        if (tryFindNextCharacterId(node->Ligatures, curr, end, codeUnit))
        {
            it = curr;
            return true;
        }
    }

    if (node->CodeUnit.CodeSpaceSize == 0)
    {
        // Undefined char code
        goto NotFound;
    }
    else
    {
        codeUnit = node->CodeUnit;
        return true;
    }

NotFound:
    codeUnit = { };
    return false;
}

const PdfCharCodeMap::CPMapNode* PdfCharCodeMap::findNode(const CPMapNode* node, char32_t codePoint)
{
    if (node == nullptr)
        return nullptr;

    if (node->CodePoint == codePoint)
        return node;
    else if (node->CodePoint > codePoint)
        return findNode(node->Left, codePoint);
    else
        return findNode(node->Right, codePoint);
}

void PdfCharCodeMap::reviseCPMap()
{
    if (!m_MapDirty)
        return;

    if (m_cpMapHead != nullptr)
    {
        deleteNode(m_cpMapHead);
        m_cpMapHead = nullptr;
    }

    // Randomize items in the map in a separate list
    // so BST creation will be more balanced
    // https://en.wikipedia.org/wiki/Random_binary_tree
    // TODO: Create a perfectly balanced BST
    vector<pair<PdfCharCode, vector<char32_t>>> pairs;
    pairs.reserve(m_cuMap.size());
    std::copy(m_cuMap.begin(), m_cuMap.end(), std::back_inserter(pairs));
    std::mt19937 e(random_device{}());
    std::shuffle(pairs.begin(), pairs.end(), e);

    for (auto& pair : pairs)
    {
        CPMapNode** curr = &m_cpMapHead;      // Node root being searched
        CPMapNode* found;                     // Last found node
        auto it = pair.second.begin();
        auto end = pair.second.end();
        PODOFO_INVARIANT(it != end);
        while (true)
        {
            found = findOrAddNode(*curr, *it);
            it++;
            if (it == end)
                break;

            // We add subsequent codepoints to ligatures
            curr = &found->Ligatures;
        }

        // Finally set the char code on the last found/added node
        found->CodeUnit = pair.first;
    }

    m_MapDirty = false;
}

PdfCharCodeMap::CPMapNode* PdfCharCodeMap::findOrAddNode(CPMapNode*& node, char32_t codePoint)
{
    if (node == nullptr)
    {
        node = new CPMapNode{ };
        node->CodePoint = codePoint;
        return node;
    }

    if (node->CodePoint == codePoint)
        return node;
    else if (node->CodePoint > codePoint)
        return findOrAddNode(node->Left, codePoint);
    else
        return findOrAddNode(node->Right, codePoint);
}

PdfCharCodeMap::iterator PdfCharCodeMap::begin() const
{
    return m_cuMap.begin();
}

PdfCharCodeMap::iterator PdfCharCodeMap::end() const
{
    return m_cuMap.end();
}

void PdfCharCodeMap::deleteNode(CPMapNode* node)
{
    if (node == nullptr)
        return;

    deleteNode(node->Ligatures);
    deleteNode(node->Left);
    deleteNode(node->Right);
    delete node;
}

PdfCharCode::PdfCharCode()
    : Code(0), CodeSpaceSize(0)
{
}

PdfCharCode::PdfCharCode(unsigned code)
    : Code(code), CodeSpaceSize(usr::GetCharCodeSize(code))
{
}

PdfCharCode::PdfCharCode(unsigned code, unsigned char codeSpaceSize)
    : Code(code), CodeSpaceSize(codeSpaceSize)
{
}

void PdfCharCode::AppendTo(string& str, bool clear) const
{
    if (clear)
        str.clear();

    for (unsigned i = CodeSpaceSize; i >= 1; i--)
        str.append(1, (char)((Code >> (i - 1) * CHAR_BIT) & 0xFF));
}

void PdfCharCode::WriteHexTo(string& str, bool wrap) const
{
    const char* pattern;
    // TODO: Use fmt
    switch (CodeSpaceSize)
    {
        case 1:
            pattern = "%02X";
            break;
        case 2:
            pattern = "%04X";
            break;
        case 3:
            pattern = "%06X";
            break;
        case 4:
            pattern = "%08X";
            break;
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Code space must be [1,4]");
    }

    size_t size = CodeSpaceSize * 2 + (wrap ? 2 : 0);
    str.resize(size);
    if (wrap)
        str.front() = '<';
    snprintf(str.data() + (wrap ? 1 : 0), size, pattern, Code);
    if (wrap)
        str.back() = '>';
}

size_t PdfCharCodeMap::HashCharCode::operator()(const PdfCharCode& code) const
{
    return code.CodeSpaceSize << 24 | code.Code;
}

bool PdfCharCodeMap::EqualCharCode::operator()(const PdfCharCode& lhs, const PdfCharCode& rhs) const
{
    return lhs.CodeSpaceSize == rhs.CodeSpaceSize && lhs.Code == rhs.Code;
}
