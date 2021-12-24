/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_CACHE_H
#define PDF_FONT_CACHE_H

#include "PdfDeclarations.h"

#include <unordered_map>

#include "PdfFont.h"
#include "PdfEncodingFactory.h"

#ifdef PDFMM_HAVE_FONTCONFIG
#include "PdfFontConfigWrapper.h"
#endif

FORWARD_DECLARE_FTFACE();

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)
// To have LOGFONTW available
typedef struct HFONT__* HFONT;
#endif

namespace mm {

class PdfFontMetrics;
class PdfIndirectObjectList;

struct PdfFontSearchParams
{
    bool Bold = false;
    bool Italic = false;
    PdfAutoSelectFontOptions AutoSelectOpts = PdfAutoSelectFontOptions::None;
    bool NormalizeFontName = true;
};

struct PdfFontCreationParams
{
    PdfFontSearchParams SearchParams;
    PdfFontInitFlags InitFlags = PdfFontInitFlags::Embed;
    PdfEncoding Encoding = PdfEncodingFactory::CreateWinAnsiEncoding();
    std::string FilePath;
    unsigned short FaceIndex = 0;
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

    /** Get a font from the cache of objects loaded fonts
     *
     *  \param obj a PdfObject that is a font
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetLoadedFont(const PdfObject& obj);

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param fontName a valid fontname
     *  \param params font creation params
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetFont(const std::string_view& fontName,
        const PdfFontCreationParams& params = { });

    /** Try to search for fontmetrics from the given fontname and parameters
     *
     * \returns the found metrics. Null if not found 
     */
    static PdfFontMetricsConstPtr GetFontMetrics(const std::string_view& fontName,
        const PdfFontSearchParams& params = { });

    /** Get a font from the cache. If the font does not yet
     *  exist, add it to the cache.
     *
     *  \param face a valid freetype font face (will be free'd by pdfmm)
     *  \param encoding the encoding of the font. The font will not take ownership of this object.
     *  \param embed if true a font for embedding into
     *                 PDF will be created
     *
     *  \returns a PdfFont object or nullptr if the font could
     *           not be created or found.
     */
    PdfFont* GetFont(FT_Face face,
        const PdfEncoding& encoding = PdfEncodingFactory::CreateWinAnsiEncoding(),
        PdfFontInitFlags initFlags = PdfFontInitFlags::Embed);

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)
    PdfFont* GetFont(HFONT font,
        const PdfEncoding& encoding = PdfEncodingFactory::CreateWinAnsiEncoding(),
        PdfFontInitFlags initFlags = PdfFontInitFlags::Embed);
#endif

#ifdef PDFMM_HAVE_FONTCONFIG
    /**
     * Set wrapper for the fontconfig library.
     * Useful to avoid initializing Fontconfig multiple times.
     *
     * This setter can be called until first use of Fontconfig
     * as the library is initialized at first use.
     */
    static void SetFontConfigWrapper(const std::shared_ptr<PdfFontConfigWrapper>& fontConfig);

    static PdfFontConfigWrapper& GetFontConfigWrapper();
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
        Element(const std::string_view& fontname, PdfStandard14FontType stdType,
            const PdfEncoding& encoding, bool bold, bool italic);

        Element(const Element& rhs) = default;
        Element& operator=(const Element& rhs) = default;

        std::string FontName;
        PdfStandard14FontType StdType;
        size_t EncodingId;
        bool Bold;
        bool Italic;
    };

    struct HashElement
    {
        size_t operator()(const Element& elem) const;
    };

    struct EqualElement
    {
        bool operator()(const Element& lhs, const Element& rhs) const;
    };

    typedef std::unordered_map<Element, std::unique_ptr<PdfFont>, HashElement, EqualElement> FontCacheMap;

private:
#ifdef PDFMM_HAVE_FONTCONFIG
    static std::shared_ptr<PdfFontConfigWrapper> ensureInitializedFontConfig();
#endif // PDFMM_HAVE_FONTCONFIG

    static std::unique_ptr<charbuff> getFontData(const std::string_view& fontName,
        const PdfFontSearchParams& params);
    static std::unique_ptr<charbuff> getFontData(const std::string_view& fontName,
        std::string filepath, int faceIndex, const PdfFontSearchParams& params);
    PdfFont* getFont(const std::string_view& baseFontName, const PdfFontCreationParams& params);

    /** Create a font and put it into the fontcache
     */
    PdfFont* createFontObject(const std::string_view& fontName,
        const PdfFontMetricsConstPtr& metrics, const PdfEncoding& encoding,
        PdfFontInitFlags initFlags);

#if defined(_WIN32) && defined(PDFMM_HAVE_WIN32GDI)
    static std::unique_ptr<cmn::charbuff> getWin32FontData(const std::string_view& fontName,
        const PdfFontSearchParams& params);
#endif

private:
    PdfDocument* m_doc;
    // Sorted list of all imported fonts currently in the cache
    FontCacheMap m_fontMap;
    // Map of parsed fonts
    std::unordered_map<PdfReference, std::unique_ptr<PdfFont>> m_loadedFontMap;

#ifdef PDFMM_HAVE_FONTCONFIG
    static std::shared_ptr<PdfFontConfigWrapper> m_fontConfig;
#endif
};

};

#endif // PDF_FONT_CACHE_H
