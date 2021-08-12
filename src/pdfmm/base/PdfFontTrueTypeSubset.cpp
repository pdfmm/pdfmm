/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// The code was initally based on work by ZhangYang
// (张杨.国际) <zhang_yang@founder.com>

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontTrueTypeSubset.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/tttables.h>
#include <freetype/tttags.h>

#include <iostream>
#include <algorithm>

#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

// Required TrueType tables
enum class ReqTable
{
    none = 0,
    head = 1,
    hhea = 2,
    loca = 4,
    maxp = 8,
    glyf = 16,
    hmtx = 32,
    all = head | hhea | loca | maxp | glyf | hmtx,
};

ENABLE_BITMASK_OPERATORS(ReqTable);

static constexpr unsigned LENGTH_HEADER12 = 12;
static constexpr unsigned LENGTH_OFFSETTABLE16 = 16;
static constexpr unsigned LENGTH_DWORD = 4;
static constexpr unsigned LENGTH_WORD = 2;

//Get the number of bytes to pad the ul, because of 4-byte-alignment.
static uint32_t GetTableCheksum(const char* buf, uint32_t size);

static void TTFWriteUInt32(PdfOutputDevice& output, uint32_t value);
static void TTFWriteUInt16(PdfOutputDevice& output, uint16_t value);
static void TTFWriteUInt32(char* buf, uint32_t value);
static void TTFWriteUInt16(char* buf, uint16_t value);

PdfFontTrueTypeSubset::PdfFontTrueTypeSubset(PdfInputDevice& device, TrueTypeFontFileType type,
        const PdfFontMetrics& metrics, unsigned short faceIndex) :
    m_Device(&device),
    m_FontFileType(type),
    m_Metrics(&metrics),
    m_StartOfTTFOffsets(0),
    m_faceIndex(faceIndex),
    m_IsLongLoca(false),
    m_glyphCount(0),
    m_HMetricsCount(0)
{
}

void PdfFontTrueTypeSubset::BuildFont(string& buffer, PdfInputDevice& input,
    TrueTypeFontFileType type, unsigned short faceIndex,
    const PdfFontMetrics& metrics, const CIDToGIDMap& cidToGidMap)
{
    PdfFontTrueTypeSubset subset(input, type, metrics, faceIndex);
    subset.BuildFont(buffer, cidToGidMap);
}

void PdfFontTrueTypeSubset::BuildFont(string& buffer,
    const CIDToGIDMap& cidToGidMap)
{
    Init();

    GlyphContext context;
    context.GlyfTableOffset = GetTableOffset(TTAG_glyf);
    context.LocaTableOffset = GetTableOffset(TTAG_loca);
    LoadGlyphs(context, cidToGidMap);
    WriteTables(buffer);
}

void PdfFontTrueTypeSubset::Init()
{
    GetStartOfTTFOffsets();
    InitTables();
    GetNumberOfGlyphs();
    SeeIfLongLocaOrNot();
}

unsigned PdfFontTrueTypeSubset::GetTableOffset(unsigned tag)
{
    for (auto& table : m_Tables)
    {
        if (table.Tag == tag)
            return table.Offset;
    }
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "table missing");
}

void PdfFontTrueTypeSubset::GetNumberOfGlyphs()
{
    unsigned offset = GetTableOffset(TTAG_maxp);

    GetData(&m_glyphCount, offset + LENGTH_DWORD * 1, LENGTH_WORD);
    m_glyphCount = FROM_BIG_ENDIAN(m_glyphCount);

    offset = GetTableOffset(TTAG_hhea);

    GetData(&m_HMetricsCount, offset + LENGTH_WORD * 17, LENGTH_WORD);
    m_HMetricsCount = FROM_BIG_ENDIAN(m_HMetricsCount);
}

void PdfFontTrueTypeSubset::InitTables()
{
    uint16_t tableCount;
    GetData(&tableCount, m_StartOfTTFOffsets + LENGTH_DWORD * 1, LENGTH_WORD);
    tableCount = FROM_BIG_ENDIAN(tableCount);

    ReqTable tableMask = ReqTable::none;
    TrueTypeTable tbl;

    for (unsigned short i = 0; i < tableCount; i++)
    {
        // Name of each table:
        GetData(&tbl.Tag, m_StartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i, LENGTH_DWORD);
        tbl.Tag = FROM_BIG_ENDIAN(tbl.Tag);

        // Checksum of each table:
        GetData(&tbl.Checksum, m_StartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + LENGTH_DWORD * 1, LENGTH_DWORD);
        tbl.Checksum = FROM_BIG_ENDIAN(tbl.Checksum);

        // Offset of each table:
        GetData(&tbl.Offset, m_StartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + LENGTH_DWORD * 2, LENGTH_DWORD);
        tbl.Offset = FROM_BIG_ENDIAN(tbl.Offset);

        // Length of each table:
        GetData(&tbl.Length, m_StartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + LENGTH_DWORD * 3, LENGTH_DWORD);
        tbl.Length = FROM_BIG_ENDIAN(tbl.Length);

        // PDF 32000-1:2008 9.9 Embedded Font Programs
        // "These TrueType tables shall always be present if present in the original TrueType font program:
        // 'head', 'hhea', 'loca', 'maxp', 'cvt','prep', 'glyf', 'hmtx' and 'fpgm'. [..]  If used with a
        // CIDFont dictionary, the 'cmap' table is not needed and shall not be present

        bool skipTable = false;
        switch (tbl.Tag)
        {
            case TTAG_head:
                tableMask |= ReqTable::head;
                break;
            case TTAG_hhea:
                // required to get numHMetrics
                tableMask |= ReqTable::hhea;
                break;
            case TTAG_loca:
                tableMask |= ReqTable::loca;
                break;
            case TTAG_maxp:
                tableMask |= ReqTable::maxp;
                break;
            case TTAG_glyf:
                tableMask |= ReqTable::glyf;
                break;
            case TTAG_hmtx:
                // advance width
                tableMask |= ReqTable::hmtx;
                break;
            case TTAG_cvt:
            case TTAG_fpgm:
            case TTAG_prep:
                // Just include these tables inconditionally if present
                // in the original font
                break;
            case TTAG_post:
                if (tbl.Length < 32)
                    skipTable = true;

                // Reduce table size, leter we will change format to 'post' Format 3
                tbl.Length = 32;
                break;
            // Exclude all other tables, including cmap which
            // is not required
            case TTAG_cmap:
            default:
                skipTable = true;
                break;
        }
        if (!skipTable)
            m_Tables.push_back(tbl);
    }

    if ((tableMask & ReqTable::all) == ReqTable::none)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, "Required TrueType table missing");
}

void PdfFontTrueTypeSubset::GetStartOfTTFOffsets()
{
    switch (m_FontFileType)
    {
        case TrueTypeFontFileType::TTF:
        case TrueTypeFontFileType::OTF:
            m_StartOfTTFOffsets = 0;
            break;
        case TrueTypeFontFileType::TTC:
        {
            uint32_t ulnumFace;
            GetData(&ulnumFace, 8, LENGTH_DWORD);
            ulnumFace = FROM_BIG_ENDIAN(ulnumFace);

            GetData(&m_StartOfTTFOffsets, (m_faceIndex + 3) * LENGTH_DWORD, LENGTH_DWORD);
            m_StartOfTTFOffsets = FROM_BIG_ENDIAN(m_StartOfTTFOffsets);
        }
        break;
        case TrueTypeFontFileType::Unknown:
        default:
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Invalid font type");
    }
}

void PdfFontTrueTypeSubset::SeeIfLongLocaOrNot()
{
    unsigned ulHeadOffset = GetTableOffset(TTAG_head);
    uint16_t isLong;
    GetData(&isLong, ulHeadOffset + 50, LENGTH_WORD);
    isLong = FROM_BIG_ENDIAN(isLong);
    m_IsLongLoca = (isLong == 0 ? false : true);  // 1 for long
}

void PdfFontTrueTypeSubset::LoadGlyphs(GlyphContext& ctx, const CIDToGIDMap& usedCodes)
{
    // For any fonts, assume that glyph 0 is needed.
    LoadGID(ctx, 0);
    unsigned prevCID = 0;
    for (auto& pair : usedCodes)
    {
        if ((pair.first - prevCID) != 1)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "The cid to gid map should be starting with 1 have consecutive indices");

        LoadGID(ctx, pair.second);
        prevCID = pair.first;
    }
}

void PdfFontTrueTypeSubset::LoadGID(GlyphContext& ctx, unsigned gid)
{
    if (gid >= m_glyphCount)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "GID out of range");

    if (m_GlyphMap.find(gid) != m_GlyphMap.end())
        return;

    GlyphData glyphData;
    if (m_IsLongLoca)
    {
        GetData(&glyphData.GlyphAddress, ctx.LocaTableOffset + LENGTH_DWORD * gid, LENGTH_DWORD);
        glyphData.GlyphAddress = FROM_BIG_ENDIAN(glyphData.GlyphAddress);

        GetData(&glyphData.GlyphLength, ctx.LocaTableOffset + LENGTH_DWORD * (gid + 1), LENGTH_DWORD);
        glyphData.GlyphLength = FROM_BIG_ENDIAN(glyphData.GlyphLength);
    }
    else
    {
        GetData(&ctx.ShortOffset, ctx.LocaTableOffset + LENGTH_WORD * gid, LENGTH_WORD);
        glyphData.GlyphAddress = FROM_BIG_ENDIAN(ctx.ShortOffset);
        glyphData.GlyphAddress <<= 1;

        GetData(&ctx.ShortOffset, ctx.LocaTableOffset + LENGTH_WORD * (gid + 1), LENGTH_WORD);
        glyphData.GlyphLength = FROM_BIG_ENDIAN(ctx.ShortOffset);
        glyphData.GlyphLength <<= 1;
    }

    glyphData.GlyphLength -= glyphData.GlyphAddress;

    m_GlyphMap[gid] = glyphData;
    m_OrderedGlyphs.push_back(gid);

    GetData(&ctx.ContourCount, ctx.GlyfTableOffset + glyphData.GlyphAddress, LENGTH_WORD);
    ctx.ContourCount = FROM_BIG_ENDIAN(ctx.ContourCount);
    if (ctx.ContourCount < 0)
    {
        // skeep over numberOfContours, xMin, yMin, xMax and yMax
        LoadCompound(ctx, glyphData.GlyphAddress + 5 * LENGTH_WORD);
    }
}

void PdfFontTrueTypeSubset::LoadCompound(GlyphContext& ctx, unsigned offset)
{
    throw runtime_error("Untested after utf8 migration");
    // The following code can't be correct anymore. Compounds can load
    // further glyps but these can't be added subsequentially because
    // we espect explicitly loaded glyphs to be consecutive. We should
    // just load glyphs in the compound after all the explicitly loaded
    // ones and remap the indices


    uint16_t flags;
    uint16_t glyphIndex;

    constexpr unsigned ARG_1_AND_2_ARE_WORDS = 0x01;
    constexpr unsigned WE_HAVE_A_SCALE = 0x08;
    constexpr unsigned MORE_COMPONENTS = 0x20;
    constexpr unsigned WE_HAVE_AN_X_AND_Y_SCALE = 0x40;
    constexpr unsigned WE_HAVE_TWO_BY_TWO = 0x80;

    while (true)
    {
        GetData(&flags, ctx.GlyfTableOffset + offset, LENGTH_WORD);
        flags = FROM_BIG_ENDIAN(flags);

        GetData(&glyphIndex, ctx.GlyfTableOffset + offset + LENGTH_WORD, LENGTH_WORD);
        glyphIndex = FROM_BIG_ENDIAN(glyphIndex);

        LoadGID(ctx, glyphIndex);

        if ((flags & MORE_COMPONENTS) == 0)
            break;

        offset += (flags & ARG_1_AND_2_ARE_WORDS) ? 4 * LENGTH_WORD : 3 * LENGTH_WORD;
        if ((flags & WE_HAVE_A_SCALE) != 0)
            offset += LENGTH_WORD;
        else if ((flags & WE_HAVE_AN_X_AND_Y_SCALE) != 0)
            offset += 2 * LENGTH_WORD;
        else if ((flags & WE_HAVE_TWO_BY_TWO) != 0)
            offset += 4 * LENGTH_WORD;
    }
}

// Ref: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6glyf.html
void PdfFontTrueTypeSubset::WriteGlyphTable(PdfOutputDevice& output, unsigned glyphTableOffset)
{
    for (unsigned gid : m_OrderedGlyphs)
    {
        auto& glyphData = m_GlyphMap[gid];
        if (glyphData.GlyphLength != 0)
            GetData(output, glyphTableOffset + glyphData.GlyphAddress, glyphData.GlyphLength);
    }
}

// The 'hmtx' table contains the horizontal metrics for each glyph in the font
// https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6hmtx.html
void PdfFontTrueTypeSubset::WriteHmtxTable(PdfOutputDevice& output)
{
    struct LongHorMetrics
    {
        uint16_t AdvanceWidth;
        int16_t LeftSideBearing;
    };

    unsigned tableOffset = GetTableOffset(TTAG_hmtx);
    for (unsigned gid : m_OrderedGlyphs)
        GetData(output, tableOffset + gid * sizeof(LongHorMetrics), sizeof(LongHorMetrics));
}

// "The 'loca' table stores the offsets to the locations
// of the glyphs in the font relative to the beginning of
// the 'glyf' table. [..] To make it possible to compute
// the length of the last glyph element, there is an extra
// entry after the offset that points to the last valid
// index. This index points to the end of the glyph data"
// Ref: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6loca.html
void PdfFontTrueTypeSubset::WriteLocaTable(PdfOutputDevice& output)
{
    unsigned glyphAddress = 0;
    if (m_IsLongLoca)
    {
        for (unsigned gid : m_OrderedGlyphs)
        {
            auto& glyphData = m_GlyphMap[gid];
            TTFWriteUInt32(output, glyphAddress);
            glyphAddress += glyphData.GlyphLength;
        }

        // Last "extra" entry
        TTFWriteUInt32(output, glyphAddress);
    }
    else
    {
        for (unsigned gid : m_OrderedGlyphs)
        {
            auto& glyphData = m_GlyphMap[gid];
            TTFWriteUInt16(output, static_cast<uint16_t>(glyphAddress >> 1));
            glyphAddress += glyphData.GlyphLength;
        }

        // Last "extra" entry
        TTFWriteUInt16(output, static_cast<uint16_t>(glyphAddress >> 1));
    }

}

void PdfFontTrueTypeSubset::WriteTables(string& buffer)
{
    PdfStringOutputDevice output(buffer);

    uint16_t entrySelector = (uint16_t)std::ceil(std::log2(m_Tables.size()));
    uint16_t searchRange = (uint16_t)std::pow(2, entrySelector);
    uint16_t rangeShift = (16 * (uint16_t)m_Tables.size()) - searchRange;

    // Write the font directory table
    // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html
    TTFWriteUInt32(output, 0x00010000);     // Scaler type, 0x00010000 is True type font
    TTFWriteUInt16(output, (uint16_t)m_Tables.size());
    TTFWriteUInt16(output, searchRange);
    TTFWriteUInt16(output, entrySelector);
    TTFWriteUInt16(output, rangeShift);

    size_t directoryTableOffset = output.Tell();

    // Prepare table offsets
    for (unsigned i = 0; i < m_Tables.size(); i++)
    {
        auto& table = m_Tables[i];
        TTFWriteUInt32(output, table.Tag);
        // Write empty placeholders
        TTFWriteUInt32(output, 0); // Table checksum
        TTFWriteUInt32(output, 0); // Table offset
        TTFWriteUInt32(output, 0); // Table length (actual length not padded length)
    }

    optional<size_t> headOffset;
    size_t tableOffset;
    for (unsigned i = 0; i < m_Tables.size(); i++)
    {
        auto& table = m_Tables[i];
        tableOffset = output.Tell();
        switch (table.Tag)
        {
            case TTAG_head:
                headOffset = tableOffset;
                GetData(output, table.Offset, table.Length);
                // Set the checkSumAdjustment to 0
                TTFWriteUInt32(buffer.data() + tableOffset + 4, 0);
                break;
            case TTAG_maxp:
                // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6maxp.html
                GetData(output, table.Offset, table.Length);
                // Write the number of glyphs in the font
                TTFWriteUInt16(buffer.data() + tableOffset + 4, (uint16_t)m_GlyphMap.size());
                break;
            case TTAG_hhea:
                // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6hhea.html
                GetData(output, table.Offset, table.Length);
                // Write numOfLongHorMetrics, see also 'hmtx' table
                TTFWriteUInt16(buffer.data() + tableOffset + 34, (uint16_t)m_GlyphMap.size());
                break;
            case TTAG_post:
                // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6post.html
                GetData(output, table.Offset, table.Length);
                // Enforce 'post' Format 3, written as a Fixed 16.16 number
                TTFWriteUInt32(buffer.data() + tableOffset, 0x00030000);
                // Clear Type42/Type1 font information
                memset(buffer.data() + tableOffset + 16, 0, 16);
                break;
            case TTAG_glyf:
                WriteGlyphTable(output, table.Offset);
                break;
            case TTAG_loca:
                WriteLocaTable(output);
                break;
            case TTAG_hmtx:
                WriteHmtxTable(output);
                break;
            case TTAG_cvt:
            case TTAG_fpgm:
            case TTAG_prep:
                GetData(output, table.Offset, table.Length);
                break;
            default:
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unsupported table at this context");
        }

        // Align the table length to 4 bytes and pad remaing space with zeroes
        size_t tableLength = output.Tell() - tableOffset;
        size_t tableLengthPadded = (tableLength + 3) & ~3;
        for (size_t i = tableLength; i < tableLengthPadded; i++)
            output.Put('\0');

        // Write dynamic font directory table entries
        size_t currDirTableOffset = directoryTableOffset + i * LENGTH_OFFSETTABLE16;
        TTFWriteUInt32(buffer.data() + currDirTableOffset + 4, GetTableCheksum(buffer.data() + tableOffset, (uint32_t)tableLength));
        TTFWriteUInt32(buffer.data() + currDirTableOffset + 8, (uint32_t)tableOffset);
        TTFWriteUInt32(buffer.data() + currDirTableOffset + 12, (uint32_t)tableLength);
    }

    // Check for head table
    if (!headOffset.has_value())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "'head' table missing");

    // As explained in the "Table Directory"
    // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html#Directory
    uint32_t fontChecksum = 0xB1B0AFBA - GetTableCheksum(buffer.data(), (uint32_t)output.Tell());
    TTFWriteUInt32(buffer.data() + *headOffset + 4, fontChecksum);
}

void PdfFontTrueTypeSubset::GetData(PdfOutputDevice& output, unsigned offset, unsigned size)
{
    m_Device->Seek(offset);
    m_tmpBuffer.resize(size);
    m_Device->Read(m_tmpBuffer.data(), size);
    output.Write(m_tmpBuffer.data(), size);
}

void PdfFontTrueTypeSubset::GetData(void* dst, unsigned offset, unsigned size)
{
    m_Device->Seek(offset);
    m_Device->Read((char*)dst, size);
}

uint32_t GetTableCheksum(const char* buf, uint32_t size)
{
    // As explained in the "Table Directory"
    // https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6.html#Directory
    uint32_t sum = 0;
    uint32_t nLongs = (size + 3) / 4;
    while (nLongs-- > 0)
        sum += *buf++;

    return sum;
}

void TTFWriteUInt32(PdfOutputDevice& output, uint32_t value)
{
    char buf[4];
    TTFWriteUInt32(buf, value);
    output.Write(buf, 4);
}

void TTFWriteUInt16(PdfOutputDevice& output, uint16_t value)
{
    char buf[2];
    TTFWriteUInt16(buf, value);
    output.Write(buf, 2);
}

void TTFWriteUInt32(char* buf, uint32_t value)
{
    // Write the unsigned integer in big-endian format
    buf[0] = static_cast<char>((value >> 24) & 0xFF);
    buf[1] = static_cast<char>((value >> 16) & 0xFF);
    buf[2] = static_cast<char>((value >>  8) & 0xFF);
    buf[3] = static_cast<char>((value >>  0) & 0xFF);
}

void TTFWriteUInt16(char* buf, uint16_t value)
{
    // Write the unsigned integer in big-endian format
    buf[0] = static_cast<char>((value >> 8) & 0xFF);
    buf[1] = static_cast<char>((value >> 0) & 0xFF);
}
