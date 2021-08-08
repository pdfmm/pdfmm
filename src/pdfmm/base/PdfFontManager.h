/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_CACHE_H
#define PDF_FONT_CACHE_H

#include "PdfDefines.h"

#include <unordered_map>

#include "PdfFont.h"
#include "Pdf3rdPtyForwardDecl.h"
#include "PdfEncodingFactory.h"

#ifdef PDFMM_HAVE_FONTCONFIG
#include "PdfFontConfigWrapper.h"
#endif

#ifdef WIN32

// To have LOGFONTW available
typedef struct tagLOGFONTW LOGFONTW;

#endif // WIN32

namespace mm {

class PdfFontMetrics;
class PdfIndirectObjectList;

/**
 * Flags to control font creation.
 */
enum class PdfFontCreationFlags
{
    None = 0,                   ///< No special settings
    AutoSelectStandard14 = 1,   ///< Create automatically a Standard14 font if the fontname matches one of them
    DoSubsetting = 2            ///< Create subsetted, which includes only used characters
};

struct PdfFontCreationParams
{
    bool Bold = false;
    bool Italic = false;
    bool IsSymbolCharset = false; // CHECK-ME: Migrate this to a flag?
    PdfFontCreationFlags Flags = PdfFontCreationFlags::None;
    bool Embed = true;
    PdfEncoding Encoding = PdfEncodingFactory::CreateWinAnsiEncoding();
    std::string FilePath;
};

/**
 * This class assists PdfDocument
 * with caching font information.
 *
 * Additional to font caching, this class is also
 * responsible for font matching.
 *
 * PdfFont is an actual font that can be used in
 * a PDF file (i.e. it does also font embedding)
 * and PdfFontMetrics provides only metrics informations.
 *
 * \see PdfDocument
 */
class PDFMM_API PdfFontManager final
{
    friend class PdfDocument;
    friend class PdfMemDocument;
    friend class PdfStreamedDocument;

    PdfFontManager(const PdfFontManager&) = delete;
    PdfFontManager& operator=(const PdfFontManager&) = delete;

public:
    /** Destroy and empty the font cache
     */
    ~PdfFontManager();

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache. This font is created
     *  from an existing object.
     *
     *  \param obj a PdfObject that is a font
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetFont(PdfObject& obj);

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param fontName a valid fontname
     *  \param params font creation params
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetFont(const std::string_view& fontName, const PdfFontCreationParams& params = { });

    /** Get a fontsubset from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param fontName a valid fontname
     *  \param params params for font creation
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetFontSubset(const std::string_view& fontName, const PdfFontCreationParams& params = { });

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param face a valid freetype font face (will be free'd by pdfmm)
     *  \param encoding the encoding of the font. The font will not take ownership of this object.
     *  \param isSymbolCharset whether to use a symbol charset
     *  \param embed if true a font for embedding into
     *                 PDF will be created
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetFont(FT_Face face,
        const PdfEncoding& encoding = PdfEncodingFactory::CreateWinAnsiEncoding(),
        bool isSymbolCharset = false,
        bool embed = false);

#ifdef WIN32

    PdfFont* GetFont(const LOGFONTW& logFont,
        const PdfEncoding& encoding = PdfEncodingFactory::CreateWinAnsiEncoding(),
        bool embed = true);

#endif // WIN32

    /** Returns the internal handle to the freetype library from font cache
     */
    inline FT_Library GetFontLibrary() const { return this->m_ftLibrary; }

#ifdef PDFMM_HAVE_FONTCONFIG

    /**
     * Set wrapper for the fontconfig library.
     * Useful to avoid initializing Fontconfig multiple times.
     *
     * This setter can be called until first use of Fontconfig
     * as the library is initialized at first use.
     */
    void SetFontConfigWrapper(PdfFontConfigWrapper* fontConfig);

#endif // PDFMM_HAVE_FONTCONFIG

    // These methods are available for PdfDocument only
private:
    PdfFontManager(PdfDocument& doc);

    /** Embeds all pending subset-fonts
     */
    void EmbedSubsetFonts();

    /**
     * Empty the internal font cache.
     * This should be done when ever a new document
     * is created or openened.
     */
    void EmptyCache();

private:
    /** A private structure,
     *  which represents a font in the cache.
     */
    struct Element
    {
        Element(const std::string_view& fontname, const PdfEncoding& encoding,
            bool bold, bool italic, bool isSymbolCharset);

        Element(const Element& rhs);

        const Element& operator=(const Element& rhs);

        std::string FontName;
        size_t EncodingId;
        bool Bold;
        bool Italic;
        bool IsSymbolCharset;
    };

    struct HashElement
    {
        size_t operator()(const Element& elem) const;
    };

    struct EqualElement
    {
        bool operator()(const Element& lhs, const Element& rhs) const;
    };

    typedef std::unordered_map<Element, PdfFont*, HashElement, EqualElement> FontCacheMap;

private:
    /**
     * Get the path to a font file for a certain fontname
     */
    std::string GetFontPath(const std::string_view& fontName, bool bold, bool italic);

    /** Create a font and put it into the fontcache
     */
    PdfFont* CreateFontObject(FontCacheMap& map, const std::string_view& fontName,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding,
        bool bold, bool italic, bool embed, bool subsetting);

#ifdef WIN32
    PdfFont* GetWin32Font(FontCacheMap& map, const std::string_view& fontName,
        const PdfEncoding& encoding, bool bold, bool italic, bool symbolCharset,
        bool embed, bool subsetting);

    PdfFont* GetWin32Font(FontCacheMap& map, const std::string_view& fontName,
        const LOGFONTW& logFont, const PdfEncoding& encoding,
        bool embed, bool subsetting);
#endif // WIN32

    void Init();

private:
    FontCacheMap m_fontMap;             // Sorted list of all fonts, currently in the cache
    FontCacheMap m_fontSubsetMap;
    FT_Library m_ftLibrary;                 // Handle to the freetype library
    PdfDocument* m_doc;                     // Handle to parent for creating new fonts and objects

#ifdef PDFMM_HAVE_FONTCONFIG
    PdfFontConfigWrapper* m_fontConfig;
#endif
};

};

ENABLE_BITMASK_OPERATORS(mm::PdfFontCreationFlags);

#endif // PDF_FONT_CACHE_H
