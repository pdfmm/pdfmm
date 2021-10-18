/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDFMM_H
#define PDFMM_H

/**
 * This file can be used in client applications to include
 * all files required by pdfmm at once.
 */

// Disable unit test friendship
#ifdef PDFMM_UNIT_TEST
#undef PDFMM_UNIT_TEST
#endif
#define PDFMM_UNIT_TEST(classname)

#include "pdfmm-base.h"
#include "pdfmm-contrib.h"

#endif // PDFMM_H
