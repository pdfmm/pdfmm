/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_FONT_CACHE_H
#define PDF_FONT_CACHE_H

#include "PdfDeclarations.h"

#include "PdfFont.h"
#include "PdfEncodingFactory.h"

#ifdef PODOFO_HAVE_FONTCONFIG
#include "PdfFontConfigWrapper.h"
#endif

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
// To have LOGFONTW available
typedef struct HFONT__* HFONT;
#endif

namespace PoDoFo {

class PdfIndirectObjectList;

struct PdfFontSearchParams
{
    PdfFontStyle Style = PdfFontStyle::Regular;
    PdfFontAutoSelectBehavior AutoSelect = PdfFontAutoSelectBehavior::None;
    PdfFontMatchBehaviorFlags MatchBehavior = PdfFontMatchBehaviorFlags::None;
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
class PODOFO_API PdfFontManager final
{
    friend class PdfDocument;
    friend class PdfMemDocument;
    friend class PdfStreamedDocument;
    friend class PdfFont;
    friend class PdfCommon;

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
        const PdfFontSearchParams& searchParams = { }, const PdfFontCreateParams& createParams = { });

    PdfFont* GetFont(const std::string_view& fontName, const PdfFontCreateParams& createParams);

    PdfFont* GetStandard14Font(PdfStandard14FontType stdFont,
        const PdfFontCreateParams& params = { });

    /**
     * \param face a valid freetype font face. The face is
     *        referenced and the font data is copied
     * \param params font creation params
     *
     * \returns a PdfFont object or nullptr if the font could
     *          not be created or found.
     */
    PdfFont* GetFont(FT_Face face, const PdfFontCreateParams& params = { });

    /** Try to search for fontmetrics from the given fontname and parameters
     *
     * \returns the found metrics. Null if not found
     */
    static PdfFontMetricsConstPtr GetFontMetrics(const std::string_view& fontName,
        const PdfFontSearchParams& params = { });

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
    PdfFont* GetFont(HFONT font, const PdfFontCreateParams& params = { });
#endif

#ifdef PODOFO_HAVE_FONTCONFIG
    /**
     * Set wrapper for the fontconfig library.
     * Useful to avoid initializing Fontconfig multiple times.
     *
     * This setter can be called until first use of Fontconfig
     * as the library is initialized at first use.
     */
    static void SetFontConfigWrapper(const std::shared_ptr<PdfFontConfigWrapper>& fontConfig);

    static PdfFontConfigWrapper& GetFontConfigWrapper();
#endif // PODOFO_HAVE_FONTCONFIG

    /** Called by PdfDocument before saving
     */
    void EmbedFonts();

    // These methods are reserved to use to selected friend classes
private:
    PdfFontManager(PdfDocument& doc);

    /**
     * Empty the internal font cache.
     * This should be done when ever a new document
     * is created or openened.
     */
    void Clear();

    PdfFont* AddImported(std::unique_ptr<PdfFont>&& font);

    /** Returns a new ABCDEF+ like font subset prefix
     */
    std::string GenerateSubsetPrefix();

    static void AddFontDirectory(const std::string_view& path);

private:
    /** A private structure, which represents a cached font
     */
    struct Descriptor
    {
        Descriptor(const std::string_view& fontname, PdfStandard14FontType stdType,
            const PdfEncoding& encoding, PdfFontStyle style);

        Descriptor(const Descriptor& rhs) = default;
        Descriptor& operator=(const Descriptor& rhs) = default;

        std::string FontName;
        PdfStandard14FontType StdType;
        size_t EncodingId;
        PdfFontStyle Style;
    };

    struct HashElement
    {
        size_t operator()(const Descriptor& elem) const;
    };

    struct EqualElement
    {
        bool operator()(const Descriptor& lhs, const Descriptor& rhs) const;
    };

    using ImportedFontMap = std::unordered_map<Descriptor, std::vector<PdfFont*>, HashElement, EqualElement>;

    struct Storage
    {
        bool IsLoaded;
        std::unique_ptr<PdfFont> Font;
    };

    using FontMap = std::unordered_map<PdfReference, Storage>;

    using FontMatcher = std::function<PdfFont*(const mspan<PdfFont*>&)>;
private:
#ifdef PODOFO_HAVE_FONTCONFIG
    static std::shared_ptr<PdfFontConfigWrapper> ensureInitializedFontConfig();
#endif // PODOFO_HAVE_FONTCONFIG

    static std::unique_ptr<charbuff> getFontData(const std::string_view& fontName,
        const PdfFontSearchParams& params);
    static std::unique_ptr<charbuff> getFontData(const std::string_view& fontName,
        std::string filepath, unsigned faceIndex, const PdfFontSearchParams& params);
    PdfFont* getImportedFont(const std::string_view& fontName, const std::string_view& baseFontName,
        const PdfFontSearchParams& searchParams, const PdfFontCreateParams& createParams);
    static std::string adaptSearchParams(const std::string_view& fontName,
        PdfFontSearchParams& searchParams);
    PdfFont* getImportedFont(const PdfFontMetricsConstPtr& metrics,
        const PdfFontCreateParams& params, const FontMatcher& matchFont);
    PdfFont* addImported(std::vector<PdfFont*>& fonts, std::unique_ptr<PdfFont>&& font);

#if defined(_WIN32) && defined(PODOFO_HAVE_WIN32GDI)
    static std::unique_ptr<charbuff> getWin32FontData(const std::string_view& fontName,
        const PdfFontSearchParams& params);
#endif

private:
    PdfDocument* m_doc;
    std::string m_currentPrefix;

    // Map of all imported fonts
    ImportedFontMap m_importedFonts;
    // Map of all fonts
    FontMap m_fonts;

#ifdef PODOFO_HAVE_FONTCONFIG
    static std::shared_ptr<PdfFontConfigWrapper> m_fontConfig;
#endif
};

};

#endif // PDF_FONT_CACHE_H
