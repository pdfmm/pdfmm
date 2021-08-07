/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfParser.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfMemStream.h"
#include "PdfObjectStreamParser.h"
#include "PdfOutputDevice.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfXRefStreamParserObject.h"

#include <sstream>
#include <algorithm>
#include <iostream>

#define PDF_VERSION_LENGHT  3
#define PDF_MAGIC_LENGHT    8
#define PDF_XREF_ENTRY_SIZE 20
#define PDF_XREF_BUF        512

using namespace std;
using namespace mm;

static bool CheckEOL(char e1, char e2);
static bool CheckXRefEntryType(char c);
static bool ReadMagicWord(char ch, int& charidx);

constexpr int64_t MaxNumIndirectObjects = (1 << 23) - 1;

class PdfRecursionGuard
{
  // RAII recursion guard ensures m_nRecursionDepth is always decremented
  // because the destructor is always called when control leaves a method
  // via return or an exception.
  // see http://en.cppreference.com/w/cpp/language/raii

  // It's used like this in PdfParser methods
  // PdfRecursionGuard guard(m_nRecursionDepth);

public:
    PdfRecursionGuard(unsigned& nRecursionDepth)
        : m_RecursionDepth(nRecursionDepth)
    {
        // be careful changing this limit - overflow limits depend on the OS, linker settings, and how much stack space compiler allocates
        // 500 limit prevents overflow on Win7 with VC++ 2005 with default linker stack size (1000 caused overflow with same compiler/OS)
        constexpr unsigned maxRecursionDepth = 500;

        m_RecursionDepth++;

        if (m_RecursionDepth > maxRecursionDepth)
        {
            // avoid stack overflow on documents that have circular cross references in /Prev entries
            // in trailer and XRef streams (possible via a chain of entries with a loop)
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidXRef);
        }
    }

    ~PdfRecursionGuard()
    {
        m_RecursionDepth--;
    }

private:
    // must be a reference so that we modify m_nRecursionDepth in parent class
    unsigned& m_RecursionDepth;
};

PdfParser::PdfParser(PdfVecObjects& objects) :
    m_buffer(PdfTokenizer::BufferSize),
    m_tokenizer(m_buffer, true),
    m_Objects(&objects),
    m_StrictParsing(false)
{
    this->Reset();
}

PdfParser::~PdfParser()
{
    Reset();
}

void PdfParser::Reset()
{
    m_PdfVersion = PdfVersionDefault;
    m_LoadOnDemand = false;

    m_magicOffset = 0;
    m_HasXRefStream = false;
    m_XRefOffset = 0;
    m_objectCount = 0;
    m_XRefLinearizedOffset = 0;
    m_LastEOFOffset = 0;

    m_Trailer = nullptr;
    m_Linearization = nullptr;
    m_entries.clear();
    m_ObjectStreams.clear();

    m_Encrypt = nullptr;

    m_IgnoreBrokenObjects = true;
    m_IncrementalUpdateCount = 0;
    m_RecursionDepth = 0;
}

void PdfParser::ParseFile(const string_view& filename, bool loadOnDemand)
{
    if (filename.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfRefCountedInputDevice device(filename);
    if (device.Device() == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::FileNotFound, filename.data());

    this->Parse(device, loadOnDemand);
}

void PdfParser::ParseBuffer(const string_view& buffer, bool loadOnDemand)
{
    if (buffer.length() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    PdfRefCountedInputDevice device(buffer.data(), buffer.length());
    if (device.Device() == nullptr)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidHandle, "Cannot create PdfParser from buffer.");

    this->Parse(device, loadOnDemand);
}

void PdfParser::Parse(const PdfRefCountedInputDevice& device, bool loadOnDemand)
{
    Reset();

    m_LoadOnDemand = loadOnDemand;

    try
    {
        if (!IsPdfFile(device))
            PDFMM_RAISE_ERROR(PdfErrorCode::NoPdfFile);

        ReadDocumentStructure(device);
        ReadObjects(device);
    }
    catch (PdfError& e)
    {
        if (e.GetError() == PdfErrorCode::InvalidPassword)
        {
            // Do not clean up, expect user to call ParseFile again
            throw e;
        }

        // If this is being called from a constructor then the
        // destructor will not be called.
        // Clean up here  
        Reset();
        e.AddToCallstack(__FILE__, __LINE__, "Unable to load objects from file.");
        throw e;
    }
}

void PdfParser::ReadDocumentStructure(const PdfRefCountedInputDevice& device)
{
    // Ulrich Arnold 8.9.2009, deactivated because of problems during reading xref's
    // HasLinearizationDict();

    // position at the end of the file to search the xref table.
    device.Device()->Seek(0, ios_base::end);
    m_FileSize = device.Device()->Tell();

    // James McGill 18.02.2011, validate the eof marker and when not in strict mode accept garbage after it
    try
    {
        CheckEOFMarker(device);
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "EOF marker could not be found.");
        throw e;
    }

    try
    {
        ReadXRef(device, &m_XRefOffset);
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "Unable to find startxref entry in file.");
        throw e;
    }

    try
    {
        ReadTrailer(device);
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "Unable to find trailer in file.");
        throw e;
    }

    if (m_Linearization)
    {
        try
        {
            ReadXRefContents(device, m_XRefOffset, true);
        }
        catch (PdfError& e)
        {
            e.AddToCallstack(__FILE__, __LINE__, "Unable to skip xref dictionary.");
            throw e;
        }

        // another trailer directory is to follow right after this XRef section
        try
        {
            ReadNextTrailer(device);
        }
        catch (PdfError& e)
        {
            if (e != PdfErrorCode::NoTrailer)
                throw e;
        }
    }

    if (m_Trailer->IsDictionary() && m_Trailer->GetDictionary().HasKey(PdfName::KeySize))
    {
        m_objectCount = static_cast<unsigned>(m_Trailer->GetDictionary().FindKeyAs<int64_t>(PdfName::KeySize));
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Warning, "PDF Standard Violation: No /Size key was specified in the trailer directory. Will attempt to recover.");
        // Treat the xref size as unknown, and expand the xref dynamically as we read it.
        m_objectCount = 0;
    }

    if (m_objectCount > 0)
    {
        ResizeEntries(m_objectCount);
    }

    if (m_Linearization)
    {
        try
        {
            ReadXRefContents(device, m_XRefLinearizedOffset);
        }
        catch (PdfError& e)
        {
            e.AddToCallstack(__FILE__, __LINE__, "Unable to read linearized XRef section.");
            throw e;
        }
    }

    try
    {
        ReadXRefContents(device, m_XRefOffset);
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "Unable to load xref entries.");
        throw e;
    }
}

bool PdfParser::IsPdfFile(const PdfRefCountedInputDevice& device)
{
    int i = 0;
    device.Device()->Seek(0, ios_base::beg);
    while (true)
    {
        char ch;
        if (!device.Device()->TryGetChar(ch))
            return false;

        if (ReadMagicWord(ch, i))
            break;
    }

    char version[PDF_VERSION_LENGHT];
    if (device.Device()->Read(version, PDF_VERSION_LENGHT) != PDF_VERSION_LENGHT)
        return false;

    m_magicOffset = device.Device()->Tell() - PDF_MAGIC_LENGHT;
    // try to determine the excact PDF version of the file
    for (i = 0; i <= MAX_PDF_VERSION_STRING_INDEX; i++)
    {
        if (strncmp(version, s_PdfVersionNums[i], PDF_VERSION_LENGHT) == 0)
        {
            m_PdfVersion = static_cast<PdfVersion>(i);
            return true;
        }
    }

    return false;
}

void PdfParser::HasLinearizationDict(const PdfRefCountedInputDevice& device)
{
    if (m_Linearization)
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic,
            "HasLinarizationDict() called twice on one object");
    }

    device.Device()->Seek(0);

    // The linearization dictionary must be in the first 1024 
    // bytes of the PDF, our buffer might be larger so.
    // Therefore read only the first 1024 byte.
    // Normally we should jump to the end of the file, to determine
    // it's filesize and read the min(1024, filesize) to not fail
    // on smaller files, but jumping to the end is against the idea
    // of linearized PDF. Therefore just check if we read anything.
    constexpr streamoff MAX_READ = 1024;
    PdfRefCountedBuffer linearizeBuffer(MAX_READ);

    streamoff size = device.Device()->Read(linearizeBuffer.GetBuffer(),
        linearizeBuffer.GetSize());
    // Only fail if we read nothing, to allow files smaller than MAX_READ
    if (static_cast<size_t>(size) <= 0)
    {
        // Ignore Error Code: ERROR_PDF_NO_TRAILER;
        return;
    }

    char* objStr = strstr(linearizeBuffer.GetBuffer(), "obj");
    if (objStr == nullptr)
    {
        // strange that there is no obj in the first 1024 bytes,
        // but ignore it
        return;
    }

    objStr--;
    while (*objStr && (PdfTokenizer::IsWhitespace(*objStr) || (*objStr >= '0' && *objStr <= '9')))
        objStr--;

    m_Linearization.reset(new PdfParserObject(m_Objects->GetDocument(), device, linearizeBuffer,
        objStr - linearizeBuffer.GetBuffer() + 2));

    try
    {
        // Do not care for encryption here, as the linearization dictionary does not contain strings or streams
        // ... hint streams do, but we do not load the hintstream.
        m_Linearization->ParseFile(nullptr);
        if (!(m_Linearization->IsDictionary() &&
            m_Linearization->GetDictionary().HasKey("Linearized")))
        {
            m_Linearization = nullptr;
            return;
        }
    }
    catch (PdfError& e)
    {
        PdfError::LogMessage(LogSeverity::Warning, e.ErrorName(e.GetError()));
        m_Linearization = nullptr;
        return;
    }

    int64_t xRef = -1;
    xRef = m_Linearization->GetDictionary().FindKeyAs<int64_t>("T", xRef);
    if (xRef == -1)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidLinearization);
    }

    // avoid moving to a negative file position here
    device.Device()->Seek(static_cast<size_t>(xRef - PDF_XREF_BUF) > 0 ? static_cast<size_t>(xRef - PDF_XREF_BUF) : PDF_XREF_BUF);
    m_XRefLinearizedOffset = device.Device()->Tell();

    if (device.Device()->Read(m_buffer.GetBuffer(), PDF_XREF_BUF) != PDF_XREF_BUF)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidLinearization);

    m_buffer.GetBuffer()[PDF_XREF_BUF] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    const int XREF_LEN = 4; // strlen( "xref" );
    int i = 0;
    char* startStr = nullptr;
    for (i = PDF_XREF_BUF - XREF_LEN; i >= 0; i--)
    {
        if (strncmp(m_buffer.GetBuffer() + i, "xref", XREF_LEN) == 0)
        {
            startStr = m_buffer.GetBuffer() + i;
            break;
        }
    }

    m_XRefLinearizedOffset += i;

    if (startStr == nullptr)
    {
        if (m_PdfVersion < PdfVersion::V1_5)
        {
            PdfError::LogMessage(LogSeverity::Warning,
                "Linearization dictionaries are only supported with PDF version 1.5. This is 1.%i. Trying to continue.\n",
                static_cast<int>(m_PdfVersion));
            // PDFMM_RAISE_ERROR( EPdfError::InvalidLinearization );
        }

        {
            m_XRefLinearizedOffset = static_cast<size_t>(xRef);
            //code = ReadXRefStreamContents();
            //i     = 0;
        }
    }
}

void PdfParser::MergeTrailer(const PdfObject& trailer)
{
    PdfVariant var;

    if (m_Trailer == nullptr)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);
    }

    // Only update keys, if not already present                   
    if (trailer.GetDictionary().HasKey(PdfName::KeySize)
        && !m_Trailer->GetDictionary().HasKey(PdfName::KeySize))
        m_Trailer->GetDictionary().AddKey(PdfName::KeySize, *(trailer.GetDictionary().GetKey(PdfName::KeySize)));

    if (trailer.GetDictionary().HasKey("Root")
        && !m_Trailer->GetDictionary().HasKey("Root"))
        m_Trailer->GetDictionary().AddKey("Root", *(trailer.GetDictionary().GetKey("Root")));

    if (trailer.GetDictionary().HasKey("Encrypt")
        && !m_Trailer->GetDictionary().HasKey("Encrypt"))
        m_Trailer->GetDictionary().AddKey("Encrypt", *(trailer.GetDictionary().GetKey("Encrypt")));

    if (trailer.GetDictionary().HasKey("Info")
        && !m_Trailer->GetDictionary().HasKey("Info"))
        m_Trailer->GetDictionary().AddKey("Info", *(trailer.GetDictionary().GetKey("Info")));

    if (trailer.GetDictionary().HasKey("ID")
        && !m_Trailer->GetDictionary().HasKey("ID"))
        m_Trailer->GetDictionary().AddKey("ID", *(trailer.GetDictionary().GetKey("ID")));
}

void PdfParser::ReadNextTrailer(const PdfRefCountedInputDevice& device)
{
    PdfRecursionGuard guard(m_RecursionDepth);

    if (m_tokenizer.IsNextToken(*device.Device(), "trailer"))
    {
        PdfParserObject trailer(m_Objects->GetDocument(), device, m_buffer);
        try
        {
            // Ignore the encryption in the trailer as the trailer may not be encrypted
            trailer.ParseFile(nullptr, true);
        }
        catch (PdfError& e)
        {
            e.AddToCallstack(__FILE__, __LINE__, "The linearized trailer was found in the file, but contains errors.");
            throw e;
        }

        // now merge the information of this trailer with the main documents trailer
        MergeTrailer(trailer);

        if (trailer.GetDictionary().HasKey("XRefStm"))
        {
            // Whenever we read a XRefStm key, 
            // we know that the file was updated.
            if (!trailer.GetDictionary().HasKey("Prev"))
                m_IncrementalUpdateCount++;

            try
            {
                ReadXRefStreamContents(device, static_cast<size_t>(trailer.GetDictionary().FindKeyAs<int64_t>("XRefStm", 0)), false);
            }
            catch (PdfError& e)
            {
                e.AddToCallstack(__FILE__, __LINE__, "Unable to load /XRefStm xref stream.");
                throw e;
            }
        }

        if (trailer.GetDictionary().HasKey("Prev"))
        {
            // Whenever we read a Prev key, 
            // we know that the file was updated.
            m_IncrementalUpdateCount++;

            try
            {
                size_t offset = static_cast<size_t>(trailer.GetDictionary().FindKeyAs<int64_t>("Prev", 0));

                if (m_visitedXRefOffsets.find(offset) == m_visitedXRefOffsets.end())
                    ReadXRefContents(device, offset);
                else
                    PdfError::LogMessage(LogSeverity::Warning, "XRef contents at offset %" PDF_FORMAT_INT64 " requested twice, skipping the second read", static_cast<int64_t>(offset));
            }
            catch (PdfError& e)
            {
                e.AddToCallstack(__FILE__, __LINE__, "Unable to load /Prev xref entries.");
                throw e;
            }
        }
    }
    else // OC 13.08.2010 BugFix: else belongs to IsNextToken( "trailer" ) and not to HasKey( "Prev" )
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NoTrailer);
    }
}

void PdfParser::ReadTrailer(const PdfRefCountedInputDevice& device)
{
    FindToken(device, "trailer", PDF_XREF_BUF);

    if (!m_tokenizer.IsNextToken(*device.Device(), "trailer"))
    {
        if (m_PdfVersion < PdfVersion::V1_3)
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::NoTrailer);
        }
        else
        {
            // Since PDF 1.5 trailer information can also be found
            // in the crossreference stream object
            // and a trailer dictionary is not required
            device.Device()->Seek(m_XRefOffset);

            m_Trailer.reset(new PdfParserObject(m_Objects->GetDocument(), device, m_buffer));
            m_Trailer->ParseFile(nullptr, false);
            return;
        }
    }
    else
    {
        m_Trailer.reset(new PdfParserObject(m_Objects->GetDocument(), device, m_buffer));
        try
        {
            // Ignore the encryption in the trailer as the trailer may not be encrypted
            m_Trailer->ParseFile(nullptr, true);
        }
        catch (PdfError& e)
        {
            e.AddToCallstack(__FILE__, __LINE__, "The trailer was found in the file, but contains errors.");
            throw e;
        }
#ifdef PDFMM_VERBOSE_DEBUG
        PdfError::DebugMessage("Size=%" PDF_FORMAT_INT64 "\n", m_Trailer->GetDictionary().GetAs<int64_t>(PdfName::KeySize, 0));
#endif // PDFMM_VERBOSE_DEBUG
    }
}

void PdfParser::ReadXRef(const PdfRefCountedInputDevice& device, size_t* xRefOffset)
{
    FindToken(device, "startxref", PDF_XREF_BUF);

    if (!m_tokenizer.IsNextToken(*device.Device(), "startxref"))
    {
        // Could be non-standard startref
        if (!m_StrictParsing)
        {
            FindToken(device, "startref", PDF_XREF_BUF);
            if (!m_tokenizer.IsNextToken(*device.Device(), "startref"))
                PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
        else
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
    }

    // Support also files with whitespace offset before magic start
    *xRefOffset = (size_t)m_tokenizer.ReadNextNumber(*device.Device()) + m_magicOffset;
}

void PdfParser::ReadXRefContents(const PdfRefCountedInputDevice& device, size_t offset, bool positionAtEnd)
{
    PdfRecursionGuard guard(m_RecursionDepth);

    int64_t firstObject = 0;
    int64_t objectCount = 0;

    if (m_visitedXRefOffsets.find(offset) != m_visitedXRefOffsets.end())
    {
        ostringstream oss;
        oss << "Cycle in xref structure. Offset  "
            << offset << " already visited.";

        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, oss.str());
    }
    else
    {
        m_visitedXRefOffsets.insert(offset);
    }


    size_t curPosition = device.Device()->Tell();
    device.Device()->Seek(0, ios_base::end);
    size_t fileSize = device.Device()->Tell();
    device.Device()->Seek(curPosition, ios_base::beg);

    if (offset > fileSize)
    {
        // Invalid "startxref" Peter Petrov 23 December 2008
         // ignore returned value and get offset from the device
        ReadXRef(device, &offset);
        offset = device.Device()->Tell();
        // TODO: hard coded value "4"
        m_buffer.Resize(PDF_XREF_BUF * 4);
        FindToken2(device, "xref", PDF_XREF_BUF * 4, offset);
        m_buffer.Resize(PDF_XREF_BUF);
        offset = device.Device()->Tell();
        m_XRefOffset = offset;
    }
    else
    {
        device.Device()->Seek(offset);
    }

    if (!m_tokenizer.IsNextToken(*device.Device(), "xref"))
    {
        //		Ulrich Arnold 19.10.2009, found linearized 1.3-pdf's with trailer-info in xref-stream
        if (m_PdfVersion < PdfVersion::V1_3)
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
        else
        {
            m_HasXRefStream = true;
            ReadXRefStreamContents(device, offset, positionAtEnd);
            return;
        }
    }

    // read all xref subsections
    // OC 13.08.2010: Avoid exception to terminate endless loop
    for (int xrefSectionCount = 0; ; xrefSectionCount++)
    {
        try
        {
            // OC 13.08.2010: Avoid exception to terminate endless loop
            if (xrefSectionCount > 0)
            {
                // something like PeekNextToken()
                PdfTokenType tokenType;
                string_view readToken;
                bool gotToken = m_tokenizer.TryReadNextToken(*device.Device(), readToken, tokenType);
                if (gotToken)
                {
                    m_tokenizer.EnqueueToken(readToken, tokenType);
                    if (readToken == "trailer")
                        break;
                }
            }

            firstObject = m_tokenizer.ReadNextNumber(*device.Device());
            objectCount = m_tokenizer.ReadNextNumber(*device.Device());

#ifdef PDFMM_VERBOSE_DEBUG
            PdfError::DebugMessage("Reading numbers: %" PDF_FORMAT_INT64 " %" PDF_FORMAT_INT64 "\n", firstObject, objectCount);
#endif // PDFMM_VERBOSE_DEBUG

            if (positionAtEnd)
            {
#ifdef WIN32
                device.Device()->Seek(static_cast<streamoff>(objectCount * PDF_XREF_ENTRY_SIZE), ios_base::cur);
#else
                device.Device()->Seek(objectCount * PDF_XREF_ENTRY_SIZE, ios_base::cur);
#endif // WIN32
            }
            else
            {
                ReadXRefSubsection(device, firstObject, objectCount);
            }
        }
        catch (PdfError& e)
        {
            if (e == PdfErrorCode::NoNumber || e == PdfErrorCode::InvalidXRef || e == PdfErrorCode::UnexpectedEOF)
                break;
            else
            {
                e.AddToCallstack(__FILE__, __LINE__);
                throw e;
            }
        }
    }

    try
    {
        ReadNextTrailer(device);
    }
    catch (PdfError& e)
    {
        if (e != PdfErrorCode::NoTrailer)
        {
            e.AddToCallstack(__FILE__, __LINE__);
            throw e;
        }
    }
}

bool CheckEOL(char e1, char e2)
{
    // From pdf reference, page 94:
    // If the file's end-of-line marker is a single character (either a carriage return or a line feed),
    // it is preceded by a single space; if the marker is 2 characters (both a carriage return and a line feed),
    // it is not preceded by a space.            
    return ((e1 == '\r' && e2 == '\n') || (e1 == '\n' && e2 == '\r') || (e1 == ' ' && (e2 == '\r' || e2 == '\n')));
}

bool CheckXRefEntryType(char c)
{
    return c == 'n' || c == 'f';
}

void PdfParser::ReadXRefSubsection(const PdfRefCountedInputDevice& device, int64_t& firstObject, int64_t& objectCount)
{
    int64_t count = 0;

#ifdef PDFMM_VERBOSE_DEBUG
    PdfError::DebugMessage("Reading XRef Section: %" PDF_FORMAT_INT64 " with %" PDF_FORMAT_INT64 " Objects.\n", firstObject, objectCount);
#endif // PDFMM_VERBOSE_DEBUG 

    if (firstObject < 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: nFirstObject is negative");
    if (objectCount < 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: nNumObjects is negative");

    // overflow guard, fixes CVE-2017-5853 (signed integer overflow)
    // also fixes CVE-2017-6844 (buffer overflow) together with below size check
    if (MaxNumIndirectObjects >= objectCount && firstObject <= (MaxNumIndirectObjects - objectCount))
    {
        if (firstObject + objectCount > m_objectCount)
        {
            // Total number of xref entries to read is greater than the /Size
            // specified in the trailer if any. That's an error unless we're
            // trying to recover from a missing /Size entry.
            PdfError::LogMessage(LogSeverity::Warning,
                "There are more objects (%" PDF_FORMAT_INT64 ") in this XRef "
                "table than specified in the size key of the trailer directory "
                "(%" PDF_FORMAT_INT64 ")!\n", firstObject + objectCount,
                static_cast<int64_t>(m_objectCount));
        }

        if (static_cast<uint64_t>(firstObject) + static_cast<uint64_t>(objectCount) > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange,
                "xref subsection's given entry numbers together too large");
        }

        if (firstObject + objectCount > m_objectCount)
        {
            try
            {
                m_objectCount = static_cast<unsigned>(firstObject + objectCount);
                ResizeEntries((size_t)(firstObject + objectCount));

            }
            catch (std::exception&) {
                // If ObjectCount*sizeof(TXRefEntry) > std::numeric_limits<size_t>::max() then
                // resize() throws std::length_error, for smaller allocations that fail it may throw
                // std::bad_alloc (implementation-dependent). "20.5.5.12 Restrictions on exception 
                // handling" in the C++ Standard says any function that throws an exception is allowed 
                // to throw implementation-defined exceptions derived the base type (std::exception)
                // so we need to catch all std::exceptions here
                PDFMM_RAISE_ERROR(PdfErrorCode::OutOfMemory);
            }
        }
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Error, "There are more objects (%" PDF_FORMAT_INT64
            " + %" PDF_FORMAT_INT64 " seemingly) in this XRef"
            " table than supported by standard PDF, or it's inconsistent.",
            firstObject, objectCount);
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidXRef);
    }

    // consume all whitespaces
    int charcode;
    while (m_tokenizer.IsWhitespace((charcode = device.Device()->Look())))
        (void)device.Device()->GetChar();

    while (count < objectCount && device.Device()->Read(m_buffer.GetBuffer(), PDF_XREF_ENTRY_SIZE) == PDF_XREF_ENTRY_SIZE)
    {
        char empty1;
        char empty2;
        m_buffer.GetBuffer()[PDF_XREF_ENTRY_SIZE] = '\0';

        const int objID = static_cast<int>(firstObject + count);

        PdfXRefEntry& entry = m_entries[objID];
        if (static_cast<size_t>(objID) < m_entries.size() && !entry.Parsed)
        {
            uint64_t variant = 0;
            uint32_t generation = 0;
            char cType = 0;

            // XRefEntry is defined in PDF spec section 7.5.4 Cross-Reference Table as
            // nnnnnnnnnn ggggg n eol
            // nnnnnnnnnn is 10-digit offset number with max value 9999999999 (bigger than 2**32 = 4GB)
            // ggggg is a 5-digit generation number with max value 99999 (smaller than 2**17)
            // eol is a 2-character end-of-line sequence
            int read = sscanf(m_buffer.GetBuffer(), "%10" SCNu64 " %5" SCNu32 " %c%c%c",
                &variant, &generation, &cType, &empty1, &empty2);

            if (!CheckXRefEntryType(cType))
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "Invalid used keyword, must be eiter 'n' or 'f'");

            XRefEntryType type = XRefEntryTypeFromChar(cType);

            if (read != 5 || !CheckEOL(empty1, empty2))
            {
                // part of XrefEntry is missing, or i/o error
                PDFMM_RAISE_ERROR(PdfErrorCode::InvalidXRef);
            }

            switch (type)
            {
                case XRefEntryType::Free:
                {
                    // The variant is the number of the next free object
                    entry.ObjectNumber = variant;
                    break;
                }
                case XRefEntryType::InUse:
                {
                    // Support also files with whitespace offset before magic start
                    variant += (uint64_t)m_magicOffset;
                    if (variant > PTRDIFF_MAX)
                    {
                        // max size is PTRDIFF_MAX, so throw error if llOffset too big
                        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
                    }

                    entry.Offset = variant;
                    break;
                }
                default:
                {
                    // This flow should have beeb alredy been cathed earlier
                    PDFMM_ASSERT(false);
                }
            }

            entry.Generation = generation;
            entry.Type = type;
            entry.Parsed = true;
        }

        count++;
    }

    if (count != objectCount)
    {
        PdfError::LogMessage(LogSeverity::Warning, "Count of readobject is %i. Expected %" PDF_FORMAT_INT64 ".", count, objectCount);
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
    }

}

void PdfParser::ReadXRefStreamContents(const PdfRefCountedInputDevice& device, size_t offset, bool readOnlyTrailer)
{
    PdfRecursionGuard guard(m_RecursionDepth);

    device.Device()->Seek(offset);

    PdfXRefStreamParserObject xrefObject(m_Objects->GetDocument(), device, m_buffer, m_entries);
    xrefObject.Parse();

    if (m_Trailer == nullptr)
        m_Trailer.reset(new PdfParserObject(m_Objects->GetDocument(), device, m_buffer));

    MergeTrailer(xrefObject);

    if (readOnlyTrailer)
        return;

    xrefObject.ReadXRefTable();

    // Check for a previous XRefStm or xref table
    size_t previousOffset;
    if (xrefObject.TryGetPreviousOffset(previousOffset) && previousOffset != offset)
    {
        try
        {
            m_IncrementalUpdateCount++;

            // PDFs that have been through multiple PDF tools may have a mix of xref tables (ISO 32000-1 7.5.4) 
            // and XRefStm streams (ISO 32000-1 7.5.8.1) and in the Prev chain, 
            // so call ReadXRefContents (which deals with both) instead of ReadXRefStreamContents 
            ReadXRefContents(device, previousOffset, readOnlyTrailer);
        }
        catch (PdfError& e)
        {
            // Be forgiving, the error happens when an entry in XRef
            // stream points to a wrong place (offset) in the PDF file.
            if (e != PdfErrorCode::NoNumber)
            {
                e.AddToCallstack(__FILE__, __LINE__);
                throw e;
            }
        }
    }
}

void PdfParser::ReadObjects(const PdfRefCountedInputDevice& device)
{
    int i = 0;
    PdfParserObject* obj = nullptr;

    m_Objects->Reserve(m_objectCount);

    // Check for encryption and make sure that the encryption object
    // is loaded before all other objects
    PdfObject* encrypt = m_Trailer->GetDictionary().GetKey("Encrypt");
    if (encrypt != nullptr && !encrypt->IsNull())
    {
#ifdef PDFMM_VERBOSE_DEBUG
        PdfError::DebugMessage("The PDF file is encrypted.\n");
#endif // PDFMM_VERBOSE_DEBUG

        if (encrypt->IsReference())
        {
            i = encrypt->GetReference().ObjectNumber();
            if (i <= 0 || static_cast<size_t>(i) >= m_entries.size())
            {
                ostringstream oss;
                oss << "Encryption dictionary references a nonexistent object " << encrypt->GetReference().ObjectNumber() << " "
                    << encrypt->GetReference().GenerationNumber();
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, oss.str().c_str());
            }

            obj = new PdfParserObject(m_Objects->GetDocument(), device, m_buffer, (ssize_t)m_entries[i].Offset);
            obj->SetLoadOnDemand(false); // Never load this on demand, as we will use it immediately
            try
            {
                obj->ParseFile(nullptr); // The encryption dictionary is not encrypted
                // Never add the encryption dictionary to m_Objects
                // we create a new one, if we need it for writing
                // m_Objects->push_back( obj );
                m_entries[i].Parsed = false;
                m_Encrypt = PdfEncrypt::CreatePdfEncrypt(obj);
                delete obj;
            }
            catch (PdfError& e)
            {
                ostringstream oss;
                oss << "Error while loading object " << obj->GetIndirectReference().ObjectNumber() << " "
                    << obj->GetIndirectReference().GenerationNumber() << std::endl;
                delete obj;

                e.AddToCallstack(__FILE__, __LINE__, oss.str().c_str());
                throw e;

            }
        }
        else if (encrypt->IsDictionary())
        {
            m_Encrypt = PdfEncrypt::CreatePdfEncrypt(encrypt);
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                "The encryption entry in the trailer is neither an object nor a reference.");
        }

        // Generate encryption keys
        bool isAuthenticated = m_Encrypt->Authenticate(m_password, this->GetDocumentId());
        if (!isAuthenticated)
        {
            // authentication failed so we need a password from the user.
            // The user can set the password using PdfParser::SetPassword
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidPassword, "A password is required to read this PDF file.");
        }
    }

    ReadObjectsInternal(device);
}

void PdfParser::ReadObjectsInternal(const PdfRefCountedInputDevice& device)
{
    unsigned last = 0;
    PdfParserObject* obj = nullptr;

    // Read objects
    for (unsigned i = 0; i < m_objectCount; i++)
    {
        PdfXRefEntry& entry = m_entries[i];
#ifdef PDFMM_VERBOSE_DEBUG
        std::cerr << "ReadObjectsInteral\t" << i << " "
            << (entry.Parsed ? "parsed" : "unparsed") << " "
            << entry.Offset << " "
            << entry.Generation << std::endl;
#endif
        if (entry.Parsed)
        {
            switch (entry.Type)
            {
                case XRefEntryType::InUse:
                {
                    if (entry.Offset > 0)
                    {
                        obj = new PdfParserObject(m_Objects->GetDocument(), device, m_buffer, (ssize_t)entry.Offset);
                        auto reference = obj->GetIndirectReference();
                        if (reference.GenerationNumber() != entry.Generation)
                        {
                            if (m_StrictParsing)
                            {
                                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
                                    "Found object with generation different than reported in XRef sections");
                            }
                            else
                            {
                                PdfError::LogMessage(LogSeverity::Warning,
                                    "Found object with generation different than reported in XRef sections");
                            }
                        }

                        obj->SetLoadOnDemand(m_LoadOnDemand);
                        try
                        {
                            obj->ParseFile(m_Encrypt.get());
                            if (m_Encrypt && obj->IsDictionary())
                            {
                                PdfObject* pObjType = obj->GetDictionary().GetKey(PdfName::KeyType);
                                if (pObjType && pObjType->IsName() && pObjType->GetName() == "XRef")
                                {
                                    // XRef is never encrypted
                                    delete obj;
                                    obj = new PdfParserObject(m_Objects->GetDocument(), device, m_buffer, (ssize_t)entry.Offset);
                                    reference = obj->GetIndirectReference();
                                    obj->SetLoadOnDemand(m_LoadOnDemand);
                                    obj->ParseFile(nullptr);
                                }
                            }
                            last = reference.ObjectNumber();

                            // final pdf should not contain a linerization dictionary as it contents are invalid 
                            // as we change some objects and the final xref table
                            if (m_Linearization && last == m_Linearization->GetIndirectReference().ObjectNumber())
                            {
                                m_Objects->SafeAddFreeObject(reference);
                                delete obj;
                            }
                            else
                            {
                                m_Objects->AddObject(obj);
                            }
                        }
                        catch (PdfError& e)
                        {
                            ostringstream oss;
                            oss << "Error while loading object " << obj->GetIndirectReference().ObjectNumber()
                                << " " << obj->GetIndirectReference().GenerationNumber()
                                << " Offset = " << entry.Offset
                                << " Index = " << i << std::endl;
                            delete obj;

                            if (m_IgnoreBrokenObjects)
                            {
                                PdfError::LogMessage(LogSeverity::Error, oss.str().c_str());
                                m_Objects->SafeAddFreeObject(reference);
                            }
                            else
                            {
                                e.AddToCallstack(__FILE__, __LINE__, oss.str().c_str());
                                throw e;
                            }
                        }
                    }
                    else if (entry.Generation == 0)
                    {
                        assert(entry.Offset == 0);
                        // There are broken PDFs which add objects with 'n' 
                        // and 0 offset and 0 generation number
                        // to the xref table instead of using free objects
                        // treating them as free objects
                        if (m_StrictParsing)
                        {
                            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
                                "Found object with 0 offset which should be 'f' instead of 'n'.");
                        }
                        else
                        {
                            PdfError::LogMessage(LogSeverity::Warning,
                                "Treating object %i 0 R as a free object.");
                            m_Objects->AddFreeObject(PdfReference(i, 1));
                        }
                    }
                    break;
                }
                case XRefEntryType::Free:
                {
                    // NOTE: We don't need entry.ObjectNumber, which is supposed to be
                    // the entry of the next free object
                    if (i != 0)
                        m_Objects->SafeAddFreeObject(PdfReference(i, entry.Generation));

                    break;
                }
                case XRefEntryType::Compressed:
                    // CHECK-ME
                    break;
                default:
                    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);

            }
        }
        else if (i != 0) // Unparsed
        {
            m_Objects->AddFreeObject(PdfReference(i, 1));
        }
        // the linked free list in the xref section is not always correct in pdf's
        // (especially Illustrator) but Acrobat still accepts them. I've seen XRefs 
        // where some object-numbers are alltogether missing and multiple XRefs where 
        // the link list is broken.
        // Because PdfVecObjects relies on a unbroken range, fill the free list more
        // robustly from all places which are either free or unparsed
    }

    // all normal objects including object streams are available now,
    // we can parse the object streams safely now.
    //
    // Note that even if demand loading is enabled we still currently read all
    // objects from the stream into memory then free the stream.
    //
    for (unsigned i = 0; i < m_objectCount; i++)
    {
        PdfXRefEntry& entry = m_entries[i];
        if (entry.Parsed && entry.Type == XRefEntryType::Compressed) // we have an compressed object stream
        {
#ifndef VERBOSE_DEBUG_DISABLED
            if (m_LoadOnDemand) cerr << "Demand loading on, but can't demand-load from object stream." << endl;
#endif
            ReadCompressedObjectFromStream((uint32_t)entry.ObjectNumber, static_cast<int>(entry.Index));
        }
    }

    if (!m_LoadOnDemand)
    {
        // Force loading of streams. We can't do this during the initial
        // run that populates m_Objects because a stream might have a /Length
        // key that references an object we haven't yet read. So we must do it here
        // in a second pass, or (if demand loading is enabled) defer it for later.
        for (auto objToLoad : *m_Objects)
        {
            obj = dynamic_cast<PdfParserObject*>(objToLoad);
            obj->ForceStreamParse();
        }
    }


    // Now sort the list of objects
    m_Objects->Sort();

    UpdateDocumentVersion();
}

void PdfParser::ReadCompressedObjectFromStream(uint32_t objNo, int index)
{
    // NOTE: index of the compressed object is ignored since
    // we eagerly read all the objects from the stream
    (void)index;

    // If we already have read all objects from this stream just return
    if (m_ObjectStreams.find(objNo) != m_ObjectStreams.end())
        return;

    m_ObjectStreams.insert(objNo);

    // generation number of object streams is always 0
    PdfParserObject* stream = dynamic_cast<PdfParserObject*>(m_Objects->GetObject(PdfReference(objNo, 0)));
    if (stream == nullptr)
    {
        ostringstream oss;
        oss << "Loading of object " << objNo << " 0 R failed!" << std::endl;

        if (m_IgnoreBrokenObjects)
        {
            PdfError::LogMessage(LogSeverity::Error, oss.str().c_str());
            return;
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, oss.str().c_str());
        }
    }

    PdfObjectStreamParser::ObjectIdList list;
    for (unsigned i = 0; i < m_objectCount; i++)
    {
        PdfXRefEntry& entry = m_entries[i];
        if (entry.Parsed && entry.Type == XRefEntryType::Compressed &&
            m_entries[i].ObjectNumber == objNo)
        {
            list.push_back(static_cast<int64_t>(i));
        }
    }

    PdfObjectStreamParser parserObject(*stream, *m_Objects, m_buffer);
    parserObject.Parse(list);
}

const char* PdfParser::GetPdfVersionString() const
{
    return s_PdfVersions[static_cast<int>(m_PdfVersion)];
}

void PdfParser::FindToken(const PdfRefCountedInputDevice& device, const char* token, size_t range)
{
    // James McGill 18.02.2011, offset read position to the EOF marker if it is not the last thing in the file
    device.Device()->Seek(-(streamoff)m_LastEOFOffset, ios_base::end);

    streamoff fileSize = device.Device()->Tell();
    if (fileSize == -1)
    {
        PDFMM_RAISE_ERROR_INFO(
            PdfErrorCode::NoXRef,
            "Failed to seek to EOF when looking for xref");
    }

    size_t xRefBuf = std::min(static_cast<size_t>(fileSize), range);
    size_t tokenLen = strlen(token);

    device.Device()->Seek(-(streamoff)xRefBuf, ios_base::cur);
    if (device.Device()->Read(m_buffer.GetBuffer(), xRefBuf) != xRefBuf && !device.Device()->Eof())
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    m_buffer.GetBuffer()[xRefBuf] = '\0';

    ssize_t i; // Do not make this unsigned, this will cause infinte loops in files without trailer

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for (i = xRefBuf - tokenLen; i >= 0; i--)
    {
        if (strncmp(m_buffer.GetBuffer() + i, token, tokenLen) == 0)
            break;
    }

    if (i == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);

    // James McGill 18.02.2011, offset read position to the EOF marker if it is not the last thing in the file
    device.Device()->Seek(-((streamoff)xRefBuf - i) - m_LastEOFOffset, ios_base::end);
}

// Peter Petrov 23 December 2008
void PdfParser::FindToken2(const PdfRefCountedInputDevice& device, const char* token, const size_t range, size_t searchEnd)
{
    device.Device()->Seek(searchEnd, ios_base::beg);

    streamoff fileSize = device.Device()->Tell();
    if (fileSize == -1)
    {
        PDFMM_RAISE_ERROR_INFO(
            PdfErrorCode::NoXRef,
            "Failed to seek to EOF when looking for xref");
    }

    size_t xRefBuf = std::min(static_cast<size_t>(fileSize), range);
    size_t tokenLen = strlen(token);

    device.Device()->Seek(-(streamoff)xRefBuf, ios_base::cur);
    if (device.Device()->Read(m_buffer.GetBuffer(), xRefBuf) != xRefBuf && !device.Device()->Eof())
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    m_buffer.GetBuffer()[xRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    ssize_t i; // Do not use an unsigned variable here
    for (i = xRefBuf - tokenLen; i >= 0; i--)
    {
        if (strncmp(m_buffer.GetBuffer() + i, token, tokenLen) == 0)
            break;
    }

    if (!i)
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);
    }

    device.Device()->Seek(searchEnd - ((streamoff)xRefBuf - i), ios_base::beg);
}

const PdfString& PdfParser::GetDocumentId()
{
    if (!m_Trailer->GetDictionary().HasKey("ID"))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "No document ID found in trailer.");

    return m_Trailer->GetDictionary().GetKey("ID")->GetArray()[0].GetString();
}

void PdfParser::UpdateDocumentVersion()
{
    if (m_Trailer->IsDictionary() && m_Trailer->GetDictionary().HasKey("Root"))
    {
        auto catalog = m_Trailer->GetDictionary().FindKey("Root");
        if (catalog != nullptr
            && catalog->IsDictionary()
            && catalog->GetDictionary().HasKey("Version"))
        {
            auto& version = catalog->GetDictionary().MustGetKey("Version");
            for (int i = 0; i <= MAX_PDF_VERSION_STRING_INDEX; i++)
            {
                if (IsStrictParsing() && !version.IsName())
                {
                    // Version must be of type name, according to PDF Specification
                    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidName);
                }

                if (version.IsName() && version.GetName().GetString() == s_PdfVersionNums[i])
                {
                    PdfError::LogMessage(LogSeverity::Information,
                        "Updating version from %s to %s",
                        s_PdfVersionNums[static_cast<int>(m_PdfVersion)],
                        s_PdfVersionNums[i]);
                    m_PdfVersion = static_cast<PdfVersion>(i);
                    break;
                }
            }
        }
    }
}

void PdfParser::ResizeEntries(size_t newSize)
{
    // allow caller to specify a max object count to avoid very slow load times on large documents
    if (newSize > MaxNumIndirectObjects)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "newSize is greater than m_MaxObjects.");


    m_entries.resize(newSize);
}

void PdfParser::CheckEOFMarker(const PdfRefCountedInputDevice& device)
{
    // Check for the existence of the EOF marker
    m_LastEOFOffset = 0;
    const char* EOFToken = "%%EOF";
    constexpr size_t EOFTokenLen = 5;
    char buff[EOFTokenLen + 1];

    device.Device()->Seek(-static_cast<int>(EOFTokenLen), ios_base::end);
    if (IsStrictParsing())
    {
        // For strict mode EOF marker must be at the very end of the file
        if (static_cast<size_t>(device.Device()->Read(buff, EOFTokenLen)) != EOFTokenLen
            && !device.Device()->Eof())
            PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);

        if (strncmp(buff, EOFToken, EOFTokenLen) != 0)
            PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);
    }
    else
    {
        // Search for the Marker from the end of the file
        ssize_t currentPos = device.Device()->Tell();

        bool found = false;
        while (currentPos >= 0)
        {
            if (static_cast<size_t>(device.Device()->Read(buff, EOFTokenLen)) != EOFTokenLen)
                PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);

            if (strncmp(buff, EOFToken, EOFTokenLen) == 0)
            {
                found = true;
                break;
            }

            currentPos--;
            device.Device()->Seek(currentPos, ios_base::beg);
        }

        // Try and deal with garbage by offsetting the buffer reads in PdfParser from now on
        if (found)
            m_LastEOFOffset = (m_FileSize - (device.Device()->Tell() - 1)) + EOFTokenLen;
        else
            PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);
    }
}

const PdfObject& PdfParser::GetTrailer() const
{
    if (m_Trailer == nullptr)
        PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

    return *m_Trailer;
}

bool PdfParser::IsLinearized() const
{
    return m_Linearization != nullptr;
}

bool PdfParser::IsEncrypted() const
{
    return m_Encrypt != nullptr;
}

std::unique_ptr<PdfEncrypt> PdfParser::TakeEncrypt()
{
    return std::move(m_Encrypt);
}


// Read magic word keeping cursor
bool ReadMagicWord(char ch, int& cursoridx)
{
    bool readchar;
    switch (cursoridx)
    {
        case 0:
            readchar = ch == '%';
            break;
        case 1:
            readchar = ch == 'P';
            break;
        case 2:
            readchar = ch == 'D';
            break;
        case 3:
            readchar = ch == 'F';
            break;
        case 4:
            readchar = ch == '-';
            if (readchar)
                return true;

            break;
        default:
            throw std::runtime_error("Unexpected flow");
    }

    if (readchar)
    {
        // Advance cursor
        cursoridx++;
    }
    else
    {
        // Reset cursor
        cursoridx = 0;
    }

    return false;
}
