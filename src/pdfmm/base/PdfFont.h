/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_H
#define PDF_FONT_H

#include "PdfDefines.h"

#include <ostream>
#include <map>

#include "PdfTextState.h"
#include "PdfName.h"
#include "PdfEncoding.h"
#include "PdfElement.h"
#include "PdfFontMetrics.h"

namespace mm {

class PdfObject;
class PdfPage;
class PdfWriter;
class PdfCharCodeMap;

typedef std::map<unsigned, PdfCID> UsedGIDsMap;

/** Before you can draw text on a PDF document, you have to create
 *  a font object first. You can reuse this font object as often
 *  as you want.
 *
 *  Use PdfDocument::CreateFont to create a new font object.
 *  It will choose a correct subclass using PdfFontFactory.
 *
 *  This is only an abstract base class which is implemented
 *  for different font formats.
 */
class PDFMM_API PdfFont : public PdfDictionaryElement
{
    friend class PdfFontFactory;
    friend class PdfFontObject;
    friend class PdfEncoding;
    friend class PdfFontManager;
    friend class PdfFontSimple;

protected:
    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param parent parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param encoding the encoding of this font
     */
    PdfFont(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

private:
    /** Create a PdfFont based on an existing PdfObject
     * To be used by PdfFontObject, PdfFontSimple
     */
    PdfFont(PdfObject& obj, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding);

public:
    virtual ~PdfFont();

public:
    /** Create a new PdfFont object.
     *
     *  \param doc the parent of the created font.
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the created font. In case of an error, it is deleted
     *         here.
     *  \param encoding the encoding of this font.
     *  \param flags flags for font init
     *  \returns a new PdfFont object or nullptr
     */
    static std::unique_ptr<PdfFont> Create(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding, PdfFontInitFlags flags);

    /**
     * Creates a new standard 14 font object (of class PdfFontStandard14) if
     * the font name (has to include variant) is one of the standard 14 fonts.
     * The font name is to be given as specified (with an ASCII hyphen).
     *
     * \param doc the parent of the created font
     * \param std14Font standard14 font type
     * \param encoding an encoding compatible with the font
     * \param flags flags for font init
     */
    static std::unique_ptr<PdfFont> CreateStandard14(PdfDocument& doc, PdfStandard14FontType std14Font,
        const PdfEncoding& encoding, PdfFontInitFlags flags);

    /** Create a new PdfFont from an existing
     *  font in a PDF file.
     *
     *  \param obj a PDF font object
     *  \param font the created font object
     */
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfFont>& font);

public:
    /** Try get a replacement font based on this font characteristics
     *  \param substFont the created substitute font
     */
    bool TryGetSubstituteFont(std::unique_ptr<PdfFont>& substFont);

    /** Write a string to a PdfStream in a format so that it can
     *  be used with this font.
     *  This is used by PdfPainter::DrawText to display a text string.
     *  The following PDF operator will be Tj
     *
     *  \param stream the string will be appended to stream without any leading
     *                 or following whitespaces.
     *  \param str a unicode or ansi string which will be displayed
     */
    void WriteStringToStream(PdfStream& stream, const std::string_view& str) const;

    /** Write a string to a PdfStream in a format so that it can
     *  be used with this font.
     *  This is used by PdfPainter::DrawText to display a text string.
     *  The following PDF operator will be Tj
     *
     *  \param stream the string will be appended to the stream without any leading
     *                 or following whitespaces.
     *  \param str a unicode or ansi string which will be displayed
     */
    void WriteStringToStream(std::ostream& stream, const std::string_view& str) const;

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param view a text string of which the width should be calculated
     *  \returns the width in PDF units
     *  \remarks Doesn't throw if string glyphs could not be partially or totally found
     */
    double GetStringWidth(const std::string_view& view, const PdfTextState& state) const;

    /**
     * \remarks Produces a partial result also in case of failures
     */
    bool TryGetStringWidth(const std::string_view& view, const PdfTextState& state, double& width) const;

    /** Retrieve the width of a given encoded PdfString in PDF units when
     *  drawn with the current font
     *  \param view a text string of which the width should be calculated
     *  \returns the width in PDF units
     *  \remarks Doesn't throw if string glyphs could not be partially or totally found
     */
    double GetStringWidth(const PdfString& encodedStr, const PdfTextState& state) const;

    /**
     * \remarks Produces a partial result also in case of failures
     */
    bool TryGetStringWidth(const PdfString& encodedStr, const PdfTextState& state, double& width) const;

    /**
     *  \remarks Doesn't throw if characater glyph could not be found
     */
    double GetCharWidth(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing = false) const;

    bool TryGetCharWidth(char32_t codePoint, const PdfTextState& state, bool ignoreCharSpacing, double& width) const;

    bool TryGetCharWidth(char32_t codePoint, const PdfTextState& state, double& width) const;

    double GetDefaultCharWidth(const PdfTextState& state, bool ignoreCharSpacing = false) const;

    /** Optional function to map a CID to a GID
     *
     * Example for /Type2 CID fonts may have a /CIDToGIDMap
     */
    virtual bool TryMapCIDToGID(unsigned cid, unsigned& gid) const;

    /** Optional function to map a GID to a CID
     *
     * Example for /Type2 CID fonts may have a /CIDToGIDMap
     */
    virtual bool TryMapGIDToCID(unsigned gid, unsigned& cid) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    double GetLineSpacing(const PdfTextState& state) const;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    double GetUnderlineThickness(const PdfTextState& state) const;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetUnderlinePosition(const PdfTextState& state) const;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetStrikeOutPosition(const PdfTextState& state) const;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    double GetStrikeOutThickness(const PdfTextState& state) const;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *
     *  \see GetAscent
     */
    double GetAscent(const PdfTextState& state) const;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    double GetDescent(const PdfTextState& state) const;

    virtual bool SupportsSubsetting() const;

    virtual PdfFontType GetType() const = 0;

    bool IsStandard14Font() const;

    bool IsStandard14Font(PdfStandard14FontType& std14Font) const;

    /** Extract base font name, removing known bold/italic/subset prefixes/suffixes
     */
    static std::string ExtractBaseName(const std::string_view& fontName, bool& isBold, bool& isItalic);

    /** Extract base font name, removing known bold/italic/subset prefixes/suffixes
     */
    static std::string ExtractBaseName(const std::string_view& fontName);

    static std::string_view GetStandard14FontName(PdfStandard14FontType stdFont);

    /** Determine if font name is a Standard14 font
     *
     * By default use both standard names and alternative ones (Arial, TimesNewRoman, CourierNew)
     * \param fontName the unprocessed font name
     */
    static bool IsStandard14Font(const std::string_view& fontName, PdfStandard14FontType& stdFont);

    /** Determine if font name is a Standard14 font
     * \param fontName the unprocessed font name
     */
    static bool IsStandard14Font(const std::string_view& fontName, bool useAltNames, PdfStandard14FontType& stdFont);

public:
    /** True if the font is CID keyed
     */
    bool IsCIDKeyed() const;

    /**
     * True if the font is loaded from a PdfObject
     */
    inline bool IsObjectLoaded() const { return m_IsObjectLoaded; }

    /** Check if this is a subsetting font.
     * \returns true if this is a subsetting font
     */
    inline bool IsSubsettingEnabled() const { return m_SubsettingEnabled; }

    inline bool IsEmbeddingEnabled() const { return m_EmbeddingEnabled; }

    /**
     * \returns empty string or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
    inline const std::string& GetSubsetPrefix() const { return m_SubsetPrefix; }

    /** Returns the identifier of this font how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Ft13)
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    /** Returns a reference to the fonts encoding
     *  \returns a PdfEncoding object.
     */
    inline const PdfEncoding& GetEncoding() const { return *m_Encoding; }

    /** Returns a handle to the fontmetrics object of this font.
     *  This can be used for size calculations of text strings when
     *  drawn using this font.
     *  \returns a handle to the font metrics object
     */
    inline const PdfFontMetrics& GetMetrics() const { return *m_Metrics; }

    /** Get the base font name of this font
     *
     *  \returns the font name, usually the /BaseFont key of a type1 or type0 font,
     *  or the /FontName in a font descriptor
     */
    inline const std::string& GetName() const { return m_Name; }

    const UsedGIDsMap& GetUsedGIDs() const { return m_UsedGIDs; }

    PdfObject& GetDescendantFontObject();

protected:
    virtual PdfObject* getDescendantFontObject();

    /**
     * Get the raw width of a CID identifier
     */
    double GetCIDWidthRaw(unsigned cid) const;

    void GetBoundingBox(PdfArray& arr) const;

    /** Fill the /FontDescriptor object dictionary
     */
    void FillDescriptor(PdfDictionary& dict) const;

    /** Inititialization tasks for imported/created from scratch fonts
     */
    virtual void initImported();

    virtual void embedFont();

    virtual void embedFontSubset();

private:
    /** Embeds pending subset-font into PDF page
     *  Only call if IsSubsetting() returns true. Might throw an exception otherwise.
     *
     *  \see IsSubsetting
     */
    void EmbedFontSubset();

    /** Add glyph to used in case of subsetting
     *  It either maps them using the font encoding or generate a new code
     *
     *  \param gid the gid to add
     *  \param mappedCodePoints code points mapped by this gid. May be a single
     *      code point or a ligature
     *  \return A mapped CID
     *  \remarks To be called by PdfEncoding. In case of a ligature
     *      parameter, it must match entirely
     */
    PdfCID AddUsedGID(unsigned gid, const cspan<char32_t>& mappedCodePoints);

    PdfFont(const PdfFont& rhs) = delete;

    /**
     * Perform inititialization tasks for fonts imported or created
     * from scratch
     */
    void InitImported(bool embeddingEnabled, bool subsettingEnabled);

    void initBase(const PdfEncoding& encoding);

    double getStringWidth(const std::vector<PdfCID>& cids, const PdfTextState& state) const;

    double getCIDWidth(unsigned cid, const PdfTextState& state, bool ignoreCharSpacing) const;

    static std::unique_ptr<PdfFont> createFontForType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding, PdfFontFileType type);

private:
    std::string m_Name;
    std::string m_SubsetPrefix;
    bool m_EmbeddingEnabled;
    bool m_IsEmbedded;
    bool m_SubsettingEnabled;
    UsedGIDsMap m_UsedGIDs;

protected:
    PdfFontMetricsConstPtr m_Metrics;
    std::unique_ptr<PdfEncoding> m_Encoding;
    bool m_IsObjectLoaded; // Font is loaded from object, as opposed as being imported
    std::shared_ptr<PdfCharCodeMap> m_DynCharCodeMap;

    PdfName m_Identifier;
};

};

#endif // PDF_FONT_H

