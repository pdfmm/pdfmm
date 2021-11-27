/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#define CATCH_CONFIG_RUNNER
#include <catch.hpp>
#include <pdfmm/pdfmm.h>

using namespace mm;

int main(int argc, char* argv[])
{
    PdfError::SetMinLoggingSeverity(LogSeverity::Warning);
    return Catch::Session().run(argc, argv);
}
