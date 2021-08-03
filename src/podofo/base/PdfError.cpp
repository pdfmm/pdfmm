/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

// PdfError.h doesn't, and can't, include PdfDefines.h so we do so here.
// PdfDefinesPrivate.h will include PdfError.h for us.
#include "PdfDefinesPrivate.h"

#include <stdarg.h>
#include <stdio.h>

using namespace std;
using namespace PoDoFo;

bool PdfError::s_DgbEnabled = true;
bool PdfError::s_LogEnabled = true;

// OC 17.08.2010 New to optionally replace stderr output by a callback:
PdfError::LogMessageCallback* PdfError::m_fLogMessageCallback = nullptr;

//static
PdfError::LogMessageCallback* PdfError::SetLogMessageCallback(LogMessageCallback* fLogMessageCallback)
{
    PdfError::LogMessageCallback* old_fLogMessageCallback = m_fLogMessageCallback;
    m_fLogMessageCallback = fLogMessageCallback;
    return old_fLogMessageCallback;
}

PdfErrorInfo::PdfErrorInfo()
    : m_nLine(-1)
{
}

PdfErrorInfo::PdfErrorInfo(int line, const char* pszFile, std::string sInfo)
    : m_nLine(line), m_sFile(pszFile ? pszFile : ""), m_sInfo(sInfo)
{

}

PdfErrorInfo::PdfErrorInfo(int line, const char* pszFile, const char* pszInfo)
    : m_nLine(line), m_sFile(pszFile ? pszFile : ""), m_sInfo(pszInfo ? pszInfo : "")
{

}

PdfErrorInfo::PdfErrorInfo(int line, const char* pszFile, const wchar_t* pszInfo)
    : m_nLine(line), m_sFile(pszFile ? pszFile : ""), m_swInfo(pszInfo ? pszInfo : L"")
{

}
PdfErrorInfo::PdfErrorInfo(const PdfErrorInfo& rhs)
{
    this->operator=(rhs);
}

const PdfErrorInfo& PdfErrorInfo::operator=(const PdfErrorInfo& rhs)
{
    m_nLine = rhs.m_nLine;
    m_sFile = rhs.m_sFile;
    m_sInfo = rhs.m_sInfo;
    m_swInfo = rhs.m_swInfo;

    return *this;
}

PdfError::PdfError()
{
    m_error = EPdfError::ErrOk;
}

PdfError::PdfError(const EPdfError& eCode, const char* pszFile, int line,
    std::string sInformation)
{
    this->SetError(eCode, pszFile, line, sInformation);
}

PdfError::PdfError(const EPdfError& eCode, const char* pszFile, int line,
    const char* pszInformation)
{
    this->SetError(eCode, pszFile, line, pszInformation);
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

const PdfError& PdfError::operator=(const EPdfError& eCode)
{
    m_error = eCode;
    m_callStack.clear();

    return *this;
}

bool PdfError::operator==(const PdfError& rhs)
{
    return this->operator==(rhs.m_error);
}

bool PdfError::operator==(const EPdfError& eCode)
{
    return m_error == eCode;
}

bool PdfError::operator!=(const PdfError& rhs)
{
    return this->operator!=(rhs.m_error);
}

bool PdfError::operator!=(const EPdfError& eCode)
{
    return !this->operator==(eCode);
}

void PdfError::PrintErrorMsg() const
{
    const char* pszMsg = PdfError::ErrorMessage(m_error);
    const char* pszName = PdfError::ErrorName(m_error);

    int i = 0;

    PdfError::LogErrorMessage(LogSeverity::Error, "\n\nPoDoFo encountered an error. Error: %i %s", m_error, pszName ? pszName : "");

    if (pszMsg)
        PdfError::LogErrorMessage(LogSeverity::Error, "\tError Description: %s", pszMsg);

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

const char* PdfError::ErrorName(EPdfError eCode)
{
    const char* pszMsg = nullptr;

    switch (eCode)
    {
        case EPdfError::ErrOk:
            pszMsg = "EPdfError::ErrOk";
            break;
        case EPdfError::TestFailed:
            pszMsg = "EPdfError::TestFailed";
            break;
        case EPdfError::InvalidHandle:
            pszMsg = "EPdfError::InvalidHandle";
            break;
        case EPdfError::FileNotFound:
            pszMsg = "EPdfError::FileNotFound";
            break;
        case EPdfError::InvalidDeviceOperation:
            pszMsg = "EPdfError::InvalidDeviceOperation";
            break;
        case EPdfError::UnexpectedEOF:
            pszMsg = "EPdfError::UnexpectedEOF";
            break;
        case EPdfError::OutOfMemory:
            pszMsg = "EPdfError::OutOfMemory";
            break;
        case EPdfError::ValueOutOfRange:
            pszMsg = "EPdfError::ValueOutOfRange";
            break;
        case EPdfError::InternalLogic:
            pszMsg = "EPdfError::InternalLogic";
            break;
        case EPdfError::InvalidEnumValue:
            pszMsg = "EPdfError::InvalidEnumValue";
            break;
        case EPdfError::BrokenFile:
            pszMsg = "EPdfError::BrokenFile";
            break;
        case EPdfError::PageNotFound:
            pszMsg = "EPdfError::PageNotFound";
            break;
        case EPdfError::NoPdfFile:
            pszMsg = "EPdfError::NoPdfFile";
            break;
        case EPdfError::NoXRef:
            pszMsg = "EPdfError::NoXRef";
            break;
        case EPdfError::NoTrailer:
            pszMsg = "EPdfError::NoTrailer";
            break;
        case EPdfError::NoNumber:
            pszMsg = "EPdfError::NoNumber";
            break;
        case EPdfError::NoObject:
            pszMsg = "EPdfError::NoObject";
            break;
        case EPdfError::NoEOFToken:
            pszMsg = "EPdfError::NoEOFToken";
            break;
        case EPdfError::InvalidTrailerSize:
            pszMsg = "EPdfError::InvalidTrailerSize";
            break;
        case EPdfError::InvalidLinearization:
            pszMsg = "EPdfError::InvalidLinearization";
            break;
        case EPdfError::InvalidDataType:
            pszMsg = "EPdfError::InvalidDataType";
            break;
        case EPdfError::InvalidXRef:
            pszMsg = "EPdfError::InvalidXRef";
            break;
        case EPdfError::InvalidXRefStream:
            pszMsg = "EPdfError::InvalidXRefStream";
            break;
        case EPdfError::InvalidXRefType:
            pszMsg = "EPdfError::InvalidXRefType";
            break;
        case EPdfError::InvalidPredictor:
            pszMsg = "EPdfError::InvalidPredictor";
            break;
        case EPdfError::InvalidStrokeStyle:
            pszMsg = "EPdfError::InvalidStrokeStyle";
            break;
        case EPdfError::InvalidHexString:
            pszMsg = "EPdfError::InvalidHexString";
            break;
        case EPdfError::InvalidStream:
            pszMsg = "EPdfError::InvalidStream";
            break;
        case EPdfError::InvalidStreamLength:
            pszMsg = "EPdfError::InvalidStream";
            break;
        case EPdfError::InvalidKey:
            pszMsg = "EPdfError::InvalidKey";
            break;
        case EPdfError::InvalidName:
            pszMsg = "EPdfError::InvalidName";
            break;
        case EPdfError::InvalidEncryptionDict:
            pszMsg = "EPdfError::InvalidEncryptionDict";    /**< The encryption dictionary is invalid or misses a required key */
            break;
        case EPdfError::InvalidPassword:                    /**< The password used to open the PDF file was invalid */
            pszMsg = "EPdfError::InvalidPassword";
            break;
        case EPdfError::InvalidFontFile:
            pszMsg = "EPdfError::InvalidFontFile";
            break;
        case EPdfError::InvalidContentStream:
            pszMsg = "EPdfError::InvalidContentStream";
            break;
        case EPdfError::UnsupportedFilter:
            pszMsg = "EPdfError::UnsupportedFilter";
            break;
        case EPdfError::UnsupportedFontFormat:    /**< This font format is not supported by PoDoFO. */
            pszMsg = "EPdfError::UnsupportedFontFormat";
            break;
        case EPdfError::ActionAlreadyPresent:
            pszMsg = "EPdfError::ActionAlreadyPresent";
            break;
        case EPdfError::WrongDestinationType:
            pszMsg = "EPdfError::WrongDestinationType";
            break;
        case EPdfError::MissingEndStream:
            pszMsg = "EPdfError::MissingEndStream";
            break;
        case EPdfError::Date:
            pszMsg = "EPdfError::Date";
            break;
        case EPdfError::Flate:
            pszMsg = "EPdfError::Flate";
            break;
        case EPdfError::FreeType:
            pszMsg = "EPdfError::FreeType";
            break;
        case EPdfError::SignatureError:
            pszMsg = "EPdfError::SignatureError";
            break;
        case EPdfError::UnsupportedImageFormat:    /**< This image format is not supported by PoDoFO. */
            pszMsg = "EPdfError::UnsupportedImageFormat";
            break;
        case EPdfError::CannotConvertColor:       /**< This color format cannot be converted. */
            pszMsg = "EPdfError::CannotConvertColor";
            break;
        case EPdfError::NotImplemented:
            pszMsg = "EPdfError::NotImplemented";
            break;
        case EPdfError::NotCompiled:
            pszMsg = "EPdfError::NotCompiled";
            break;
        case EPdfError::DestinationAlreadyPresent:
            pszMsg = "EPdfError::DestinationAlreadyPresent";
            break;
        case EPdfError::ChangeOnImmutable:
            pszMsg = "EPdfError::ChangeOnImmutable";
            break;
        case EPdfError::OutlineItemAlreadyPresent:
            pszMsg = "EPdfError::OutlineItemAlreadyPresent";
            break;
        case EPdfError::NotLoadedForUpdate:
            pszMsg = "EPdfError::NotLoadedForUpdate";
            break;
        case EPdfError::CannotEncryptedForUpdate:
            pszMsg = "EPdfError::CannotEncryptedForUpdate";
            break;
        case EPdfError::Unknown:
            pszMsg = "EPdfError::Unknown";
            break;
        default:
            break;
    }

    return pszMsg;
}

const char* PdfError::ErrorMessage(EPdfError eCode)
{
    const char* pszMsg = nullptr;

    switch (eCode)
    {
        case EPdfError::ErrOk:
            pszMsg = "No error during execution.";
            break;
        case EPdfError::TestFailed:
            pszMsg = "An error curred in an automatic test included in PoDoFo.";
            break;
        case EPdfError::InvalidHandle:
            pszMsg = "A nullptr handle was passed, but initialized data was expected.";
            break;
        case EPdfError::FileNotFound:
            pszMsg = "The specified file was not found.";
            break;
        case EPdfError::InvalidDeviceOperation:
            pszMsg = "Tried to do something unsupported to an I/O device like seek a non-seekable input device";
            break;
        case EPdfError::UnexpectedEOF:
            pszMsg = "End of file was reached unxexpectedly.";
            break;
        case EPdfError::OutOfMemory:
            pszMsg = "PoDoFo is out of memory.";
            break;
        case EPdfError::ValueOutOfRange:
            pszMsg = "The passed value is out of range.";
            break;
        case EPdfError::InternalLogic:
            pszMsg = "An internal error occurred.";
            break;
        case EPdfError::InvalidEnumValue:
            pszMsg = "An invalid enum value was specified.";
            break;
        case EPdfError::BrokenFile:
            pszMsg = "The file content is broken.";
            break;
        case EPdfError::PageNotFound:
            pszMsg = "The requested page could not be found in the PDF.";
            break;
        case EPdfError::NoPdfFile:
            pszMsg = "This is not a PDF file.";
            break;
        case EPdfError::NoXRef:
            pszMsg = "No XRef table was found in the PDF file.";
            break;
        case EPdfError::NoTrailer:
            pszMsg = "No trailer was found in the PDF file.";
            break;
        case EPdfError::NoNumber:
            pszMsg = "A number was expected but not found.";
            break;
        case EPdfError::NoObject:
            pszMsg = "A object was expected but not found.";
            break;
        case EPdfError::NoEOFToken:
            pszMsg = "No EOF Marker was found in the PDF file.";
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
            pszMsg = "The encryption dictionary is invalid or misses a required key.";
            break;
        case EPdfError::InvalidPassword:
            pszMsg = "The password used to open the PDF file was invalid.";
            break;
        case EPdfError::InvalidFontFile:
            pszMsg = "The font file is invalid.";
            break;
        case EPdfError::InvalidContentStream:
            pszMsg = "The content stream is invalid due to mismatched context pairing or other problems.";
            break;
        case EPdfError::UnsupportedFilter:
            break;
        case EPdfError::UnsupportedFontFormat:
            pszMsg = "This font format is not supported by PoDoFO.";
            break;
        case EPdfError::DestinationAlreadyPresent:
        case EPdfError::ActionAlreadyPresent:
            pszMsg = "Outlines can have either destinations or actions.";
            break;
        case EPdfError::WrongDestinationType:
            pszMsg = "The requested field is not available for the given destination type";
            break;
        case EPdfError::MissingEndStream:
        case EPdfError::Date:
            break;
        case EPdfError::Flate:
            pszMsg = "ZLib returned an error.";
            break;
        case EPdfError::FreeType:
            pszMsg = "FreeType returned an error.";
            break;
        case EPdfError::SignatureError:
            pszMsg = "The signature contains an error.";
            break;
        case EPdfError::UnsupportedImageFormat:
            pszMsg = "This image format is not supported by PoDoFO.";
            break;
        case EPdfError::CannotConvertColor:
            pszMsg = "This color format cannot be converted.";
            break;
        case EPdfError::ChangeOnImmutable:
            pszMsg = "Changing values on immutable objects is not allowed.";
            break;
        case EPdfError::NotImplemented:
            pszMsg = "This feature is currently not implemented.";
            break;
        case EPdfError::NotCompiled:
            pszMsg = "This feature was disabled during compile time.";
            break;
        case EPdfError::OutlineItemAlreadyPresent:
            pszMsg = "Given OutlineItem already present in destination tree.";
            break;
        case EPdfError::NotLoadedForUpdate:
            pszMsg = "The document had not been loaded for update.";
            break;
        case EPdfError::CannotEncryptedForUpdate:
            pszMsg = "Cannot load encrypted documents for update.";
            break;
        case EPdfError::Unknown:
            pszMsg = "Error code unknown.";
            break;
        default:
            break;
    }

    return pszMsg;
}

void PdfError::LogMessage(LogSeverity eLogSeverity, const char* pszMsg, ...)
{
    if (!PdfError::LoggingEnabled())
        return;

#ifdef DEBUG
    const LogSeverity eMinSeverity = LogSeverity::Debug;
#else
    const ELogSeverity eMinSeverity = ELogSeverity::Information;
#endif // DEBUG

    if (eLogSeverity > eMinSeverity)
        return;

    va_list  args;
    va_start(args, pszMsg);

    LogMessageInternal(eLogSeverity, pszMsg, args);
    va_end(args);
}

void PdfError::LogErrorMessage(LogSeverity eLogSeverity, const char* pszMsg, ...)
{
    va_list  args;
    va_start(args, pszMsg);

    LogMessageInternal(eLogSeverity, pszMsg, args);
    va_end(args);
}

void PdfError::LogMessageInternal(LogSeverity eLogSeverity, const char* pszMsg, va_list& args)
{
    const char* pszPrefix = nullptr;

    switch (eLogSeverity)
    {
        case LogSeverity::Error:
            break;
        case LogSeverity::Critical:
            pszPrefix = "CRITICAL: ";
            break;
        case LogSeverity::Warning:
            pszPrefix = "WARNING: ";
            break;
        case LogSeverity::Information:
            break;
        case LogSeverity::Debug:
            pszPrefix = "DEBUG: ";
            break;
        case LogSeverity::None:
        case LogSeverity::Unknown:
        default:
            break;
    }

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    if (m_fLogMessageCallback != nullptr)
    {
        m_fLogMessageCallback->LogMessage(eLogSeverity, pszPrefix, pszMsg, args);
        return;
    }

    if (pszPrefix)
        fprintf(stderr, "%s", pszPrefix);

    vfprintf(stderr, pszMsg, args);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void PdfError::LogMessage(LogSeverity eLogSeverity, const wchar_t* pszMsg, ...)
{
    if (!PdfError::LoggingEnabled())
        return;

#ifdef DEBUG
    const LogSeverity eMinSeverity = LogSeverity::Debug;
#else
    const ELogSeverity eMinSeverity = ELogSeverity::Information;
#endif // DEBUG

    // OC 17.08.2010 BugFix: Higher level is lower value
 // if( eLogSeverity < eMinSeverity )
    if (eLogSeverity > eMinSeverity)
        return;

    va_list  args;
    va_start(args, pszMsg);

    LogMessageInternal(eLogSeverity, pszMsg, args);
    va_end(args);
}

void PdfError::EnableLogging(bool bEnable)
{
    PdfError::s_LogEnabled = bEnable;
}

bool PdfError::LoggingEnabled()
{
    return PdfError::s_LogEnabled;
}

void PdfError::LogErrorMessage(LogSeverity eLogSeverity, const wchar_t* pszMsg, ...)
{
    va_list  args;
    va_start(args, pszMsg);

    LogMessageInternal(eLogSeverity, pszMsg, args);
    va_end(args);
}

void PdfError::LogMessageInternal(LogSeverity eLogSeverity, const wchar_t* pszMsg, va_list& args)
{
    const wchar_t* pszPrefix = nullptr;

    switch (eLogSeverity)
    {
        case LogSeverity::Error:
            break;
        case LogSeverity::Critical:
            pszPrefix = L"CRITICAL: ";
            break;
        case LogSeverity::Warning:
            pszPrefix = L"WARNING: ";
            break;
        case LogSeverity::Information:
            break;
        case LogSeverity::Debug:
            pszPrefix = L"DEBUG: ";
            break;
        case LogSeverity::None:
        case LogSeverity::Unknown:
        default:
            break;
    }

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    if (m_fLogMessageCallback != nullptr)
    {
        m_fLogMessageCallback->LogMessage(eLogSeverity, pszPrefix, pszMsg, args);
        return;
    }

    if (pszPrefix)
        fwprintf(stderr, pszPrefix);

    vfwprintf(stderr, pszMsg, args);
}

void PdfError::DebugMessage(const char* pszMsg, ...)
{
    if (!PdfError::DebugEnabled())
        return;

    const char* pszPrefix = "DEBUG: ";

    va_list  args;
    va_start(args, pszMsg);

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    if (m_fLogMessageCallback != nullptr)
    {
        m_fLogMessageCallback->LogMessage(LogSeverity::Debug, pszPrefix, pszMsg, args);
    }
    else
    {
        if (pszPrefix)
            fprintf(stderr, "%s", pszPrefix);

        vfprintf(stderr, pszMsg, args);
    }

    // must call va_end after calling va_start (which allocates memory on some platforms)
    va_end(args);
}

void PdfError::EnableDebug(bool bEnable)
{
    PdfError::s_DgbEnabled = bEnable;
}

bool PdfError::DebugEnabled()
{
    return PdfError::s_DgbEnabled;
}

void PdfError::SetError(const EPdfError& eCode, const char* pszFile, int line, const char* pszInformation)
{
    m_error = eCode;
    this->AddToCallstack(pszFile, line, pszInformation);
}

void PdfError::AddToCallstack(const char* pszFile, int line, const char* pszInformation)
{
    m_callStack.push_front(PdfErrorInfo(line, pszFile, pszInformation));
}

void PdfError::SetError(const EPdfError& eCode, const char* pszFile, int line, string sInformation)
{
    m_error = eCode;
    this->AddToCallstack(pszFile, line, sInformation);
}

void PdfError::AddToCallstack(const char* pszFile, int line, string sInformation)
{
    m_callStack.push_front(PdfErrorInfo(line, pszFile, sInformation));
}

void PdfError::SetErrorInformation(const char* pszInformation)
{
    if (m_callStack.size())
        m_callStack.front().SetInformation(pszInformation ? pszInformation : "");
}

void PdfError::SetErrorInformation(const wchar_t* pszInformation)
{
    if (m_callStack.size())
        m_callStack.front().SetInformation(pszInformation ? pszInformation : L"");
}

bool PdfError::IsError() const
{
    return m_error != EPdfError::ErrOk;
}
