/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfExtension.h"

using namespace std;
using namespace mm;

PdfExtension::PdfExtension(const std::string_view& ns, PdfVersion baseVersion, int64_t level) :
    m_Ns(ns), m_BaseVersion(baseVersion), m_Level(level) { }
