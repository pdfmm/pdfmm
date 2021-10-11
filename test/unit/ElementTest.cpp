/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "ElementTest.h"

#include <podofo.h>

using namespace PoDoFo;

CPPUNIT_TEST_SUITE_REGISTRATION(ElementTest);

void ElementTest::setUp()
{
}

void ElementTest::tearDown()
{
}

void ElementTest::testTypeToIndexAnnotation()
{
    // Check last entry in the type names array of PdfAnnotation
    PdfObject object;
    object.GetDictionary().AddKey(PdfName("Type"), PdfName("Annot"));
    object.GetDictionary().AddKey(PdfName("Subtype"), PdfName("RichMedia"));

    PdfAnnotation annot(&object, NULL);
    CPPUNIT_ASSERT_EQUAL(ePdfAnnotation_RichMedia, annot.GetType());
}

void ElementTest::testTypeToIndexAction()
{
    // Check last entry in the type names array of PdfAction
    PdfObject object;
    object.GetDictionary().AddKey(PdfName("Type"), PdfName("Action"));
    object.GetDictionary().AddKey(PdfName("S"), PdfName("GoTo3DView"));

    PdfAction action(&object);
    CPPUNIT_ASSERT_EQUAL(ePdfAction_GoTo3DView, action.GetType());
}

void ElementTest::testTypeToIndexAnnotationUnknown()
{
    // Check last entry in the type names array of PdfAnnotation
    PdfObject object;
    object.GetDictionary().AddKey(PdfName("Type"), PdfName("Annot"));
    object.GetDictionary().AddKey(PdfName("Subtype"), PdfName("PoDoFoRocksUnknownType"));

    PdfAnnotation annot(&object, NULL);
    CPPUNIT_ASSERT_EQUAL(ePdfAnnotation_Unknown, annot.GetType());
}

void ElementTest::testTypeToIndexActionUnknown()
{
    // Check last entry in the type names array of PdfAction
    PdfObject object;
    object.GetDictionary().AddKey(PdfName("Type"), PdfName("Action"));
    object.GetDictionary().AddKey(PdfName("S"), PdfName("PoDoFoRocksUnknownType"));

    PdfAction action(&object);
    CPPUNIT_ASSERT_EQUAL(ePdfAction_Unknown, action.GetType());
}
