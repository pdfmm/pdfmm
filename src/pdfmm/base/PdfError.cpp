/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// PdfError.h doesn't, and can't, include PdfDefines.h so we do so here.
// PdfDefinesPrivate.h will include PdfError.h for us.
#include <pdfmm/private/PdfDefinesPrivate.h>

#include <stdarg.h>
#include <stdio.h>

using namespace std;
using namespace mm;

bool PdfError::s_DgbEnabled = true;
bool PdfError::s_LogEnabled = true;

// OC 17.08.2010 New to optionally replace stderr output by a callback:
PdfError::LogMessageCallback* PdfError::m_LogMessageCallback = nullptr;

//static
PdfError::LogMessageCallback* PdfError::SetLogMessageCallback(LogMessageCallback* logMessageCallback)
{
    PdfError::LogMessageCallback* old_LogMessageCallback = m_LogMessageCallback;
    m_LogMessageCallback = logMessageCallback;
    return old_LogMessageCallback;
}

PdfErrorInfo::PdfErrorInfo()
    : m_Line(-1)
{
}

PdfErrorInfo::PdfErrorInfo(int line, const char* file, const string_view& info)
    : m_Line(line), m_File(file ? file : ""), m_Info(info)
{

}

PdfErrorInfo::PdfErrorInfo(int line, const char* file, const char* info)
    : m_Line(line), m_File(file ? file : ""), m_Info(info ? info : "")
{

}

PdfErrorInfo::PdfErrorInfo(int line, const char* file, const wchar_t* info)
    : m_Line(line), m_File(file ? file : ""), m_wInfo(info ? info : L"")
{

}
PdfErrorInfo::PdfErrorInfo(const PdfErrorInfo& rhs)
{
    this->operator=(rhs);
}

const PdfErrorInfo& PdfErrorInfo::operator=(const PdfErrorInfo& rhs)
{
    m_Line = rhs.m_Line;
    m_File = rhs.m_File;
    m_Info = rhs.m_Info;
    m_wInfo = rhs.m_wInfo;

    return *this;
}

PdfError::PdfError()
{
    m_error = PdfErrorCode::Ok;
}

PdfError::PdfError(const PdfErrorCode& code, const char* file, int line,
    const string_view& information)
{
    this->SetError(code, file, line, information);
}

PdfError::PdfError(const PdfErrorCode& code, const char* file, int line,
    const char* information)
{
    this->SetError(code, file, line, information);
}

PdfError::PdfError(const PdfError& rhs)
{
    this->operator=(rhs);
}

const PdfError& PdfError::operator=(const PdfError& rhs)
{
    m_error = rhs.m_error;
    m_callStack = rhs.m_callStack;

    return *this;
}

const PdfError& PdfError::operator=(const PdfErrorCode& code)
{
    m_error = code;
    m_callStack.clear();

    return *this;
}

bool PdfError::operator==(const PdfError& rhs)
{
    return this->operator==(rhs.m_error);
}

bool PdfError::operator==(const PdfErrorCode& code)
{
    return m_error == code;
}

bool PdfError::operator!=(const PdfError& rhs)
{
    return this->operator!=(rhs.m_error);
}

bool PdfError::operator!=(const PdfErrorCode& code)
{
    return !this->operator==(code);
}

void PdfError::PrintErrorMsg() const
{
    const char* msg = PdfError::ErrorMessage(m_error);
    const char* name = PdfError::ErrorName(m_error);

    int i = 0;

    PdfError::LogErrorMessage(LogSeverity::Error, "\n\npdfmm encountered an error. Error: %i %s", m_error, name ? name : "");

    if (msg)
        PdfError::LogErrorMessage(LogSeverity::Error, "\tError Description: %s", msg);

    if (m_callStack.size())
        PdfError::LogErrorMessage(LogSeverity::Error, "\tCallstack:");

    for (auto& info: m_callStack)
    {
        if (!info.GetFilename().empty())
            PdfError::LogErrorMessage(LogSeverity::Error, "\t#%i Error Source: %s:%i", i, info.GetFilename().c_str(), info.GetLine());

        if (!info.GetInformation().empty())
            PdfError::LogErrorMessage(LogSeverity::Error, "\t\tInformation: %s", info.GetInformation().c_str());

        if (!info.GetInformationW().empty())
            PdfError::LogErrorMessage(LogSeverity::Error, L"\t\tInformation: %s", info.GetInformationW().c_str());

        ++i;
    }


    PdfError::LogErrorMessage(LogSeverity::Error, "\n");
}

const char* PdfError::what() const
{
    return PdfError::ErrorName(m_error);
}

const char* PdfError::ErrorName(PdfErrorCode code)
{
    const char* msg = nullptr;

    switch (code)
    {
        case PdfErrorCode::Ok:
            msg = "PdfErrorCode::ErrOk";
            break;
        case PdfErrorCode::TestFailed:
            msg = "PdfErrorCode::TestFailed";
            break;
        case PdfErrorCode::InvalidHandle:
            msg = "PdfErrorCode::InvalidHandle";
            break;
        case PdfErrorCode::FileNotFound:
            msg = "PdfErrorCode::FileNotFound";
            break;
        case PdfErrorCode::InvalidDeviceOperation:
            msg = "PdfErrorCode::InvalidDeviceOperation";
            break;
        case PdfErrorCode::UnexpectedEOF:
            msg = "PdfErrorCode::UnexpectedEOF";
            break;
        case PdfErrorCode::OutOfMemory:
            msg = "PdfErrorCode::OutOfMemory";
            break;
        case PdfErrorCode::ValueOutOfRange:
            msg = "PdfErrorCode::ValueOutOfRange";
            break;
        case PdfErrorCode::InternalLogic:
            msg = "PdfErrorCode::InternalLogic";
            break;
        case PdfErrorCode::InvalidEnumValue:
            msg = "PdfErrorCode::InvalidEnumValue";
            break;
        case PdfErrorCode::BrokenFile:
            msg = "PdfErrorCode::BrokenFile";
            break;
        case PdfErrorCode::PageNotFound:
            msg = "PdfErrorCode::PageNotFound";
            break;
        case PdfErrorCode::NoPdfFile:
            msg = "PdfErrorCode::NoPdfFile";
            break;
        case PdfErrorCode::NoXRef:
            msg = "PdfErrorCode::NoXRef";
            break;
        case PdfErrorCode::NoTrailer:
            msg = "PdfErrorCode::NoTrailer";
            break;
        case PdfErrorCode::NoNumber:
            msg = "PdfErrorCode::NoNumber";
            break;
        case PdfErrorCode::NoObject:
            msg = "PdfErrorCode::NoObject";
            break;
        case PdfErrorCode::NoEOFToken:
            msg = "PdfErrorCode::NoEOFToken";
            break;
        case PdfErrorCode::InvalidTrailerSize:
            msg = "PdfErrorCode::InvalidTrailerSize";
            break;
        case PdfErrorCode::InvalidLinearization:
            msg = "PdfErrorCode::InvalidLinearization";
            break;
        case PdfErrorCode::InvalidDataType:
            msg = "PdfErrorCode::InvalidDataType";
            break;
        case PdfErrorCode::InvalidXRef:
            msg = "PdfErrorCode::InvalidXRef";
            break;
        case PdfErrorCode::InvalidXRefStream:
            msg = "PdfErrorCode::InvalidXRefStream";
            break;
        case PdfErrorCode::InvalidXRefType:
            msg = "PdfErrorCode::InvalidXRefType";
            break;
        case PdfErrorCode::InvalidPredictor:
            msg = "PdfErrorCode::InvalidPredictor";
            break;
        case PdfErrorCode::InvalidStrokeStyle:
            msg = "PdfErrorCode::InvalidStrokeStyle";
            break;
        case PdfErrorCode::InvalidHexString:
            msg = "PdfErrorCode::InvalidHexString";
            break;
        case PdfErrorCode::InvalidStream:
            msg = "PdfErrorCode::InvalidStream";
            break;
        case PdfErrorCode::InvalidStreamLength:
            msg = "PdfErrorCode::InvalidStream";
            break;
        case PdfErrorCode::InvalidKey:
            msg = "PdfErrorCode::InvalidKey";
            break;
        case PdfErrorCode::InvalidName:
            msg = "PdfErrorCode::InvalidName";
            break;
        case PdfErrorCode::InvalidEncryptionDict:
            msg = "PdfErrorCode::InvalidEncryptionDict";    /**< The encryption dictionary is invalid or misses a required key */
            break;
        case PdfErrorCode::InvalidPassword:                    /**< The password used to open the PDF file was invalid */
            msg = "PdfErrorCode::InvalidPassword";
            break;
        case PdfErrorCode::InvalidFontFile:
            msg = "PdfErrorCode::InvalidFontFile";
            break;
        case PdfErrorCode::InvalidContentStream:
            msg = "PdfErrorCode::InvalidContentStream";
            break;
        case PdfErrorCode::UnsupportedFilter:
            msg = "PdfErrorCode::UnsupportedFilter";
            break;
        case PdfErrorCode::UnsupportedFontFormat:    /**< This font format is not supported by pdfmm. */
            msg = "PdfErrorCode::UnsupportedFontFormat";
            break;
        case PdfErrorCode::ActionAlreadyPresent:
            msg = "PdfErrorCode::ActionAlreadyPresent";
            break;
        case PdfErrorCode::WrongDestinationType:
            msg = "PdfErrorCode::WrongDestinationType";
            break;
        case PdfErrorCode::MissingEndStream:
            msg = "PdfErrorCode::MissingEndStream";
            break;
        case PdfErrorCode::Date:
            msg = "PdfErrorCode::Date";
            break;
        case PdfErrorCode::Flate:
            msg = "PdfErrorCode::Flate";
            break;
        case PdfErrorCode::FreeType:
            msg = "PdfErrorCode::FreeType";
            break;
        case PdfErrorCode::SignatureError:
            msg = "PdfErrorCode::SignatureError";
            break;
        case PdfErrorCode::UnsupportedImageFormat:    /**< This image format is not supported by pdfmm. */
            msg = "PdfErrorCode::UnsupportedImageFormat";
            break;
        case PdfErrorCode::CannotConvertColor:       /**< This color format cannot be converted. */
            msg = "PdfErrorCode::CannotConvertColor";
            break;
        case PdfErrorCode::NotImplemented:
            msg = "PdfErrorCode::NotImplemented";
            break;
        case PdfErrorCode::NotCompiled:
            msg = "PdfErrorCode::NotCompiled";
            break;
        case PdfErrorCode::DestinationAlreadyPresent:
            msg = "PdfErrorCode::DestinationAlreadyPresent";
            break;
        case PdfErrorCode::ChangeOnImmutable:
            msg = "PdfErrorCode::ChangeOnImmutable";
            break;
        case PdfErrorCode::OutlineItemAlreadyPresent:
            msg = "PdfErrorCode::OutlineItemAlreadyPresent";
            break;
        case PdfErrorCode::NotLoadedForUpdate:
            msg = "PdfErrorCode::NotLoadedForUpdate";
            break;
        case PdfErrorCode::CannotEncryptedForUpdate:
            msg = "PdfErrorCode::CannotEncryptedForUpdate";
            break;
        case PdfErrorCode::Unknown:
            msg = "PdfErrorCode::Unknown";
            break;
        default:
            break;
    }

    return msg;
}

const char* PdfError::ErrorMessage(PdfErrorCode code)
{
    const char* msg = nullptr;

    switch (code)
    {
        case PdfErrorCode::Ok:
            msg = "No error during execution.";
            break;
        case PdfErrorCode::TestFailed:
            msg = "An error curred in an automatic test included in pdfmm.";
            break;
        case PdfErrorCode::InvalidHandle:
            msg = "A nullptr handle was passed, but initialized data was expected.";
            break;
        case PdfErrorCode::FileNotFound:
            msg = "The specified file was not found.";
            break;
        case PdfErrorCode::InvalidDeviceOperation:
            msg = "Tried to do something unsupported to an I/O device like seek a non-seekable input device";
            break;
        case PdfErrorCode::UnexpectedEOF:
            msg = "End of file was reached unxexpectedly.";
            break;
        case PdfErrorCode::OutOfMemory:
            msg = "pdfmm is out of memory.";
            break;
        case PdfErrorCode::ValueOutOfRange:
            msg = "The passed value is out of range.";
            break;
        case PdfErrorCode::InternalLogic:
            msg = "An internal error occurred.";
            break;
        case PdfErrorCode::InvalidEnumValue:
            msg = "An invalid enum value was specified.";
            break;
        case PdfErrorCode::BrokenFile:
            msg = "The file content is broken.";
            break;
        case PdfErrorCode::PageNotFound:
            msg = "The requested page could not be found in the PDF.";
            break;
        case PdfErrorCode::NoPdfFile:
            msg = "This is not a PDF file.";
            break;
        case PdfErrorCode::NoXRef:
            msg = "No XRef table was found in the PDF file.";
            break;
        case PdfErrorCode::NoTrailer:
            msg = "No trailer was found in the PDF file.";
            break;
        case PdfErrorCode::NoNumber:
            msg = "A number was expected but not found.";
            break;
        case PdfErrorCode::NoObject:
            msg = "A object was expected but not found.";
            break;
        case PdfErrorCode::NoEOFToken:
            msg = "No EOF Marker was found in the PDF file.";
            break;

        case PdfErrorCode::InvalidTrailerSize:
        case PdfErrorCode::InvalidLinearization:
        case PdfErrorCode::InvalidDataType:
        case PdfErrorCode::InvalidXRef:
        case PdfErrorCode::InvalidXRefStream:
        case PdfErrorCode::InvalidXRefType:
        case PdfErrorCode::InvalidPredictor:
        case PdfErrorCode::InvalidStrokeStyle:
        case PdfErrorCode::InvalidHexString:
        case PdfErrorCode::InvalidStream:
        case PdfErrorCode::InvalidStreamLength:
        case PdfErrorCode::InvalidKey:
        case PdfErrorCode::InvalidName:
            break;
        case PdfErrorCode::InvalidEncryptionDict:
            msg = "The encryption dictionary is invalid or misses a required key.";
            break;
        case PdfErrorCode::InvalidPassword:
            msg = "The password used to open the PDF file was invalid.";
            break;
        case PdfErrorCode::InvalidFontFile:
            msg = "The font file is invalid.";
            break;
        case PdfErrorCode::InvalidContentStream:
            msg = "The content stream is invalid due to mismatched context pairing or other problems.";
            break;
        case PdfErrorCode::UnsupportedFilter:
            break;
        case PdfErrorCode::UnsupportedFontFormat:
            msg = "This font format is not supported by pdfmm.";
            break;
        case PdfErrorCode::DestinationAlreadyPresent:
        case PdfErrorCode::ActionAlreadyPresent:
            msg = "Outlines can have either destinations or actions.";
            break;
        case PdfErrorCode::WrongDestinationType:
            msg = "The requested field is not available for the given destination type";
            break;
        case PdfErrorCode::MissingEndStream:
        case PdfErrorCode::Date:
            break;
        case PdfErrorCode::Flate:
            msg = "ZLib returned an error.";
            break;
        case PdfErrorCode::FreeType:
            msg = "FreeType returned an error.";
            break;
        case PdfErrorCode::SignatureError:
            msg = "The signature contains an error.";
            break;
        case PdfErrorCode::UnsupportedImageFormat:
            msg = "This image format is not supported by pdfmm.";
            break;
        case PdfErrorCode::CannotConvertColor:
            msg = "This color format cannot be converted.";
            break;
        case PdfErrorCode::ChangeOnImmutable:
            msg = "Changing values on immutable objects is not allowed.";
            break;
        case PdfErrorCode::NotImplemented:
            msg = "This feature is currently not implemented.";
            break;
        case PdfErrorCode::NotCompiled:
            msg = "This feature was disabled during compile time.";
            break;
        case PdfErrorCode::OutlineItemAlreadyPresent:
            msg = "Given OutlineItem already present in destination tree.";
            break;
        case PdfErrorCode::NotLoadedForUpdate:
            msg = "The document had not been loaded for update.";
            break;
        case PdfErrorCode::CannotEncryptedForUpdate:
            msg = "Cannot load encrypted documents for update.";
            break;
        case PdfErrorCode::Unknown:
            msg = "Error code unknown.";
            break;
        default:
            break;
    }

    return msg;
}

void PdfError::LogMessage(LogSeverity logSeverity, const char* msg, ...)
{
    if (!PdfError::LoggingEnabled())
        return;

#ifdef DEBUG
    const LogSeverity minSeverity = LogSeverity::Debug;
#else
    const ELogSeverity minSeverity = ELogSeverity::Information;
#endif // DEBUG

    if (logSeverity > minSeverity)
        return;

    va_list  args;
    va_start(args, msg);

    LogMessageInternal(logSeverity, msg, args);
    va_end(args);
}

void PdfError::LogErrorMessage(LogSeverity logSeverity, const char* msg, ...)
{
    va_list  args;
    va_start(args, msg);

    LogMessageInternal(logSeverity, msg, args);
    va_end(args);
}

void PdfError::LogMessageInternal(LogSeverity logSeverity, const char* msg, va_list& args)
{
    const char* prefix = nullptr;

    switch (logSeverity)
    {
        case LogSeverity::Error:
            break;
        case LogSeverity::Critical:
            prefix = "CRITICAL: ";
            break;
        case LogSeverity::Warning:
            prefix = "WARNING: ";
            break;
        case LogSeverity::Information:
            break;
        case LogSeverity::Debug:
            prefix = "DEBUG: ";
            break;
        case LogSeverity::None:
        case LogSeverity::Unknown:
        default:
            break;
    }

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    if (m_LogMessageCallback != nullptr)
    {
        m_LogMessageCallback->LogMessage(logSeverity, prefix, msg, args);
        return;
    }

    if (prefix)
        fprintf(stderr, "%s", prefix);

    vfprintf(stderr, msg, args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void PdfError::LogMessage(LogSeverity logSeverity, const wchar_t* msg, ...)
{
    if (!PdfError::LoggingEnabled())
        return;

#ifdef DEBUG
    const LogSeverity minSeverity = LogSeverity::Debug;
#else
    const ELogSeverity minSeverity = ELogSeverity::Information;
#endif // DEBUG

    // OC 17.08.2010 BugFix: Higher level is lower value
 // if( logSeverity < minSeverity )
    if (logSeverity > minSeverity)
        return;

    va_list  args;
    va_start(args, msg);

    LogMessageInternal(logSeverity, msg, args);
    va_end(args);
}

void PdfError::EnableLogging(bool enable)
{
    PdfError::s_LogEnabled = enable;
}

bool PdfError::LoggingEnabled()
{
    return PdfError::s_LogEnabled;
}

void PdfError::LogErrorMessage(LogSeverity logSeverity, const wchar_t* msg, ...)
{
    va_list  args;
    va_start(args, msg);

    LogMessageInternal(logSeverity, msg, args);
    va_end(args);
}

void PdfError::LogMessageInternal(LogSeverity logSeverity, const wchar_t* msg, va_list& args)
{
    const wchar_t* prefix = nullptr;

    switch (logSeverity)
    {
        case LogSeverity::Error:
            break;
        case LogSeverity::Critical:
            prefix = L"CRITICAL: ";
            break;
        case LogSeverity::Warning:
            prefix = L"WARNING: ";
            break;
        case LogSeverity::Information:
            break;
        case LogSeverity::Debug:
            prefix = L"DEBUG: ";
            break;
        case LogSeverity::None:
        case LogSeverity::Unknown:
        default:
            break;
    }

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    if (m_LogMessageCallback != nullptr)
    {
        m_LogMessageCallback->LogMessage(logSeverity, prefix, msg, args);
        return;
    }

    if (prefix)
        fwprintf(stderr, prefix);

    vfwprintf(stderr, msg, args);
}

void PdfError::DebugMessage(const char* msg, ...)
{
    if (!PdfError::DebugEnabled())
        return;

    const char* prefix = "DEBUG: ";

    va_list  args;
    va_start(args, msg);

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    if (m_LogMessageCallback != nullptr)
    {
        m_LogMessageCallback->LogMessage(LogSeverity::Debug, prefix, msg, args);
    }
    else
    {
        if (prefix)
            fprintf(stderr, "%s", prefix);

        vfprintf(stderr, msg, args);
    }

    // must call va_end after calling va_start (which allocates memory on some platforms)
    va_end(args);
}

void PdfError::EnableDebug(bool enable)
{
    PdfError::s_DgbEnabled = enable;
}

bool PdfError::DebugEnabled()
{
    return PdfError::s_DgbEnabled;
}

void PdfError::SetError(const PdfErrorCode& code, const char* file, int line, const char* information)
{
    m_error = code;
    this->AddToCallstack(file, line, information);
}

void PdfError::AddToCallstack(const char* file, int line, const char* information)
{
    m_callStack.push_front(PdfErrorInfo(line, file, information));
}

void PdfError::SetError(const PdfErrorCode& code, const char* file, int line, const string_view& information)
{
    m_error = code;
    this->AddToCallstack(file, line, information);
}

void PdfError::AddToCallstack(const char* file, int line, const string_view& information)
{
    m_callStack.push_front(PdfErrorInfo(line, file, information.data()));
}

void PdfError::SetErrorInformation(const char* information)
{
    if (m_callStack.size())
        m_callStack.front().SetInformation(information == nullptr ? "" : information);
}

void PdfError::SetErrorInformation(const wchar_t* information)
{
    if (m_callStack.size())
        m_callStack.front().SetInformation(information == nullptr ? L"" : information);
}

bool PdfError::IsError() const
{
    return m_error != PdfErrorCode::Ok;
}
