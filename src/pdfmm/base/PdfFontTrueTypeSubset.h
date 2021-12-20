/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_TTF_SUBSET_H
#define PDF_FONT_TTF_SUBSET_H

#include "PdfDeclarations.h"

#include <map>

#include "PdfFontMetrics.h"

namespace mm {

class PdfInputDevice;
class PdfOutputDevice;

/**
 * Internal enum specifying the type of a fontfile.
 */
enum class TrueTypeFontFileType
{
    Unknown, ///< Unknown
    TTF,    ///< TrueType Font
    TTC,    ///< TrueType Collection
    OTF,    ///< OpenType Font
};

typedef std::map<unsigned, unsigned> CIDToGIDMap;

/**
 * This class is able to build a new TTF font with only
 * certain glyphs from an existing font.
 *
 */
class PDFMM_API PdfFontTrueTypeSubset final
{
private:
    PdfFontTrueTypeSubset(PdfInputDevice& device, unsigned short faceIndex);

public:
    /**
     * Actually generate the subsetted font
     * Create a new PdfFontTrueTypeSubset from an existing
     * TTF font file using an input device.
     *
     * \param output write the font to this buffer
     * \param inputDevice a PdfInputDevice
     * \param type the type of the font
     * \param faceIndex index of the face inside of the font
     * \param metrics font metrics object for this font
     * \param cidToGidMap a map from cids to gids. It shall be a map
     *     of consecutive indices starting with 1
     * \param cidSet the output /CidSet
     */
    static void BuildFont(std::string& output, PdfInputDevice& input,
        unsigned short faceIndex, const CIDToGIDMap& cidToGidMap);

private:
    PdfFontTrueTypeSubset(const PdfFontTrueTypeSubset& rhs) = delete;
    PdfFontTrueTypeSubset& operator=(const PdfFontTrueTypeSubset& rhs) = delete;

    void BuildFont(std::string& buffer, const CIDToGIDMap& cidToGidMap);

    void Init();
    void DetermineFontType();
    unsigned GetTableOffset(unsigned tag);
    void GetNumberOfGlyphs();
    void SeeIfLongLocaOrNot();
    void InitTables();
    void GetStartOfTTFOffsets();

    void CopyData(PdfOutputDevice& output, unsigned offset, unsigned size);

private:
    /** Information of TrueType tables.
     */
    struct TrueTypeTable
    {
        uint32_t Tag = 0;
        uint32_t Checksum = 0;
        uint32_t Length = 0;
        uint32_t Offset = 0;
    };

    struct GlyphCompoundComponentData
    {
        unsigned Offset;
        unsigned GlyphIndex;
    };

    /** GlyphData contains the glyph address relative
     *  to the beginning of the "glyf" table.
     */
    struct GlyphData
    {
        bool IsCompound;
        unsigned GlyphOffset;       // Offset of common "glyph" data
        unsigned GlyphLength;
        unsigned GlyphAdvOffset;    // Offset of uncommon simple/compound "glyph" data
        std::vector<GlyphCompoundComponentData> CompoundComponents;
    };

    // A CID indexed glyph map
    typedef std::map<unsigned, GlyphData> GlyphDatas;

    struct GlyphContext
    {
        unsigned GlyfTableOffset = 0;
        unsigned LocaTableOffset = 0;
        // Used internaly during recursive load
        int16_t ContourCount = 0;
    };

    struct GlyphCompoundData
    {
        unsigned Flags;
        unsigned GlyphIndex;
    };

    void LoadGlyphs(GlyphContext& ctx, const CIDToGIDMap& usedCodes);
    void LoadGID(GlyphContext& ctx, unsigned gid);
    void LoadCompound(GlyphContext& ctx, const GlyphData& data);
    void WriteGlyphTable(PdfOutputDevice& output);
    void WriteHmtxTable(PdfOutputDevice& output);
    void WriteLocaTable(PdfOutputDevice& output);
    void WriteTables(std::string& buffer);
    void ReadGlyphCompoundData(GlyphCompoundData& data, unsigned offset);

private:
    PdfInputDevice* m_device;          // Read data from this input device
    TrueTypeFontFileType m_fontFileType;

    uint32_t m_startOfTTFOffsets;	   // Start address of the truetype offset tables, differs from ttf to ttc.
    unsigned short m_faceIndex;
    bool m_isLongLoca;
    uint16_t m_glyphCount;
    uint16_t m_HMetricsCount;

    std::vector<TrueTypeTable> m_tables;
    GlyphDatas m_glyphDatas;
    std::vector<unsigned> m_orderedGIDs; // Ordered list of original GIDs as they will appear in the subset
    charbuff m_tmpBuffer;
};

};

#endif // PDF_FONT_TRUE_TYPE_H
