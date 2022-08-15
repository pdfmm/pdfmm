#ifndef PDF_TEST_H
#define PDF_TEST_H

#include "catch.hpp"

#define PDFMM_UNIT_TEST(classname) friend class classname
#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include <pdfmm/pdfmm.h>

#endif // PDF_TEST_H
