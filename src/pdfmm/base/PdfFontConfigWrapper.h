/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */


#ifndef PDF_FONT_CONFIG_WRAPPER_H
#define PDF_FONT_CONFIG_WRAPPER_H

#include "PdfDefines.h"

#include <mutex>

#include "Pdf3rdPtyForwardDecl.h"

namespace mm {

/**
 * This class initializes and destroys the FontConfig library.
 * 
 * As initializing fontconfig can take a long time, you 
 * can create a wrapper by yourself to cache initialization of
 * fontconfig.
 *
 * This class is reference counted. The last user of the fontconfig library
 * will destroy the fontconfig handle.
 *
 * The fontconfig library is initialized on first used (lazy loading!)
 */
class PDFMM_API PdfFontConfigWrapper
{
public:
    /**
     * Create a new FontConfigWrapper and initialize the fontconfig library.
     */
    PdfFontConfigWrapper(FcConfig* fcConfig = nullptr);

    ~PdfFontConfigWrapper();

    /** Get the path of a font file on a Unix system using fontconfig
     *
     *  This method is only available if pdfmm was compiled with
     *  fontconfig support. Make sure to lock any FontConfig mutexes before
     *  calling this method by yourself!
     *
     *  \param pConfig a handle to an initialized fontconfig library
     *  \param pszFontName name of the requested font
     *  \param bBold if true find a bold font
     *  \param bItalic if true find an italic font
     *  \returns the path to the fontfile or an empty string
     */
    std::string GetFontConfigFontPath(const std::string_view fontName, bool bold, bool italic);

    FcConfig* GetFcConfig();

    static PdfFontConfigWrapper* GetInstance();

private:
    PdfFontConfigWrapper(const PdfFontConfigWrapper& rhs) = delete;
    const PdfFontConfigWrapper& operator=(const PdfFontConfigWrapper& rhs) = delete;

    /**
     * Do the lazy initialization of fontconfig
     */
    void InitializeFontConfig();

private:
    std::mutex m_mutex;
    FcConfig* m_FcConfig;
};

};

#endif // PDF_FONT_CONFIG_WRAPPER_H
