/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfCommon.h"
#include "PdfFontManager.h"

using namespace std;
using namespace mm;

#ifdef DEBUG
PDFMM_EXPORT PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Debug;
#else
PDFMM_EXPORT PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Information;
#endif // DEBUG

PDFMM_EXPORT LogMessageCallback s_LogMessageCallback;

void PdfCommon::AddFontDirectory(const string_view& path)
{
    PdfFontManager::AddFontDirectory(path);
}

void PdfCommon::SetLogMessageCallback(const LogMessageCallback& logMessageCallback)
{
    s_LogMessageCallback = logMessageCallback;
}

void PdfCommon::SetMaxLoggingSeverity(PdfLogSeverity logSeverity)
{
    s_MaxLogSeverity = logSeverity;
}

PdfLogSeverity PdfCommon::GetMaxLoggingSeverity()
{
    return s_MaxLogSeverity;
}

bool PdfCommon::IsLoggingSeverityEnabled(PdfLogSeverity logSeverity)
{
    return logSeverity <= s_MaxLogSeverity;
}
