/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef FONT_TEST_H
#define FONT_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <podofo.h>

#if defined(PODOFO_HAVE_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#endif

/** This test tests the various PdfFont classes
 */
class FontTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(FontTest);
#if defined(PODOFO_HAVE_FONTCONFIG)
    CPPUNIT_TEST(testFonts);
    CPPUNIT_TEST(testCreateFontFtFace);
#endif
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

#if defined(PODOFO_HAVE_FONTCONFIG)
    void testFonts();
    void testCreateFontFtFace();
#endif

private:
#if defined(PODOFO_HAVE_FONTCONFIG)
    void testSingleFont(FcPattern* pFont, FcConfig* pConfig);

    bool GetFontInfo(FcPattern* pFont, std::string& rsFamily, std::string& rsPath,
        bool& rbBold, bool& rbItalic);
#endif

private:
    PoDoFo::PdfMemDocument* m_pDoc;
    PoDoFo::PdfVecObjects* m_pVecObjects;
    PoDoFo::PdfFontCache* m_pFontCache;

};

#endif // FONT_TEST_H
