#ifndef PDFMM_OPERATOR_UTILS_H
#define PDFMM_OPERATOR_UTILS_H

#include <pdfmm/base/PdfDeclarations.h>

namespace mm
{
    bool TryGetPDFOperator(const std::string_view& opstr, PdfContentOperator& op);
    bool TryGetString(PdfContentOperator op, std::string_view& opstr);
}

#endif // PDFMM_OPERATOR_UTILS_H
