/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDFMM_OPERATOR_UTILS_H
#define PDFMM_OPERATOR_UTILS_H

#include <pdfmm/base/PdfDeclarations.h>

namespace mm
{
    PDFMM_API PdfOperator GetPdfOperator(const std::string_view& opstr);
    PDFMM_API bool TryGetPdfOperator(const std::string_view& opstr, PdfOperator& op);

    /** Get the operands count of the operator
     * \returns count the number of operand, -1 means variadic number of operands
     */
    PDFMM_API int GetOperandCount(PdfOperator op);

    /** Get the operands count of the operator
     * \param count the number of operand, -1 means variadic number of operands
     */
    PDFMM_API bool TryGetOperandCount(PdfOperator op, int& count);

    PDFMM_API std::string_view GetPdfOperatorName(PdfOperator op);
    PDFMM_API bool TryGetPdfOperatorName(PdfOperator op, std::string_view& opstr);
}

#endif // PDFMM_OPERATOR_UTILS_H
