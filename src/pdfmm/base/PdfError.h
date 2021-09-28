/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ERROR_H
#define PDF_ERROR_H

// NOTE: PdfError.h should not include PdfDefines.h, since it is included by it.
// It should avoid depending on anything defined in PdfDefines.h .
#include "pdfmmdefs.h"

#include <queue>
#include <cstdarg>

/** \file PdfError.h
 *  Error information and logging is implemented in this file.
 */

namespace mm {

/** Error Code enum values which are used in PdfError to describe the error.
 *
 *  If you add an error code to this enum, please also add it
 *  to PdfError::ErrorName() and PdfError::ErrorMessage().
 *
 *  \see PdfError
 */
enum class PdfErrorCode
{
    Unknown = 0,              ///< Unknown error
    Ok,                       ///< The default value indicating no error.

    TestFailed,               ///< Used in pdfmm tests, to indicate that a test failed for some reason.

    InvalidHandle,            ///< Null pointer was passed, but null pointer is not allowed.
    FileNotFound,             ///< A file was not found or cannot be opened.
    InvalidDeviceOperation,	  ///< Tried to do something unsupported to an I/O device like seek a non-seekable input device
    UnexpectedEOF,            ///< End of file was reached but data was expected.
    OutOfMemory,              ///< Not enough memory to complete an operation.
    ValueOutOfRange,          ///< The specified memory is out of the allowed range.
    InternalLogic,            ///< An internal sanity check or assertion failed.
    InvalidEnumValue,         ///< An invalid enum value was specified.
    BrokenFile,               ///< The file content is broken.

    PageNotFound,             ///< The requested page could not be found in the PDF.

    NoPdfFile,                ///< The file is no PDF file.
    NoXRef,                   ///< The PDF file has no or an invalid XRef table.
    NoTrailer,                ///< The PDF file has no or an invalid trailer.
    NoNumber,                 ///< A number was expected in the PDF file, but the read string is no number.
    NoObject,                 ///< A object was expected and none was found.
    NoEOFToken,               ///< The PDF file has no or an invalid EOF marker.

    InvalidTrailerSize,       ///< The trailer size is invalid.
    InvalidLinearization,     ///< The linearization directory of a web-optimized PDF file is invalid.
    InvalidDataType,          ///< The passed datatype is invalid or was not recognized
    InvalidXRef,              ///< The XRef table is invalid
    InvalidXRefStream,        ///< A XRef steam is invalid
    InvalidXRefType,          ///< The XRef type is invalid or was not found
    InvalidPredictor,         ///< Invalid or unimplemented predictor
    InvalidStrokeStyle,       ///< Invalid stroke style during drawing
    InvalidHexString,         ///< Invalid hex string
    InvalidStream,            ///< The stream is invalid
    InvalidStreamLength,      ///< The stream length is invalid
    InvalidKey,               ///< The specified key is invalid
    InvalidName,              ///< The specified Name is not valid in this context
    InvalidEncryptionDict,    ///< The encryption dictionary is invalid or misses a required key
    InvalidPassword,          ///< The password used to open the PDF file was invalid
    InvalidFontFile,          ///< The font file is invalid
    InvalidContentStream,     ///< The content stream is invalid due to mismatched context pairing or other problems

    UnsupportedFilter,        ///< The requested filter is not yet implemented.
    UnsupportedFontFormat,    ///< This font format is not supported by pdfmm.
    ActionAlreadyPresent,     ///< An Action was already present when trying to add a Destination
    WrongDestinationType,     ///< The requested field is not available for the given destination type

    MissingEndStream,         ///< The required token endstream was not found.
    Date,                     ///< Date/time error
    Flate,                    ///< Error in zlib
    FreeType,                 ///< Error in FreeType
    SignatureError,           ///< Error in signature

    UnsupportedImageFormat,   ///< This image format is not supported by pdfmm.
    CannotConvertColor,       ///< This color format cannot be converted.

    NotImplemented,           ///< This feature is currently not implemented.

    DestinationAlreadyPresent,///< A destination was already present when trying to add an Action
    ChangeOnImmutable,        ///< Changing values on immutable objects is not allowed.

    NotCompiled,              ///< This feature was disabled at compile time.

    OutlineItemAlreadyPresent,///< An outline item to be inserted was already in that outlines tree.
    NotLoadedForUpdate,       ///< The document had not been loaded for update.
    CannotEncryptedForUpdate, ///< Cannot load encrypted documents for update.
};

/**
 * Used in PdfError::LogMessage to specify the log level.
 *
 * \see PdfError::LogMessage
 */
enum class LogSeverity
{
    Unknown = 0,         ///< Unknown log level
    Critical,            ///< Critical unexpected error
    Error,               ///< Error
    Warning,             ///< Warning
    Information,         ///< Information message
    Debug,               ///< Debug information
    None,                ///< No specified level
};

/** \def PDFMM_RAISE_ERROR(code)
 *
 *  Throw an exception of type PdfError with the error code x, which should be
 *  one of the values of the enum PdfErrorCode. File and line info are included.
 */
#define PDFMM_RAISE_ERROR(code) throw ::mm::PdfError(code, __FILE__, __LINE__);

/** \def PDFMM_RAISE_ERROR_INFO(code, msg)
 *
 *  Throw an exception of type PdfError with the error code, which should be
 *  one of the values of the enum PdfErrorCode. File and line info are included.
 *  Additionally extra information on the error, msg is set, which will also be
 *  output by PdfError::PrintErrorMsg().
 *  msg can be a C string, but can also be a C++ std::string.
 */
#define PDFMM_RAISE_ERROR_INFO(code, msg) throw ::mm::PdfError(code, __FILE__, __LINE__, msg);

/** \def PDFMM_RAISE_LOGIC_IF(cond, msg)
 *
 *  Evaluate `cond' as a binary predicate and if it is true, raise a logic error with the
 *  info string `msg' .
 */
#define PDFMM_RAISE_LOGIC_IF(cond, msg) {\
    if (cond)\
        throw ::mm::PdfError(PdfErrorCode::InternalLogic, __FILE__, __LINE__, msg);\
};

class PDFMM_API PdfErrorInfo
{
public:
    PdfErrorInfo(unsigned line, std::string file, std::string info);
    PdfErrorInfo(const PdfErrorInfo& rhs) = default;

    PdfErrorInfo& operator=(const PdfErrorInfo& rhs) = default;

    inline unsigned GetLine() const { return m_Line; }
    inline const std::string& GetFilename() const { return m_File; }
    inline const std::string& GetInformation() const { return m_Info; }

private:
    unsigned m_Line;
    std::string m_File;
    std::string m_Info;
};

typedef std::deque<PdfErrorInfo> PdErrorInfoQueue;

// This is required to generate the documentation with Doxygen.
// Without this define doxygen thinks we have a class called PDFMM_EXCEPTION_API(PDFMM_API) ...
#define PDFMM_EXCEPTION_API_DOXYGEN PDFMM_EXCEPTION_API(PDFMM_API)

/** The error handling class of the pdfmm library.
 *  If a method encounters an error,
 *  a PdfError object is thrown as a C++ exception.
 *
 *  This class does not inherit from std::exception.
 *
 *  This class also provides meaningful error descriptions
 *  for the error codes which are values of the enum PdfErrorCode,
 *  which are all codes pdfmm uses (except the first and last one).
 */
class PDFMM_EXCEPTION_API_DOXYGEN PdfError final
{
public:

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    class LogMessageCallback
    {
    public:
        virtual ~LogMessageCallback() {}
        virtual void LogMessage(LogSeverity logSeverity, const char* prefix, const char* msg, va_list& args) = 0;
    };

    /** Set a global static LogMessageCallback functor to replace stderr output in LogMessageInternal.
     *  \param logMessageCallback the pointer to the new callback functor object
     *  \returns the pointer to the previous callback functor object
     */
    static LogMessageCallback* SetLogMessageCallback(LogMessageCallback* logMessageCallback);

public:
    /** Create a PdfError object initialized to PdfErrorCode::Ok.
     */
    PdfError();

    /** Create a PdfError object with a given error code.
     *  \param code the error code of this object
     *  \param file the file in which the error has occurred.
     *         Use the compiler macro __FILE__ to initialize the field.
     *  \param line the line in which the error has occurred.
     *         Use the compiler macro __LINE__ to initialize the field.
     *  \param information additional information on this error
     */
    PdfError(PdfErrorCode code, std::string file, unsigned line,
        std::string information = { });

    /** Copy constructor
     *  \param rhs copy the contents of rhs into this object
     */
    PdfError(const PdfError& rhs) = default;

    /** Assignment operator
     *  \param rhs another PdfError object
     *  \returns this object
     */
    PdfError& operator=(const PdfError& rhs) = default;

    /** Overloaded assignment operator
     *  \param code a PdfErrorCode code
     *  \returns this object
     */
    PdfError& operator=(const PdfErrorCode& code);

    /** Compares this PdfError object
     *  with an error code
     *  \param code an error code (value of the enum PdfErrorCode)
     *  \returns true if this object has the same error code.
     */
    bool operator==(PdfErrorCode code);

    /** Compares this PdfError object
     *  with an error code
     *  \param code an error code (value of the enum PdfErrorCode)
     *  \returns true if this object has a different error code.
     */
    bool operator!=(PdfErrorCode code);

    /** Return the error code of this object.
     *  \returns the error code of this object
     */
    inline PdfErrorCode GetError() const { return m_error; }

    /** Get access to the internal callstack of this error.
     *  \returns the callstack deque of PdfErrorInfo objects.
     */
    inline const PdErrorInfoQueue& GetCallstack() const { return m_callStack; }

    /** Add callstack information to an error object. Always call this function
     *  if you get an error object but do not handle the error but throw it again.
     *
     *  \param file the filename of the source file causing
     *                 the error or nullptr. Typically you will use
     *                 the gcc macro __FILE__ here.
     *  \param line    the line of source causing the error
     *                 or 0. Typically you will use the gcc
     *                 macro __LINE__ here.
     *  \param information additional information on the error,
     *         e.g. how to fix the error. This string is intended to
     *         be shown to the user.
     */
    void AddToCallstack(std::string file, unsigned line, std::string information = { });

    /** \returns true if an error code was set
     *           and false if the error code is PdfErrorCode::Ok.
     */
    bool IsError() const;

    /** Print an error message to stderr. This includes callstack
     *  and extra info, if any of either was set.
     */
    void PrintErrorMsg() const;

    /** Obtain error description.
     *  \returns a C string describing the error.
     */
    const char* what() const;

    /** Get the name for a certain error code.
     *  \returns the name or nullptr if no name for the specified
     *           error code is available.
     */
    static const char* ErrorName(PdfErrorCode code);

    /** Get the error message for a certain error code.
     *  \returns the error message or nullptr if no error
     *           message for the specified error code
     *           is available.
     */
    static const char* ErrorMessage(PdfErrorCode code);

    /** Log a message to the logging system defined for pdfmm.
     *  \param logSeverity the severity of the log message
     *  \param msg       the message to be logged
     */
    static void LogMessage(LogSeverity logSeverity, const char* msg, ...);

    /** Enable or disable logging.
    *  \param enable       enable (true) or disable (false)
    */
    static void EnableLogging(bool enable);

    /** Is the display of debugging messages enabled or not?
     */
    static bool LoggingEnabled();

    /** Log a message to the logging system defined for pdfmm for debugging.
     *  \param msg       the message to be logged
     */
    static void DebugMessage(const char* msg, ...);

    /** Enable or disable the display of debugging messages.
     *  \param enable       enable (true) or disable (false)
     */
    static void EnableDebug(bool enable);

    /** Is the display of debugging messages enabled or not?
     */
    static bool DebugEnabled();

private:
    /** Log a message to the logging system defined for pdfmm.
     *
     *  This call does not check if logging is enabled and always
     *  prints the error message.
     *
     *  \param logSeverity the severity of the log message
     *  \param msg       the message to be logged
     */
    static void LogErrorMessage(LogSeverity logSeverity, const char* msg, ...);

    static void LogMessageInternal(LogSeverity logSeverity, const char* msg, va_list& args);

private:
    static bool s_DgbEnabled;
    static bool s_LogEnabled;

    // OC 17.08.2010 New to optionally replace stderr output by a callback:
    static LogMessageCallback* m_LogMessageCallback;
private:
    PdfErrorCode m_error;
    PdErrorInfoQueue m_callStack;
};

};

#endif // PDF_ERROR_H
