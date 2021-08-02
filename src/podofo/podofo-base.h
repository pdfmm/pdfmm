/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PODOFO_BASE_H
#define PODOFO_BASE_H

// Include files from PoDoFo-base

#include "base/PdfVersion.h"
#include "base/PdfDefines.h"
#include "base/Pdf3rdPtyForwardDecl.h"
#include "base/PdfArray.h"
#include "base/PdfCanvas.h"
#include "base/PdfColor.h"
#include "base/PdfContentsTokenizer.h"
#include "base/PdfPostScriptTokenizer.h"
#include "base/PdfData.h"
#include "base/PdfDataType.h"
#include "base/PdfDate.h"
#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "base/PdfEncodingFactory.h"
#include "base/PdfDifferenceEncoding.h"
#include "base/PdfIdentityEncoding.h"
#include "base/PdfFontType1Encoding.h"
#include "base/PdfPredefinedEncoding.h"
#include "base/PdfEncrypt.h"
#include "base/PdfError.h"
#include "base/PdfExtension.h"
#include "base/PdfFileStream.h"
#include "base/PdfFilter.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfInputDevice.h"
#include "base/PdfInputStream.h"
#include "base/PdfLocale.h"
#include "base/PdfMemoryManagement.h"
#include "base/PdfMemStream.h"
#include "base/PdfName.h"
#include "base/PdfObject.h"
#include "base/PdfObjectStreamParser.h"
#include "base/PdfOutputDevice.h"
#include "base/PdfOutputStream.h"
#include "base/PdfParser.h"
#include "base/PdfRect.h"
#include "base/PdfRefCountedBuffer.h"
#include "base/PdfRefCountedInputDevice.h"
#include "base/PdfReference.h"
#include "base/PdfSigner.h"
#include "base/PdfStream.h"
#include "base/PdfString.h"
#include "base/PdfTokenizer.h"
#include "base/PdfVariant.h"
#include "base/PdfVecObjects.h"
#include "base/PdfWriter.h"
#include "base/PdfXRef.h"
#include "base/PdfXRefStream.h"

#endif // PODOFO_BASE_H
