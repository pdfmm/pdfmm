/**
 * Copyright (C) 2022 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_COMMON_H

#include "PdfDeclarations.h"

namespace mm {

class PDFMM_API PdfCommon final
{
    PdfCommon() = delete;

public:
    static void AddFontDirectory(const std::string_view& path);

    /** Set a global static LogMessageCallback functor to replace stderr output in LogMessageInternal.
     *  \param logMessageCallback the pointer to the new callback functor object
     *  \returns the pointer to the previous callback functor object
     */
    static void SetLogMessageCallback(const LogMessageCallback& logMessageCallback);

    /** Set the maximum logging severity.
     * The higher the maximum (enum integral value), the more is logged
     */
    static void SetMaxLoggingSeverity(PdfLogSeverity logSeverity);

    /** Get the maximum logging severity
     * The higher the maximum (enum integral value), the more is logged
     */
    static PdfLogSeverity GetMaxLoggingSeverity();

    /** The if the given logging severity enabled or not
     */
    static bool IsLoggingSeverityEnabled(PdfLogSeverity logSeverity);
};

}

#define PDF_COMMON_H

#endif // PDF_COMMON_H
