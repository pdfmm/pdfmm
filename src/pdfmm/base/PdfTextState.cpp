/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfTextState.h"

using namespace mm;

PdfTextState::PdfTextState() :
    Font(nullptr),
    FontSize(-1),
    FontScale(1),
    CharSpacing(0),
    WordSpacing(0),
    RenderingMode(PdfTextRenderingMode::Fill) { }
