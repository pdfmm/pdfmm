/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_MAP_H
#define PDF_ENCODING_MAP_H

#include "PdfDeclarations.h"
#include "PdfObject.h"
#include "PdfName.h"
#include "PdfCharCodeMap.h"

namespace mm {

class PdfIndirectObjectList;
class PdfDynamicEncoding;
class PdfFont;

struct PDFMM_API PdfEncodingLimits
{
public:
    PdfEncodingLimits(unsigned char minCodeSize, unsigned char maxCodeSize,
        const PdfCharCode& firstCharCode, const PdfCharCode& lastCharCode);
    PdfEncodingLimits();

    /** Returns true if the limits are valid, such as FirstChar is <= LastChar
     */
    bool AreValid() const;

    unsigned char MinCodeSize;
    unsigned char MaxCodeSize;
    PdfCharCode FirstChar;     // The first defined character code
    PdfCharCode LastChar;      // The last defined character code
};

/** Represent a CID (Character ID) with full code unit information
 */
struct PdfCID
{
    unsigned Id;
    PdfCharCode Unit;

    PdfCID();

    /**
     * Create a CID that has an identical code unit of minimum size
     */
    explicit PdfCID(unsigned id);

    PdfCID(unsigned id, const PdfCharCode& unit);

    /**
     * Create a CID that has an identical code as a code unit representation
     */
    PdfCID(const PdfCharCode& unit);
};

/** 
 * A PdfEncodingMap is a low level interface to convert
 * between utf8 and encoded strings in Pdf.
 * \remarks Don't use this class directly, use PdfEncoding
 */
class PDFMM_API PdfEncodingMap
{
    friend class PdfEncoding;

protected:
    PdfEncodingMap(const PdfEncodingLimits& limits);

public:
    /** Try decode next char code from utf8 string range
     */
    bool TryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from unicode code point
     */
    bool TryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const;

    /**
     * Get the char code from a span of unicode code points
     * \param codePoints it can be a single code point or a ligature
     * \return true if the code points match a character code
     */
    bool TryGetCharCode(const unicodeview& codePoints, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from cid
     */
    bool TryGetCharCode(unsigned cid, PdfCharCode& codeUnit) const;

    /** Try decode next cid from from encoded string range
     */
    bool TryGetNextCID(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCID& cid) const;

    /** Try decode next code points from encoded string range
     */
    bool TryGetNextCodePoints(std::string_view::iterator& it,
        const std::string_view::iterator& end, std::vector<char32_t>& codePoints) const;

    /** Try get code points from char code unit
     *
     * \remarks it will iterate available code sizes
     */
    bool TryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

    /** Try get CID identifier code from code unit
     * \param id the identifier of the CID. The identifier is
     * actually the PdfCID::Id part in the full CID representation
     */
    bool TryGetCIDId(const PdfCharCode& codeUnit, unsigned& id) const;

    const PdfEncodingLimits& GetLimits() const { return m_limits; }

    /**
     * True if the encoding represent a CMap encoding
     *
     * This is true not only for PdfCMapEncoding that supplies
     * a proper CMap but for PdfIndentityEncoding and other
     * predefined CMap names as well (ISO 32000-1:2008 Table 118
     * Predefined CJK CMap names, currently not implemented).
     * Other marps rely on presence of /FirstChar to map from a
     * character code to a index in the glyph list
     */
    virtual bool IsCMapEncoding() const;

    /**
     * True if the encoding is a "simple" encoding, that is valid
     * both as a /Encoding entry and can be used to decode Unicode
     * codepoints.
     * 
     * This is true for regular predefined, difference
     * and Type1 implicit encodings and in general it is false for
     * CMap and identity encodings. This property is complementary
     * of IsCMapEncoding and is kept to use in other situations
     * where it is more intuitive
     */
    bool IsSimpleEncoding() const;

    /**
     * True if the encoding has ligatures support
     */
    virtual bool HasLigaturesSupport() const;

    /** Get an export object that will be used during font init
     * \param objects list to use to create document objects
     * \param name name to use
     * \param obj if not null the object will be used instead
     */
    bool TryGetExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;

public:
    virtual ~PdfEncodingMap();

protected:
    /**
     * Try get next char code unit from a utf8 string range
     *
     * \remarks Default implementation just throws
     */
    virtual bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const;

    /**
     * Try get next char code unit from a ligature
     * \param ligature the span has at least 2 unicode code points
     * \remarks Default implementation just throws
     */
    virtual bool tryGetCharCodeSpan(const unicodeview& ligature, PdfCharCode& codeUnit) const;

    /**
     * Try get char code unit from unicode code point
     */
    virtual bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const = 0;

    /**
     * Get code points from a code unit
     *
     * \param wantCID true requires mapping to CID identifier, false for Unicode code points
     */
    virtual bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const = 0;

    /** Get an export object that will be used during font init
     *
     * \remarks Default implementation just throws
     */
    virtual void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const;

    static void AppendUTF16CodeTo(PdfObjectStream& stream, const unicodeview& codePoints, std::u16string& u16tmp);

protected:
    /** During a WriteToUnicodeCMap append "beginbfchar" and "beginbfrange"
     * entries. "bf" stands for Base Font, see Adobe tecnichal notes #5014
     *
     * To be called by PdfEncoding
     */
    virtual void AppendToUnicodeEntries(PdfObjectStream& stream) const = 0;

    /** During a PdfEncoding::ExportToFont() append "begincidchar"
     * and/or "begincidrange" entries. See Adobe tecnichal notes #5014\
     *
     * To be called by PdfEncoding
     */
    virtual void AppendCIDMappingEntries(PdfObjectStream& stream, const PdfFont& font) const = 0;

private:
    bool tryGetNextCodePoints(std::string_view::iterator& it, const std::string_view::iterator& end,
        PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const;

private:
    PdfEncodingLimits m_limits;
};

/**
 * Basic PdfEncodingMap implementation using a PdfCharCodeMap
 */
class PDFMM_API PdfEncodingMapBase : public PdfEncodingMap
{
    friend class PdfDynamicEncoding;

protected:
    PdfEncodingMapBase(PdfCharCodeMap&& map, const nullable<PdfEncodingLimits>& limits = { });

protected:
    bool tryGetNextCharCode(std::string_view::iterator& it,
        const std::string_view::iterator& end, PdfCharCode& codeUnit) const override;

    bool tryGetCharCodeSpan(const unicodeview& codePoints, PdfCharCode& codeUnit) const override;

    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;

    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    void AppendToUnicodeEntries(PdfObjectStream& stream) const override;

    void AppendCIDMappingEntries(PdfObjectStream& stream, const PdfFont& font) const override;

public:
    inline const PdfCharCodeMap& GetCharMap() const { return *m_charMap; }

private:
    PdfEncodingMapBase(const std::shared_ptr<PdfCharCodeMap>& map);
    static PdfEncodingLimits findLimits(const PdfCharCodeMap& map);

private:
    std::shared_ptr<PdfCharCodeMap> m_charMap;
};

/**
 * PdfEncodingMap used by encodings like PdfBuiltInEncoding
 * or PdfDifferenceEncoding thats can define all their charset
 * with a single one byte range
 */
class PDFMM_API PdfEncodingMapOneByte : public PdfEncodingMap
{
protected:
    using PdfEncodingMap::PdfEncodingMap;

    void AppendToUnicodeEntries(PdfObjectStream& stream) const override;

    void AppendCIDMappingEntries(PdfObjectStream& stream, const PdfFont& font) const override;
};

/**
 * A common base class for built-in encodings which are
 * known by name.
 *
 *  - StandardEncoding
 *  - SymbolEncoding
 *  - ZapfDingbatsEncoding
 *  - PdfDocEncoding (only use this for strings which are not printed
 *                    in the document. This is for meta data in the PDF).
 *
 * \see PdfStandardEncoding
 * \see PdfSymbolEncoding
 * \see PdfZapfDingbatsEncoding
 * \see PdfDocEncoding
 */
class PDFMM_API PdfBuiltInEncoding : public PdfEncodingMapOneByte
{
protected:
    PdfBuiltInEncoding(const PdfName& name);

public:
    /** Get the name of this encoding.
     *
     *  \returns the name of this encoding.
     */
    inline const PdfName& GetName() const { return m_Name; }

protected:
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
    PdfName m_Name;         // The name of the encoding
    std::unordered_map<char32_t, char> m_EncodingTable; // The helper table for conversions into this encoding
};

/** Dummy encoding map that will just throw exception
 */
class PDFMM_API PdfNullEncodingMap final : public PdfEncodingMap
{
public:
    PdfNullEncodingMap();

protected:
    bool tryGetCharCode(char32_t codePoint, PdfCharCode& codeUnit) const override;

    bool tryGetCodePoints(const PdfCharCode& codeUnit, std::vector<char32_t>& codePoints) const override;

    void AppendToUnicodeEntries(PdfObjectStream& stream) const override;

    void AppendCIDMappingEntries(PdfObjectStream& stream, const PdfFont& font) const override;
};

/** Convenience typedef for a const PdfEncoding shared ptr
 */
typedef std::shared_ptr<const PdfEncodingMap> PdfEncodingMapConstPtr;

}

#endif // PDF_ENCODING_MAP_H
