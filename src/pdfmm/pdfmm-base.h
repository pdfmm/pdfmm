/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDFMM_BASE_H
#define PDFMM_BASE_H

#include "base/PdfVersion.h"
#include "base/PdfDeclarations.h"
#include "base/PdfOperatorUtils.h"
#include "base/PdfArray.h"
#include "base/PdfCanvas.h"
#include "base/PdfColor.h"
#include "base/PdfContentsReader.h"
#include "base/PdfPostScriptTokenizer.h"
#include "base/PdfData.h"
#include "base/PdfDataProvider.h"
#include "base/PdfDate.h"
#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "base/PdfCMapEncoding.h"
#include "base/PdfEncodingFactory.h"
#include "base/PdfDifferenceEncoding.h"
#include "base/PdfIdentityEncoding.h"
#include "base/PdfFontType1Encoding.h"
#include "base/PdfPredefinedEncoding.h"
#include "base/PdfEncrypt.h"
#include "base/PdfError.h"
#include "base/PdfExtension.h"
#include "base/PdfFileObjectStream.h"
#include "base/PdfFilter.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfInputDevice.h"
#include "base/PdfInputStream.h"
#include "base/PdfLocale.h"
#include "base/PdfMemoryObjectStream.h"
#include "base/PdfName.h"
#include "base/PdfObject.h"
#include "base/PdfObjectStreamParser.h"
#include "base/PdfOutputDevice.h"
#include "base/PdfOutputStream.h"
#include "base/PdfParser.h"
#include "base/PdfRect.h"
#include "base/PdfReference.h"
#include "base/PdfSigner.h"
#include "base/PdfObjectStream.h"
#include "base/PdfString.h"
#include "base/PdfTokenizer.h"
#include "base/PdfVariant.h"
#include "base/PdfIndirectObjectList.h"
#include "base/PdfWriter.h"
#include "base/PdfXRef.h"
#include "base/PdfXRefStream.h"
#include "base/PdfAcroForm.h"
#include "base/PdfAction.h"
#include "base/PdfAnnotation.h"
#include "base/PdfContents.h"
#include "base/PdfDestination.h"
#include "base/PdfDocument.h"
#include "base/PdfElement.h"
#include "base/PdfExtGState.h"
#include "base/PdfField.h"
#include "base/PdfTextBox.h"
#include "base/PdfButton.h"
#include "base/PdfCheckBox.h"
#include "base/PdfButton.h"
#include "base/PdfPushButton.h"
#include "base/PdfCheckBox.h"
#include "base/PdfRadioButton.h"
#include "base/PdfChoiceField.h"
#include "base/PdfComboBox.h"
#include "base/PdfListBox.h"
#include "base/PdfSignature.h"
#include "base/PdfFileSpec.h"
#include "base/PdfFontManager.h"
#include "base/PdfFontCIDTrueType.h"
#include "base/PdfFont.h"
#include "base/PdfFontMetricsStandard14.h"
#include "base/PdfFontMetricsFreetype.h"
#include "base/PdfFontMetrics.h"
#include "base/PdfFontMetricsObject.h"
#include "base/PdfFontSimple.h"
#include "base/PdfFontTrueType.h"
#include "base/PdfFontTrueTypeSubset.h"
#include "base/PdfFontType1.h"
#include "base/PdfFontType3.h"
#include "base/PdfImage.h"
#include "base/PdfInfo.h"
#include "base/PdfMemDocument.h"
#include "base/PdfNameTree.h"
#include "base/PdfOutlines.h"
#include "base/PdfPage.h"
#include "base/PdfPageTreeCache.h"
#include "base/PdfPageTree.h"
#include "base/PdfPainter.h"
#include "base/PdfStreamedDocument.h"
#include "base/PdfXObject.h"
#include "base/PdfXObjectForm.h"
#include "base/PdfXObjectPostScript.h"

#endif // PDFMM_BASE_H
