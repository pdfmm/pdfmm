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

#include <iostream>
#include <sstream>

using namespace std;
using namespace mm;

#ifdef DEBUG
LogSeverity s_MinLogSeverity = LogSeverity::Debug;
#else
LogSeverity s_MinLogSeverity = LogSeverity::Information;
#endif // DEBUG

static LogMessageCallback s_LogMessageCallback;

void PdfError::SetLogMessageCallback(const LogMessageCallback& logMessageCallback)
{
    s_LogMessageCallback = logMessageCallback;
}

PdfErrorInfo::PdfErrorInfo(unsigned line, string file, string info)
    : m_Line(line), m_File(std::move(file)), m_Info(std::move(info)) { }

PdfError::PdfError()
{
    m_error = PdfErrorCode::Ok;
}

PdfError::PdfError(PdfErrorCode code, string file, unsigned line,
    string information)
{
    m_error = code;
    this->AddToCallstack(std::move(file), line, std::move(information));
}

PdfError& PdfError::operator=(const PdfErrorCode& code)
{
    m_error = code;
    m_callStack.clear();
    return *this;
}

bool PdfError::operator==(PdfErrorCode code)
{
    return m_error == code;
}

bool PdfError::operator!=(PdfErrorCode code)
{
    return m_error != code;
}

void PdfError::PrintErrorMsg() const
{
    const char* msg = PdfError::ErrorMessage(m_error);
    const char* name = PdfError::ErrorName(m_error);

    ostringstream stream;
    stream << endl << endl << "pdfmm encountered an error. Error: " << (int)m_error << (name == nullptr ? "" : name);

    if (msg != nullptr)
        stream << "\tError Description: " << msg;

    if (m_callStack.size() != 0)
        stream << "\tCallstack:";

    unsigned i = 0;
    for (auto& info: m_callStack)
    {
        if (!info.GetFilename().empty())
            stream << "\t#" << i << " Error Source : " << info.GetFilename() << ": " << info.GetLine();

        if (!info.GetInformation().empty())
            stream << "\t\t" << "Information: " << info.GetInformation();

        stream << endl;
        i++;
    }

    PdfError::LogMessage(LogSeverity::Error, stream.str());
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

void PdfError::LogMessage(LogSeverity logSeverity, const string_view& msg)
{
    if (logSeverity > s_MinLogSeverity)
        return;

    if (s_LogMessageCallback == nullptr)
    {
        string_view prefix;
        bool ouputstderr = false;
        switch (logSeverity)
        {
            case LogSeverity::Error:
                prefix = "ERROR: ";
                ouputstderr = true;
                break;
            case LogSeverity::Warning:
                prefix = "WARNING: ";
                ouputstderr = true;
                break;
            case LogSeverity::Debug:
                prefix = "DEBUG: ";
                break;
            case LogSeverity::Information:
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

void PdfError::SetMinLoggingSeverity(LogSeverity logSeverity)
{
    s_MinLogSeverity = logSeverity;
}

bool PdfError::IsLoggingSeverityEnabled(LogSeverity logSeverity)
{
    if (logSeverity == LogSeverity::None)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEnumValue, "Unexpected LogSeverity::None");

    return logSeverity >= s_MinLogSeverity;
}

void PdfError::AddToCallstack(string file, unsigned line, string information)
{
    m_callStack.push_front(PdfErrorInfo(std::move(line), std::move(file), information));
}

bool PdfError::IsError() const
{
    return m_error != PdfErrorCode::Ok;
}