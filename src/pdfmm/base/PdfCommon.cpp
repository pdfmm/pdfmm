/**
 * Copyright (C) 2022 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfCommon.h"

using namespace std;
using namespace mm;


#ifdef DEBUG
PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Debug;
#else
PdfLogSeverity s_MaxLogSeverity = PdfLogSeverity::Information;
#endif // DEBUG


static LogMessageCallback s_LogMessageCallback;

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

void mm::LogMessage(PdfLogSeverity logSeverity, const string_view& msg)
{
    if (logSeverity > s_MaxLogSeverity)
        return;

    if (s_LogMessageCallback == nullptr)
    {
        string_view prefix;
        bool ouputstderr = false;
        switch (logSeverity)
        {
            case PdfLogSeverity::Error:
                prefix = "ERROR: ";
                ouputstderr = true;
                break;
            case PdfLogSeverity::Warning:
                prefix = "WARNING: ";
                ouputstderr = true;
                break;
            case PdfLogSeverity::Debug:
                prefix = "DEBUG: ";
                break;
            case PdfLogSeverity::Information:
                break;
            default:
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
        }

        ostream* stream;
        if (ouputstderr)
            stream = &cerr;
        else
            stream = &cout;

        if (!prefix.empty())
            *stream << prefix;

        *stream << msg << endl;
    }
    else
    {
        s_LogMessageCallback(logSeverity, msg);
    }
}
