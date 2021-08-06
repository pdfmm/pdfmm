/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// PdfError.h doesn't, and can't, include PdfDefines.h so we do so here.
// PdfDefinesPrivate.h will include PdfError.h for us.
#include <podofo/private/PdfDefinesPrivate.h>

#include <stdarg.h>
#include <stdio.h>

using namespace std;
using namespace PoDoFo;

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
    m_error = EPdfError::ErrOk;
}

PdfError::PdfError(const EPdfError& code, const char* file, int line,
    const string_view& information)
{
    this->SetError(code, file, line, information);
}

PdfError::PdfError(const EPdfError& code, const char* file, int line,
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

const PdfError& PdfError::operator=(const EPdfError& code)
{
    m_error = code;
    m_callStack.clear();

    return *this;
}

bool PdfError::operator==(const PdfError& rhs)
{
    return this->operator==(rhs.m_error);
}

bool PdfError::operator==(const EPdfError& code)
{
    return m_error == code;
}

bool PdfError::operator!=(const PdfError& rhs)
{
    return this->operator!=(rhs.m_error);
}

bool PdfError::operator!=(const EPdfError& code)
{
    return !this->operator==(code);
}

void PdfError::PrintErrorMsg() const
{
    const char* msg = PdfError::ErrorMessage(m_error);
    const char* name = PdfError::ErrorName(m_error);

    int i = 0;

    PdfError::LogErrorMessage(LogSeverity::Error, "\n\nPoDoFo encountered an error. Error: %i %s", m_error, name ? name : "");

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

const char* PdfError::ErrorName(EPdfError code)
{
    const char* msg = nullptr;

    switch (code)
    {
        case EPdfError::ErrOk:
            msg = "EPdfError::ErrOk";
            break;
        case EPdfError::TestFailed:
            msg = "EPdfError::TestFailed";
            break;
        case EPdfError::InvalidHandle:
            msg = "EPdfError::InvalidHandle";
            break;
        case EPdfError::FileNotFound:
            msg = "EPdfError::FileNotFound";
            break;
        case EPdfError::InvalidDeviceOperation:
            msg = "EPdfError::InvalidDeviceOperation";
            break;
        case EPdfError::UnexpectedEOF:
            msg = "EPdfError::UnexpectedEOF";
            break;
        case EPdfError::OutOfMemory:
            msg = "EPdfError::OutOfMemory";
            break;
        case EPdfError::ValueOutOfRange:
            msg = "EPdfError::ValueOutOfRange";
            break;
        case EPdfError::InternalLogic:
            msg = "EPdfError::InternalLogic";
            break;
        case EPdfError::InvalidEnumValue:
            msg = "EPdfError::InvalidEnumValue";
            break;
        case EPdfError::BrokenFile:
            msg = "EPdfError::BrokenFile";
            break;
        case EPdfError::PageNotFound:
            msg = "EPdfError::PageNotFound";
            break;
        case EPdfError::NoPdfFile:
            msg = "EPdfError::NoPdfFile";
            break;
        case EPdfError::NoXRef:
            msg = "EPdfError::NoXRef";
            break;
        case EPdfError::NoTrailer:
            msg = "EPdfError::NoTrailer";
            break;
        case EPdfError::NoNumber:
            msg = "EPdfError::NoNumber";
            break;
        case EPdfError::NoObject:
            msg = "EPdfError::NoObject";
            break;
        case EPdfError::NoEOFToken:
            msg = "EPdfError::NoEOFToken";
            break;
        case EPdfError::InvalidTrailerSize:
            msg = "EPdfError::InvalidTrailerSize";
            break;
        case EPdfError::InvalidLinearization:
            msg = "EPdfError::InvalidLinearization";
            break;
        case EPdfError::InvalidDataType:
            msg = "EPdfError::InvalidDataType";
            break;
        case EPdfError::InvalidXRef:
            msg = "EPdfError::InvalidXRef";
            break;
        case EPdfError::InvalidXRefStream:
            msg = "EPdfError::InvalidXRefStream";
            break;
        case EPdfError::InvalidXRefType:
            msg = "EPdfError::InvalidXRefType";
            break;
        case EPdfError::InvalidPredictor:
            msg = "EPdfError::InvalidPredictor";
            break;
        case EPdfError::InvalidStrokeStyle:
            msg = "EPdfError::InvalidStrokeStyle";
            break;
        case EPdfError::InvalidHexString:
            msg = "EPdfError::InvalidHexString";
            break;
        case EPdfError::InvalidStream:
            msg = "EPdfError::InvalidStream";
            break;
        case EPdfError::InvalidStreamLength:
            msg = "EPdfError::InvalidStream";
            break;
        case EPdfError::InvalidKey:
            msg = "EPdfError::InvalidKey";
            break;
        case EPdfError::InvalidName:
            msg = "EPdfError::InvalidName";
            break;
        case EPdfError::InvalidEncryptionDict:
            msg = "EPdfError::InvalidEncryptionDict";    /**< The encryption dictionary is invalid or misses a required key */
            break;
        case EPdfError::InvalidPassword:                    /**< The password used to open the PDF file was invalid */
            msg = "EPdfError::InvalidPassword";
            break;
        case EPdfError::InvalidFontFile:
            msg = "EPdfError::InvalidFontFile";
            break;
        case EPdfError::InvalidContentStream:
            msg = "EPdfError::InvalidContentStream";
            break;
        case EPdfError::UnsupportedFilter:
            msg = "EPdfError::UnsupportedFilter";
            break;
        case EPdfError::UnsupportedFontFormat:    /**< This font format is not supported by PoDoFO. */
            msg = "EPdfError::UnsupportedFontFormat";
            break;
        case EPdfError::ActionAlreadyPresent:
            msg = "EPdfError::ActionAlreadyPresent";
            break;
        case EPdfError::WrongDestinationType:
            msg = "EPdfError::WrongDestinationType";
            break;
        case EPdfError::MissingEndStream:
            msg = "EPdfError::MissingEndStream";
            break;
        case EPdfError::Date:
            msg = "EPdfError::Date";
            break;
        case EPdfError::Flate:
            msg = "EPdfError::Flate";
            break;
        case EPdfError::FreeType:
            msg = "EPdfError::FreeType";
            break;
        case EPdfError::SignatureError:
            msg = "EPdfError::SignatureError";
            break;
        case EPdfError::UnsupportedImageFormat:    /**< This image format is not supported by PoDoFO. */
            msg = "EPdfError::UnsupportedImageFormat";
            break;
        case EPdfError::CannotConvertColor:       /**< This color format cannot be converted. */
            msg = "EPdfError::CannotConvertColor";
            break;
        case EPdfError::NotImplemented:
            msg = "EPdfError::NotImplemented";
            break;
        case EPdfError::NotCompiled:
            msg = "EPdfError::NotCompiled";
            break;
        case EPdfError::DestinationAlreadyPresent:
            msg = "EPdfError::DestinationAlreadyPresent";
            break;
        case EPdfError::ChangeOnImmutable:
            msg = "EPdfError::ChangeOnImmutable";
            break;
        case EPdfError::OutlineItemAlreadyPresent:
            msg = "EPdfError::OutlineItemAlreadyPresent";
            break;
        case EPdfError::NotLoadedForUpdate:
            msg = "EPdfError::NotLoadedForUpdate";
            break;
        case EPdfError::CannotEncryptedForUpdate:
            msg = "EPdfError::CannotEncryptedForUpdate";
            break;
        case EPdfError::Unknown:
            msg = "EPdfError::Unknown";
            break;
        default:
            break;
    }

    return msg;
}

const char* PdfError::ErrorMessage(EPdfError code)
{
    const char* msg = nullptr;

    switch (code)
    {
        case EPdfError::ErrOk:
            msg = "No error during execution.";
            break;
        case EPdfError::TestFailed:
            msg = "An error curred in an automatic test included in PoDoFo.";
            break;
        case EPdfError::InvalidHandle:
            msg = "A nullptr handle was passed, but initialized data was expected.";
            break;
        case EPdfError::FileNotFound:
            msg = "The specified file was not found.";
            break;
        case EPdfError::InvalidDeviceOperation:
            msg = "Tried to do something unsupported to an I/O device like seek a non-seekable input device";
            break;
        case EPdfError::UnexpectedEOF:
            msg = "End of file was reached unxexpectedly.";
            break;
        case EPdfError::OutOfMemory:
            msg = "PoDoFo is out of memory.";
            break;
        case EPdfError::ValueOutOfRange:
            msg = "The passed value is out of range.";
            break;
        case EPdfError::InternalLogic:
            msg = "An internal error occurred.";
            break;
        case EPdfError::InvalidEnumValue:
            msg = "An invalid enum value was specified.";
            break;
        case EPdfError::BrokenFile:
            msg = "The file content is broken.";
            break;
        case EPdfError::PageNotFound:
            msg = "The requested page could not be found in the PDF.";
            break;
        case EPdfError::NoPdfFile:
            msg = "This is not a PDF file.";
            break;
        case EPdfError::NoXRef:
            msg = "No XRef table was found in the PDF file.";
            break;
        case EPdfError::NoTrailer:
            msg = "No trailer was found in the PDF file.";
            break;
        case EPdfError::NoNumber:
            msg = "A number was expected but not found.";
            break;
        case EPdfError::NoObject:
            msg = "A object was expected but not found.";
            break;
        case EPdfError::NoEOFToken:
            msg = "No EOF Marker was found in the PDF file.";
            break;

        case EPdfError::InvalidTrailerSize:
        case EPdfError::InvalidLinearization:
        case EPdfError::InvalidDataType:
        case EPdfError::InvalidXRef:
        case EPdfError::InvalidXRefStream:
        case EPdfError::InvalidXRefType:
        case EPdfError::InvalidPredictor:
        case EPdfError::InvalidStrokeStyle:
        case EPdfError::InvalidHexString:
        case EPdfError::InvalidStream:
        case EPdfError::InvalidStreamLength:
        case EPdfError::InvalidKey:
        case EPdfError::InvalidName:
            break;
        case EPdfError::InvalidEncryptionDict:
            msg = "The encryption dictionary is invalid or misses a required key.";
            break;
        case EPdfError::InvalidPassword:
            msg = "The password used to open the PDF file was invalid.";
            break;
        case EPdfError::InvalidFontFile:
            msg = "The font file is invalid.";
            break;
        case EPdfError::InvalidContentStream:
            msg = "The content stream is invalid due to mismatched context pairing or other problems.";
            break;
        case EPdfError::UnsupportedFilter:
            break;
        case EPdfError::UnsupportedFontFormat:
            msg = "This font format is not supported by PoDoFO.";
            break;
        case EPdfError::DestinationAlreadyPresent:
        case EPdfError::ActionAlreadyPresent:
            msg = "Outlines can have either destinations or actions.";
            break;
        case EPdfError::WrongDestinationType:
            msg = "The requested field is not available for the given destination type";
            break;
        case EPdfError::MissingEndStream:
        case EPdfError::Date:
            break;
        case EPdfError::Flate:
            msg = "ZLib returned an error.";
            break;
        case EPdfError::FreeType:
            msg = "FreeType returned an error.";
            break;
        case EPdfError::SignatureError:
            msg = "The signature contains an error.";
            break;
        case EPdfError::UnsupportedImageFormat:
            msg = "This image format is not supported by PoDoFO.";
            break;
        case EPdfError::CannotConvertColor:
            msg = "This color format cannot be converted.";
            break;
        case EPdfError::ChangeOnImmutable:
            msg = "Changing values on immutable objects is not allowed.";
            break;
        case EPdfError::NotImplemented:
            msg = "This feature is currently not implemented.";
            break;
        case EPdfError::NotCompiled:
            msg = "This feature was disabled during compile time.";
            break;
        case EPdfError::OutlineItemAlreadyPresent:
            msg = "Given OutlineItem already present in destination tree.";
            break;
        case EPdfError::NotLoadedForUpdate:
            msg = "The document had not been loaded for update.";
            break;
        case EPdfError::CannotEncryptedForUpdate:
            msg = "Cannot load encrypted documents for update.";
            break;
        case EPdfError::Unknown:
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

void PdfError::SetError(const EPdfError& code, const char* file, int line, const char* information)
{
    m_error = code;
    this->AddToCallstack(file, line, information);
}

void PdfError::AddToCallstack(const char* file, int line, const char* information)
{
    m_callStack.push_front(PdfErrorInfo(line, file, information));
}

void PdfError::SetError(const EPdfError& code, const char* file, int line, const string_view& information)
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
    return m_error != EPdfError::ErrOk;
}
