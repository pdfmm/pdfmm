/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfTrailer.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfTrailer::PdfTrailer(PdfObject& obj)
    : PdfDictionaryElement(obj) { }
