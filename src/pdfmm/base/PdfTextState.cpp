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
    m_FontSize(12),
    m_FontScale(1),
    m_CharSpace(0),
    m_WordSpace(0),
    m_Underlined(false),
    m_StrikedOut(false)
{
}
