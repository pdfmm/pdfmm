/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfGraphicsState.h"

using namespace std;
using namespace mm;

PdfGraphicsState::PdfGraphicsState() :
    LineWidth(0),
    MiterLimit(10),
    LineCapStyle(PdfLineCapStyle::Square),
    LineJoinStyle(PdfLineJoinStyle::Miter)
{
}
