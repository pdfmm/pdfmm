/**
 * Copyright (C) 2008 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_TTF_SUBSET_H
#define PDF_FONT_TTF_SUBSET_H

#include "PdfDefines.h"

#include <map>

#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfInputDevice;
class PdfOutputDevice;

/**
 * Internal enum specifying the type of a fontfile.
 */
enum class TrueTypeFontFileType
{
    TTF,    ///< TrueType Font
    TTC,    ///< TrueType Collection
    OTF,    ///< OpenType Font
    Unknown ///< Unknown
};

typedef std::map<unsigned, unsigned> CIDToGIDMap;

/**
 * This class is able to build a new TTF font with only
 * certain glyphs from an existing font.
 *
 */
class PODOFO_DOC_API PdfFontTrueTypeSubset final
{
private:
    PdfFontTrueTypeSubset(PdfInputDevice& device, TrueTypeFontFileType type,
        const PdfFontMetrics& metrics, unsigned short faceIndex);

public:
    /**
     * Actually generate the subsetted font
     * Create a new PdfFontTrueTypeSubset from an existing
     * TTF font file using an input device.
     *
     * \param outputDevice write the font to this device
     * \param inputDevice a PdfInputDevice
     * \param type the type of the font
     * \param faceIndex index of the face inside of the font
     * \param metrics font metrics object for this font
     * \param cidToGidMap a map from cids to gids. It shall be a map
     *     of consecutive indices starting with 1
     * \param cidSet the output /CidSet
     */
    static void BuildFont(PdfRefCountedBuffer& output, PdfInputDevice& input,
        TrueTypeFontFileType type, unsigned short faceIndex,
        const PdfFontMetrics& metrics, const CIDToGIDMap& cidToGidMap);

private:
    PdfFontTrueTypeSubset(const PdfFontTrueTypeSubset& rhs) = delete;
    PdfFontTrueTypeSubset& operator=(const PdfFontTrueTypeSubset& rhs) = delete;

    void BuildFont(PdfRefCountedBuffer& output, const CIDToGIDMap& cidToGidMap);

    void Init();

    /** Get the offset of a specified table.
     */
    unsigned GetTableOffset(unsigned tag);
    void GetNumberOfGlyphs();
    void SeeIfLongLocaOrNot();
    void InitTables();
    void GetStartOfTTFOffsets();

    void GetData(PdfOutputDevice& output, unsigned offset, unsigned size);
    void GetData(void* dst, unsigned offset, unsigned size);

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

    /** GlyphData contains the glyph address relative
     *  to the beginning of the "glyf" table.
     */
    struct GlyphData
    {
        uint32_t GlyphLength = 0;
        uint32_t GlyphAddress = 0; //In the original truetype file.
    };

    // A CID indexed glyph map
    typedef std::map<unsigned, GlyphData> GlyphMap;

    struct GlyphContext
    {
        unsigned GlyfTableOffset = 0;
        unsigned LocaTableOffset = 0;
        // Used internaly during recursive load
        int16_t ContourCount = 0;
        uint16_t ShortOffset = 0;
    };

    void LoadGlyphs(GlyphContext& ctx, const CIDToGIDMap& usedCodes);
    void LoadGID(GlyphContext& ctx, unsigned gid);
    void LoadCompound(GlyphContext& ctx, unsigned offset);
    void WriteGlyphTable(PdfOutputDevice& output, unsigned glyphTableOffset);
    void WriteHmtxTable(PdfOutputDevice& output);
    void WriteLocaTable(PdfOutputDevice& output);
    void WriteTables(PdfRefCountedBuffer& buffer);

private:
    PdfInputDevice* m_Device;          // Read data from this input device
    TrueTypeFontFileType m_FontFileType;
    const PdfFontMetrics* m_Metrics;   // FontMetrics object which is required to convert
                                       // unicode character points to glyph ids

    uint32_t m_StartOfTTFOffsets;	   // Start address of the truetype offset tables, differs from ttf to ttc.
    unsigned short m_faceIndex;
    bool m_IsLongLoca;
    uint16_t m_glyphCount;
    uint16_t m_HMetricsCount;

    std::vector<TrueTypeTable> m_Tables;
    GlyphMap m_GlyphMap;
    std::vector<unsigned> m_OrderedGlyphs;
    buffer_t m_tmpBuffer;
};

};

#endif // PDF_FONT_TRUE_TYPE_H
