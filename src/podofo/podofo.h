/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PODOFO_H
#define PODOFO_H

/**
 * This file can be used in client applications to include
 * all files required by PoDoFo at once.
 *
 * Some symbols may be declared in the PoDoFo::NonPublic namespace.
 * Client applications must never rely on or use these symbols directly.
 * On supporting platforms they will be excluded from the DLL interface,
 * and they are not guaranteed to continue to exist.
 */

#include "podofo-base.h"

// Include files from PoDoFo-doc
#include "doc/PdfAcroForm.h"
#include "doc/PdfAction.h"
#include "doc/PdfAnnotation.h"
#include "doc/PdfContents.h"
#include "doc/PdfDestination.h"
#include "doc/PdfDocument.h"
#include "doc/PdfElement.h"
#include "doc/PdfExtGState.h"
#include "doc/PdfField.h"
#include "doc/PdfTextBox.h"
#include "doc/PdfButton.h"
#include "doc/PdfCheckBox.h"
#include "doc/PdfButton.h"
#include "doc/PdfPushButton.h"
#include "doc/PdfCheckBox.h"
#include "doc/PdfRadioButton.h"
#include "doc/PdfChoiceField.h"
#include "doc/PdfComboBox.h"
#include "doc/PdfListBox.h"
#include "doc/PdfSignature.h"
#include "doc/PdfFileSpec.h"
#include "doc/PdfFontCache.h"
#include "doc/PdfFontCIDTrueType.h"
#include "doc/PdfFontFactory.h"
#include "doc/PdfFont.h"
#include "doc/PdfFontMetricsBase14.h"
#include "doc/PdfFontMetricsFreetype.h"
#include "doc/PdfFontMetrics.h"
#include "doc/PdfFontMetricsObject.h"
#include "doc/PdfFontSimple.h"
#include "doc/PdfFontTrueType.h"
#include "doc/PdfFontTrueTypeSubset.h"
#include "doc/PdfFontType1Base14.h"
#include "doc/PdfFontType1.h"
#include "doc/PdfFontType3.h"
#include "doc/PdfFunction.h"
#include "doc/PdfImage.h"
#include "doc/PdfInfo.h"
#include "doc/PdfMemDocument.h"
#include "doc/PdfNamesTree.h"
#include "doc/PdfOutlines.h"
#include "doc/PdfPage.h"
#include "doc/PdfPagesTreeCache.h"
#include "doc/PdfPagesTree.h"
#include "doc/PdfPainter.h"
#include "doc/PdfShadingPattern.h"
#include "doc/PdfStreamedDocument.h"
#include "doc/PdfTilingPattern.h"
#include "doc/PdfXObject.h"

#endif // PODOFO_H


