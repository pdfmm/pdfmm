/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
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
using namespace PoDoFo;

static bool CheckEOL(char e1, char e2);
static bool CheckXRefEntryType(char c);
static bool ReadMagicWord(char ch, int& charidx);

constexpr int64_t nMaxNumIndirectObjects = (1 << 23) - 1;

class PdfRecursionGuard
{
  // RAII recursion guard ensures m_nRecursionDepth is always decremented
  // because the destructor is always called when control leaves a method
  // via return or an exception.
  // see http://en.cppreference.com/w/cpp/language/raii

  // It's used like this in PdfParser methods
  // PdfRecursionGuard guard(m_nRecursionDepth);

public:
    PdfRecursionGuard(int& nRecursionDepth)
        : m_nRecursionDepth(nRecursionDepth)
    {
        // be careful changing this limit - overflow limits depend on the OS, linker settings, and how much stack space compiler allocates
        // 500 limit prevents overflow on Win7 with VC++ 2005 with default linker stack size (1000 caused overflow with same compiler/OS)
        const int maxRecursionDepth = 500;

        ++m_nRecursionDepth;

        if (m_nRecursionDepth > maxRecursionDepth)
        {
            // avoid stack overflow on documents that have circular cross references in /Prev entries
            // in trailer and XRef streams (possible via a chain of entries with a loop)
            PODOFO_RAISE_ERROR(EPdfError::InvalidXRef);
        }
    }

    ~PdfRecursionGuard()
    {
        --m_nRecursionDepth;
    }

private:
    // must be a reference so that we modify m_nRecursionDepth in parent class
    int& m_nRecursionDepth;
};

PdfParser::PdfParser(PdfVecObjects& pVecObjects) :
    m_buffer(PdfTokenizer::BufferSize),
    m_tokenizer(m_buffer, true),
    m_vecObjects(&pVecObjects),
    m_bStrictParsing(false)
{
    this->Reset();
}

PdfParser::~PdfParser()
{
    Reset();
}

void PdfParser::Reset()
{
    m_ePdfVersion = PdfVersionDefault;
    m_bLoadOnDemand = false;

    m_magicOffset = 0;
    m_HasXRefStream = false;
    m_nXRefOffset = 0;
    m_nNumObjects = 0;
    m_nXRefLinearizedOffset = 0;
    m_lLastEOFOffset = 0;

    m_pTrailer = nullptr;
    m_pLinearization = nullptr;
    m_entries.clear();
    m_setObjectStreams.clear();

    m_pEncrypt = nullptr;

    m_bIgnoreBrokenObjects = true;
    m_nIncrementalUpdates = 0;
    m_nRecursionDepth = 0;
}

void PdfParser::ParseFile(const string_view& filename, bool bLoadOnDemand)
{
    if (filename.length() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    PdfRefCountedInputDevice device(filename);
    if (!device.Device())
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::FileNotFound, filename.data());
    }

    this->Parse(device, bLoadOnDemand);
}

void PdfParser::ParseBuffer(const string_view& buffer, bool bLoadOnDemand)
{
    if (buffer.length() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    PdfRefCountedInputDevice device(buffer.data(), buffer.length());
    if (!device.Device())
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "Cannot create PdfParser from buffer.");
    }

    this->Parse(device, bLoadOnDemand);
}

void PdfParser::Parse(const PdfRefCountedInputDevice& device, bool bLoadOnDemand)
{
    Reset();

    m_bLoadOnDemand = bLoadOnDemand;

    try
    {
        if (!IsPdfFile(device))
            PODOFO_RAISE_ERROR(EPdfError::NoPdfFile);

        ReadDocumentStructure(device);
        ReadObjects(device);
    }
    catch (PdfError& e)
    {
        if (e.GetError() == EPdfError::InvalidPassword)
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
    m_nFileSize = device.Device()->Tell();

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
        ReadXRef(device, &m_nXRefOffset);
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

    if (m_pLinearization)
    {
        try
        {
            ReadXRefContents(device, m_nXRefOffset, true);
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
            if (e != EPdfError::NoTrailer)
                throw e;
        }
    }

    if (m_pTrailer->IsDictionary() && m_pTrailer->GetDictionary().HasKey(PdfName::KeySize))
    {
        m_nNumObjects = static_cast<unsigned>(m_pTrailer->GetDictionary().FindKeyAs<int64_t>(PdfName::KeySize));
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Warning, "PDF Standard Violation: No /Size key was specified in the trailer directory. Will attempt to recover.");
        // Treat the xref size as unknown, and expand the xref dynamically as we read it.
        m_nNumObjects = 0;
    }

    if (m_nNumObjects > 0)
    {
        ResizeOffsets(m_nNumObjects);
    }

    if (m_pLinearization)
    {
        try
        {
            ReadXRefContents(device, m_nXRefLinearizedOffset);
        }
        catch (PdfError& e)
        {
            e.AddToCallstack(__FILE__, __LINE__, "Unable to read linearized XRef section.");
            throw e;
        }
    }

    try
    {
        ReadXRefContents(device, m_nXRefOffset);
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
        if (strncmp(version, s_szPdfVersionNums[i], PDF_VERSION_LENGHT) == 0)
        {
            m_ePdfVersion = static_cast<PdfVersion>(i);
            return true;
        }
    }

    return false;
}

void PdfParser::HasLinearizationDict(const PdfRefCountedInputDevice& device)
{
    if (m_pLinearization)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic,
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

    //begin L.K
    //char * pszObj = strstr( m_buffer.GetBuffer(), "obj" );
    char* pszObj = strstr(linearizeBuffer.GetBuffer(), "obj");
    //end L.K
    if (!pszObj)
        // strange that there is no obj in the first 1024 bytes,
        // but ignore it
        return;

    --pszObj; // *pszObj == 'o', so the while would fail without decrement
    while (*pszObj && (PdfTokenizer::IsWhitespace(*pszObj) || (*pszObj >= '0' && *pszObj <= '9')))
        --pszObj;

    m_pLinearization.reset(new PdfParserObject(m_vecObjects->GetDocument(), device, linearizeBuffer,
        pszObj - linearizeBuffer.GetBuffer() + 2));

    try
    {
        // Do not care for encryption here, as the linearization dictionary does not contain strings or streams
        // ... hint streams do, but we do not load the hintstream.
        m_pLinearization->ParseFile(nullptr);
        if (!(m_pLinearization->IsDictionary() &&
            m_pLinearization->GetDictionary().HasKey("Linearized")))
        {
            m_pLinearization = nullptr;
            return;
        }
    }
    catch (PdfError& e)
    {
        PdfError::LogMessage(LogSeverity::Warning, e.ErrorName(e.GetError()));
        m_pLinearization = nullptr;
        return;
    }

    int64_t lXRef = -1;
    lXRef = m_pLinearization->GetDictionary().FindKeyAs<int64_t>("T", lXRef);
    if (lXRef == -1)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidLinearization);
    }

    // avoid moving to a negative file position here
    device.Device()->Seek(static_cast<size_t>(lXRef - PDF_XREF_BUF) > 0 ? static_cast<size_t>(lXRef - PDF_XREF_BUF) : PDF_XREF_BUF);
    m_nXRefLinearizedOffset = device.Device()->Tell();

    if (device.Device()->Read(m_buffer.GetBuffer(), PDF_XREF_BUF) != PDF_XREF_BUF)
        PODOFO_RAISE_ERROR(EPdfError::InvalidLinearization);

    m_buffer.GetBuffer()[PDF_XREF_BUF] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    const int XREF_LEN = 4; // strlen( "xref" );
    int i = 0;
    char* pszStart = nullptr;
    for (i = PDF_XREF_BUF - XREF_LEN; i >= 0; i--)
    {
        if (strncmp(m_buffer.GetBuffer() + i, "xref", XREF_LEN) == 0)
        {
            pszStart = m_buffer.GetBuffer() + i;
            break;
        }
    }

    m_nXRefLinearizedOffset += i;

    if (!pszStart)
    {
        if (m_ePdfVersion < PdfVersion::V1_5)
        {
            PdfError::LogMessage(LogSeverity::Warning,
                "Linearization dictionaries are only supported with PDF version 1.5. This is 1.%i. Trying to continue.\n",
                static_cast<int>(m_ePdfVersion));
            // PODOFO_RAISE_ERROR( EPdfError::InvalidLinearization );
        }

        {
            m_nXRefLinearizedOffset = static_cast<size_t>(lXRef);
            //eCode = ReadXRefStreamContents();
            //i     = 0;
        }
    }
}

void PdfParser::MergeTrailer(const PdfObject* pTrailer)
{
    PdfVariant  cVar;

    if (!pTrailer || !m_pTrailer)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    // Only update keys, if not already present                   
    if (pTrailer->GetDictionary().HasKey(PdfName::KeySize)
        && !m_pTrailer->GetDictionary().HasKey(PdfName::KeySize))
        m_pTrailer->GetDictionary().AddKey(PdfName::KeySize, *(pTrailer->GetDictionary().GetKey(PdfName::KeySize)));

    if (pTrailer->GetDictionary().HasKey("Root")
        && !m_pTrailer->GetDictionary().HasKey("Root"))
        m_pTrailer->GetDictionary().AddKey("Root", *(pTrailer->GetDictionary().GetKey("Root")));

    if (pTrailer->GetDictionary().HasKey("Encrypt")
        && !m_pTrailer->GetDictionary().HasKey("Encrypt"))
        m_pTrailer->GetDictionary().AddKey("Encrypt", *(pTrailer->GetDictionary().GetKey("Encrypt")));

    if (pTrailer->GetDictionary().HasKey("Info")
        && !m_pTrailer->GetDictionary().HasKey("Info"))
        m_pTrailer->GetDictionary().AddKey("Info", *(pTrailer->GetDictionary().GetKey("Info")));

    if (pTrailer->GetDictionary().HasKey("ID")
        && !m_pTrailer->GetDictionary().HasKey("ID"))
        m_pTrailer->GetDictionary().AddKey("ID", *(pTrailer->GetDictionary().GetKey("ID")));
}

void PdfParser::ReadNextTrailer(const PdfRefCountedInputDevice& device)
{
    PdfRecursionGuard guard(m_nRecursionDepth);

    if (m_tokenizer.IsNextToken(*device.Device(), "trailer"))
    {
        PdfParserObject trailer(m_vecObjects->GetDocument(), device, m_buffer);
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
        MergeTrailer(&trailer);

        if (trailer.GetDictionary().HasKey("XRefStm"))
        {
            // Whenever we read a XRefStm key, 
            // we know that the file was updated.
            if (!trailer.GetDictionary().HasKey("Prev"))
                m_nIncrementalUpdates++;

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
            m_nIncrementalUpdates++;

            try
            {
                size_t lOffset = static_cast<size_t>(trailer.GetDictionary().FindKeyAs<int64_t>("Prev", 0));

                if (m_visitedXRefOffsets.find(lOffset) == m_visitedXRefOffsets.end())
                    ReadXRefContents(device, lOffset);
                else
                    PdfError::LogMessage(LogSeverity::Warning, "XRef contents at offset %" PDF_FORMAT_INT64 " requested twice, skipping the second read", static_cast<int64_t>(lOffset));
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
        PODOFO_RAISE_ERROR(EPdfError::NoTrailer);
    }
}

void PdfParser::ReadTrailer(const PdfRefCountedInputDevice& device)
{
    FindToken(device, "trailer", PDF_XREF_BUF);

    if (!m_tokenizer.IsNextToken(*device.Device(), "trailer"))
    {
        //      if( m_ePdfVersion < EPdfVersion::V1_5 )
        //		Ulrich Arnold 19.10.2009, found linearized 1.3-pdf's with trailer-info in xref-stream
        if (m_ePdfVersion < PdfVersion::V1_3)
        {
            PODOFO_RAISE_ERROR(EPdfError::NoTrailer);
        }
        else
        {
            // Since PDF 1.5 trailer information can also be found
            // in the crossreference stream object
            // and a trailer dictionary is not required
            device.Device()->Seek(m_nXRefOffset);

            m_pTrailer.reset(new PdfParserObject(m_vecObjects->GetDocument(), device, m_buffer));
            m_pTrailer->ParseFile(nullptr, false);
            return;
        }
    }
    else
    {
        m_pTrailer.reset(new PdfParserObject(m_vecObjects->GetDocument(), device, m_buffer));
        try
        {
            // Ignore the encryption in the trailer as the trailer may not be encrypted
            m_pTrailer->ParseFile(nullptr, true);
        }
        catch (PdfError& e)
        {
            e.AddToCallstack(__FILE__, __LINE__, "The trailer was found in the file, but contains errors.");
            throw e;
        }
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("Size=%" PDF_FORMAT_INT64 "\n", m_pTrailer->GetDictionary().GetAs<int64_t>(PdfName::KeySize, 0));
#endif // PODOFO_VERBOSE_DEBUG
    }
}

void PdfParser::ReadXRef(const PdfRefCountedInputDevice& device, size_t* pXRefOffset)
{
    FindToken(device, "startxref", PDF_XREF_BUF);

    if (!m_tokenizer.IsNextToken(*device.Device(), "startxref"))
    {
        // Could be non-standard startref
        if (!m_bStrictParsing)
        {
            FindToken(device, "startref", PDF_XREF_BUF);
            if (!m_tokenizer.IsNextToken(*device.Device(), "startref"))
                PODOFO_RAISE_ERROR(EPdfError::NoXRef);
        }
        else
        {
            PODOFO_RAISE_ERROR(EPdfError::NoXRef);
        }
    }

    // Support also files with whitespace offset before magic start
    *pXRefOffset = (size_t)m_tokenizer.ReadNextNumber(*device.Device()) + m_magicOffset;
}

void PdfParser::ReadXRefContents(const PdfRefCountedInputDevice& device, size_t lOffset, bool bPositionAtEnd)
{
    PdfRecursionGuard guard(m_nRecursionDepth);

    int64_t nFirstObject = 0;
    int64_t nNumObjects = 0;

    if (m_visitedXRefOffsets.find(lOffset) != m_visitedXRefOffsets.end())
    {
        std::ostringstream oss;
        oss << "Cycle in xref structure. Offset  "
            << lOffset << " already visited.";

        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidXRef, oss.str());
    }
    else
    {
        m_visitedXRefOffsets.insert(lOffset);
    }


    size_t curPosition = device.Device()->Tell();
    device.Device()->Seek(0, ios_base::end);
    size_t fileSize = device.Device()->Tell();
    device.Device()->Seek(curPosition, ios_base::beg);

    if (lOffset > fileSize)
    {
        // Invalid "startxref" Peter Petrov 23 December 2008
         // ignore returned value and get offset from the device
        ReadXRef(device, &lOffset);
        lOffset = device.Device()->Tell();
        // TODO: hard coded value "4"
        m_buffer.Resize(PDF_XREF_BUF * 4);
        FindToken2(device, "xref", PDF_XREF_BUF * 4, lOffset);
        m_buffer.Resize(PDF_XREF_BUF);
        lOffset = device.Device()->Tell();
        m_nXRefOffset = lOffset;
    }
    else
    {
        device.Device()->Seek(lOffset);
    }

    if (!m_tokenizer.IsNextToken(*device.Device(), "xref"))
    {
        //		Ulrich Arnold 19.10.2009, found linearized 1.3-pdf's with trailer-info in xref-stream
        if (m_ePdfVersion < PdfVersion::V1_3)
        {
            PODOFO_RAISE_ERROR(EPdfError::NoXRef);
        }
        else
        {
            m_HasXRefStream = true;
            ReadXRefStreamContents(device, lOffset, bPositionAtEnd);
            return;
        }
    }

    // read all xref subsections
    // OC 13.08.2010: Avoid exception to terminate endless loop
    for (int nXrefSection = 0; ; ++nXrefSection)
    {
        try
        {
            // OC 13.08.2010: Avoid exception to terminate endless loop
            if (nXrefSection > 0)
            {
                // something like PeekNextToken()
                EPdfTokenType eType;
                string_view pszRead;
                bool gotToken = m_tokenizer.TryReadNextToken(*device.Device(), pszRead, eType);
                if (gotToken)
                {
                    m_tokenizer.EnqueueToken(pszRead, eType);
                    if (pszRead == "trailer")
                        break;
                }
            }

            nFirstObject = m_tokenizer.ReadNextNumber(*device.Device());
            nNumObjects = m_tokenizer.ReadNextNumber(*device.Device());

#ifdef PODOFO_VERBOSE_DEBUG
            PdfError::DebugMessage("Reading numbers: %" PDF_FORMAT_INT64 " %" PDF_FORMAT_INT64 "\n", nFirstObject, nNumObjects);
#endif // PODOFO_VERBOSE_DEBUG

            if (bPositionAtEnd)
            {
#ifdef WIN32
                device.Device()->Seek(static_cast<streamoff>(nNumObjects * PDF_XREF_ENTRY_SIZE), ios_base::cur);
#else
                device.Device()->Seek(nNumObjects * PDF_XREF_ENTRY_SIZE, ios_base::cur);
#endif // WIN32
            }
            else
            {
                ReadXRefSubsection(device, nFirstObject, nNumObjects);
            }
        }
        catch (PdfError& e)
        {
            if (e == EPdfError::NoNumber || e == EPdfError::InvalidXRef || e == EPdfError::UnexpectedEOF)
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
        if (e != EPdfError::NoTrailer)
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

void PdfParser::ReadXRefSubsection(const PdfRefCountedInputDevice& device, int64_t& nFirstObject, int64_t& nNumObjects)
{
    int64_t count = 0;

#ifdef PODOFO_VERBOSE_DEBUG
    PdfError::DebugMessage("Reading XRef Section: %" PDF_FORMAT_INT64 " with %" PDF_FORMAT_INT64 " Objects.\n", nFirstObject, nNumObjects);
#endif // PODOFO_VERBOSE_DEBUG 

    if (nFirstObject < 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "ReadXRefSubsection: nFirstObject is negative");
    if (nNumObjects < 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "ReadXRefSubsection: nNumObjects is negative");

    // overflow guard, fixes CVE-2017-5853 (signed integer overflow)
    // also fixes CVE-2017-6844 (buffer overflow) together with below size check
    if (nMaxNumIndirectObjects >= nNumObjects && nFirstObject <= (nMaxNumIndirectObjects - nNumObjects))
    {
        if (nFirstObject + nNumObjects > m_nNumObjects)
        {
            // Total number of xref entries to read is greater than the /Size
            // specified in the trailer if any. That's an error unless we're
            // trying to recover from a missing /Size entry.
            PdfError::LogMessage(LogSeverity::Warning,
                "There are more objects (%" PDF_FORMAT_INT64 ") in this XRef "
                "table than specified in the size key of the trailer directory "
                "(%" PDF_FORMAT_INT64 ")!\n", nFirstObject + nNumObjects,
                static_cast<int64_t>(m_nNumObjects));
        }

        if (static_cast<uint64_t>(nFirstObject) + static_cast<uint64_t>(nNumObjects) > static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange,
                "xref subsection's given entry numbers together too large");
        }

        if (nFirstObject + nNumObjects > m_nNumObjects)
        {
            try
            {
                m_nNumObjects = static_cast<unsigned>(nFirstObject + nNumObjects);
                ResizeOffsets((size_t)(nFirstObject + nNumObjects));

            }
            catch (std::exception&) {
                // If m_nNumObjects*sizeof(TXRefEntry) > std::numeric_limits<size_t>::max() then
                // resize() throws std::length_error, for smaller allocations that fail it may throw
                // std::bad_alloc (implementation-dependent). "20.5.5.12 Restrictions on exception 
                // handling" in the C++ Standard says any function that throws an exception is allowed 
                // to throw implementation-defined exceptions derived the base type (std::exception)
                // so we need to catch all std::exceptions here
                PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
            }
        }
    }
    else
    {
        PdfError::LogMessage(LogSeverity::Error, "There are more objects (%" PDF_FORMAT_INT64
            " + %" PDF_FORMAT_INT64 " seemingly) in this XRef"
            " table than supported by standard PDF, or it's inconsistent.",
            nFirstObject, nNumObjects);
        PODOFO_RAISE_ERROR(EPdfError::InvalidXRef);
    }

    // consume all whitespaces
    int charcode;
    while (m_tokenizer.IsWhitespace((charcode = device.Device()->Look())))
        (void)device.Device()->GetChar();

    while (count < nNumObjects && device.Device()->Read(m_buffer.GetBuffer(), PDF_XREF_ENTRY_SIZE) == PDF_XREF_ENTRY_SIZE)
    {
        char empty1;
        char empty2;
        m_buffer.GetBuffer()[PDF_XREF_ENTRY_SIZE] = '\0';

        const int objID = static_cast<int>(nFirstObject + count);

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
                PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidXRef, "Invalid used keyword, must be eiter 'n' or 'f'");

            EXRefEntryType eType = XRefEntryTypeFromChar(cType);

            if (read != 5 || !CheckEOL(empty1, empty2))
            {
                // part of XrefEntry is missing, or i/o error
                PODOFO_RAISE_ERROR(EPdfError::InvalidXRef);
            }

            switch (eType)
            {
                case EXRefEntryType::Free:
                {
                    // The variant is the number of the next free object
                    entry.ObjectNumber = variant;
                    break;
                }
                case EXRefEntryType::InUse:
                {
                    // Support also files with whitespace offset before magic start
                    variant += (uint64_t)m_magicOffset;
                    if (variant > PTRDIFF_MAX)
                    {
                        // max size is PTRDIFF_MAX, so throw error if llOffset too big
                        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
                    }

                    entry.Offset = variant;
                    break;
                }
                default:
                {
                    // This flow should have beeb alredy been cathed earlier
                    PODOFO_ASSERT(false);
                }
            }

            entry.Generation = generation;
            entry.Type = eType;
            entry.Parsed = true;
        }

        count++;
    }

    if (count != nNumObjects)
    {
        PdfError::LogMessage(LogSeverity::Warning, "Count of readobject is %i. Expected %" PDF_FORMAT_INT64 ".", count, nNumObjects);
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);
    }

}

void PdfParser::ReadXRefStreamContents(const PdfRefCountedInputDevice& device, size_t lOffset, bool bReadOnlyTrailer)
{
    PdfRecursionGuard guard(m_nRecursionDepth);

    device.Device()->Seek(lOffset);

    PdfXRefStreamParserObject xrefObject(m_vecObjects->GetDocument(), device, m_buffer, m_entries);
    xrefObject.Parse();

    if (!m_pTrailer)
        m_pTrailer.reset(new PdfParserObject(m_vecObjects->GetDocument(), device, m_buffer));

    MergeTrailer(&xrefObject);

    if (bReadOnlyTrailer)
        return;

    xrefObject.ReadXRefTable();

    // Check for a previous XRefStm or xref table
    size_t previousOffset;
    if (xrefObject.TryGetPreviousOffset(previousOffset) && previousOffset != lOffset)
    {
        try
        {
            m_nIncrementalUpdates++;

            // PDFs that have been through multiple PDF tools may have a mix of xref tables (ISO 32000-1 7.5.4) 
            // and XRefStm streams (ISO 32000-1 7.5.8.1) and in the Prev chain, 
            // so call ReadXRefContents (which deals with both) instead of ReadXRefStreamContents 
            ReadXRefContents(device, previousOffset, bReadOnlyTrailer);
        }
        catch (PdfError& e)
        {
            // Be forgiving, the error happens when an entry in XRef
            // stream points to a wrong place (offset) in the PDF file.
            if (e != EPdfError::NoNumber)
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
    PdfParserObject* pObject = nullptr;

    m_vecObjects->Reserve(m_nNumObjects);

    // Check for encryption and make sure that the encryption object
    // is loaded before all other objects
    PdfObject* encrypt = m_pTrailer->GetDictionary().GetKey("Encrypt");
    if (encrypt != nullptr && !encrypt->IsNull())
    {
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::DebugMessage("The PDF file is encrypted.\n");
#endif // PODOFO_VERBOSE_DEBUG

        if (encrypt->IsReference())
        {
            i = encrypt->GetReference().ObjectNumber();
            if (i <= 0 || static_cast<size_t>(i) >= m_entries.size())
            {
                ostringstream oss;
                oss << "Encryption dictionary references a nonexistent object " << encrypt->GetReference().ObjectNumber() << " "
                    << encrypt->GetReference().GenerationNumber();
                PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEncryptionDict, oss.str().c_str());
            }

            pObject = new PdfParserObject(m_vecObjects->GetDocument(), device, m_buffer, (ssize_t)m_entries[i].Offset);
            if (!pObject)
                PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);

            pObject->SetLoadOnDemand(false); // Never load this on demand, as we will use it immediately
            try
            {
                pObject->ParseFile(nullptr); // The encryption dictionary is not encrypted
                // Never add the encryption dictionary to m_vecObjects
                // we create a new one, if we need it for writing
                // m_vecObjects->push_back( pObject );
                m_entries[i].Parsed = false;
                m_pEncrypt = PdfEncrypt::CreatePdfEncrypt(pObject);
                delete pObject;
            }
            catch (PdfError& e)
            {
                ostringstream oss;
                oss << "Error while loading object " << pObject->GetIndirectReference().ObjectNumber() << " "
                    << pObject->GetIndirectReference().GenerationNumber() << std::endl;
                delete pObject;

                e.AddToCallstack(__FILE__, __LINE__, oss.str().c_str());
                throw e;

            }
        }
        else if (encrypt->IsDictionary())
        {
            m_pEncrypt = PdfEncrypt::CreatePdfEncrypt(encrypt);
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEncryptionDict,
                "The encryption entry in the trailer is neither an object nor a reference.");
        }

        // Generate encryption keys
        bool isAuthenticated = m_pEncrypt->Authenticate(m_password, this->GetDocumentId());
        if (!isAuthenticated)
        {
            // authentication failed so we need a password from the user.
            // The user can set the password using PdfParser::SetPassword
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidPassword, "A password is required to read this PDF file.");
        }
    }

    ReadObjectsInternal(device);
}

void PdfParser::ReadObjectsInternal(const PdfRefCountedInputDevice& device)
{
    unsigned nLast = 0;
    PdfParserObject* pObject = nullptr;

    // Read objects
    for (unsigned i = 0; i < m_nNumObjects; i++)
    {
        PdfXRefEntry& entry = m_entries[i];
#ifdef PODOFO_VERBOSE_DEBUG
        std::cerr << "ReadObjectsInteral\t" << i << " "
            << (entry.bParsed ? "parsed" : "unparsed") << " "
            << entry.cUsed << " "
            << entry.lOffset << " "
            << entry.lGeneration << std::endl;
#endif
        if (entry.Parsed)
        {
            switch (entry.Type)
            {
                case EXRefEntryType::InUse:
                {
                    if (entry.Offset > 0)
                    {
                        pObject = new PdfParserObject(m_vecObjects->GetDocument(), device, m_buffer, (ssize_t)entry.Offset);
                        auto reference = pObject->GetIndirectReference();
                        if (reference.GenerationNumber() != entry.Generation)
                        {
                            if (m_bStrictParsing)
                            {
                                PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidXRef,
                                    "Found object with generation different than reported in XRef sections");
                            }
                            else
                            {
                                PdfError::LogMessage(LogSeverity::Warning,
                                    "Found object with generation different than reported in XRef sections");
                            }
                        }

                        pObject->SetLoadOnDemand(m_bLoadOnDemand);
                        try
                        {
                            pObject->ParseFile(m_pEncrypt.get());
                            if (m_pEncrypt && pObject->IsDictionary())
                            {
                                PdfObject* pObjType = pObject->GetDictionary().GetKey(PdfName::KeyType);
                                if (pObjType && pObjType->IsName() && pObjType->GetName() == "XRef")
                                {
                                    // XRef is never encrypted
                                    delete pObject;
                                    pObject = new PdfParserObject(m_vecObjects->GetDocument(), device, m_buffer, (ssize_t)entry.Offset);
                                    reference = pObject->GetIndirectReference();
                                    pObject->SetLoadOnDemand(m_bLoadOnDemand);
                                    pObject->ParseFile(nullptr);
                                }
                            }
                            nLast = reference.ObjectNumber();

                            // final pdf should not contain a linerization dictionary as it contents are invalid 
                            // as we change some objects and the final xref table
                            if (m_pLinearization && nLast == static_cast<int>(m_pLinearization->GetIndirectReference().ObjectNumber()))
                            {
                                m_vecObjects->SafeAddFreeObject(reference);
                                delete pObject;
                            }
                            else
                            {
                                m_vecObjects->AddObject(pObject);
                            }
                        }
                        catch (PdfError& e)
                        {
                            std::ostringstream oss;
                            oss << "Error while loading object " << pObject->GetIndirectReference().ObjectNumber()
                                << " " << pObject->GetIndirectReference().GenerationNumber()
                                << " Offset = " << entry.Offset
                                << " Index = " << i << std::endl;
                            delete pObject;

                            if (m_bIgnoreBrokenObjects)
                            {
                                PdfError::LogMessage(LogSeverity::Error, oss.str().c_str());
                                m_vecObjects->SafeAddFreeObject(reference);
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
                        if (m_bStrictParsing)
                        {
                            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidXRef,
                                "Found object with 0 offset which should be 'f' instead of 'n'.");
                        }
                        else
                        {
                            PdfError::LogMessage(LogSeverity::Warning,
                                "Treating object %i 0 R as a free object.");
                            m_vecObjects->AddFreeObject(PdfReference(i, 1));
                        }
                    }
                    break;
                }
                case EXRefEntryType::Free:
                {
                    // NOTE: We don't need entry.ObjectNumber, which is supposed to be
                    // the entry of the next free object
                    if (i != 0)
                        m_vecObjects->SafeAddFreeObject(PdfReference(i, entry.Generation));

                    break;
                }
                case EXRefEntryType::Compressed:
                    // CHECK-ME
                    break;
                default:
                    PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);

            }
        }
        else if (i != 0) // Unparsed
        {
            m_vecObjects->AddFreeObject(PdfReference(i, 1));
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
    for (unsigned i = 0; i < m_nNumObjects; i++)
    {
        PdfXRefEntry& entry = m_entries[i];
        if (entry.Parsed && entry.Type == EXRefEntryType::Compressed) // we have an compressed object stream
        {
#ifndef VERBOSE_DEBUG_DISABLED
            if (m_bLoadOnDemand) cerr << "Demand loading on, but can't demand-load from object stream." << endl;
#endif
            ReadCompressedObjectFromStream((uint32_t)entry.ObjectNumber, static_cast<int>(entry.Index));
        }
    }

    if (!m_bLoadOnDemand)
    {
        // Force loading of streams. We can't do this during the initial
        // run that populates m_vecObjects because a stream might have a /Length
        // key that references an object we haven't yet read. So we must do it here
        // in a second pass, or (if demand loading is enabled) defer it for later.
        for (TCIVecObjects itObjects = m_vecObjects->begin();
            itObjects != m_vecObjects->end();
            ++itObjects)
        {
            pObject = dynamic_cast<PdfParserObject*>(*itObjects);
            pObject->ForceStreamParse();
        }
    }


    // Now sort the list of objects
    m_vecObjects->Sort();

    UpdateDocumentVersion();
}

void PdfParser::ReadCompressedObjectFromStream(uint32_t nObjNo, int nIndex)
{
    // NOTE: index of the compressed object is ignored since
    // we eagerly read all the objects from the stream
    (void)nIndex;

    // If we already have read all objects from this stream just return
    if (m_setObjectStreams.find(nObjNo) != m_setObjectStreams.end())
        return;

    m_setObjectStreams.insert(nObjNo);

    // generation number of object streams is always 0
    PdfParserObject* pStream = dynamic_cast<PdfParserObject*>(m_vecObjects->GetObject(PdfReference(nObjNo, 0)));
    if (pStream == nullptr)
    {
        ostringstream oss;
        oss << "Loading of object " << nObjNo << " 0 R failed!" << std::endl;

        if (m_bIgnoreBrokenObjects)
        {
            PdfError::LogMessage(LogSeverity::Error, oss.str().c_str());
            return;
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::NoObject, oss.str().c_str());
        }
    }

    PdfObjectStreamParser::ObjectIdList list;
    for (unsigned i = 0; i < m_nNumObjects; i++)
    {
        PdfXRefEntry& entry = m_entries[i];
        if (entry.Parsed && entry.Type == EXRefEntryType::Compressed &&
            m_entries[i].ObjectNumber == nObjNo)
        {
            list.push_back(static_cast<int64_t>(i));
        }
    }

    PdfObjectStreamParser pParserObject(*pStream, *m_vecObjects, m_buffer);
    pParserObject.Parse(list);
}

const char* PdfParser::GetPdfVersionString() const
{
    return s_szPdfVersions[static_cast<int>(m_ePdfVersion)];
}

void PdfParser::FindToken(const PdfRefCountedInputDevice& device, const char* pszToken, size_t lRange)
{
    // James McGill 18.02.2011, offset read position to the EOF marker if it is not the last thing in the file
    device.Device()->Seek(-(streamoff)m_lLastEOFOffset, ios_base::end);

    streamoff nFileSize = device.Device()->Tell();
    if (nFileSize == -1)
    {
        PODOFO_RAISE_ERROR_INFO(
            EPdfError::NoXRef,
            "Failed to seek to EOF when looking for xref");
    }

    size_t lXRefBuf = std::min(static_cast<size_t>(nFileSize), lRange);
    size_t nTokenLen = strlen(pszToken);

    device.Device()->Seek(-(streamoff)lXRefBuf, ios_base::cur);
    if (device.Device()->Read(m_buffer.GetBuffer(), lXRefBuf) != lXRefBuf && !device.Device()->Eof())
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);

    m_buffer.GetBuffer()[lXRefBuf] = '\0';

    ssize_t i; // Do not make this unsigned, this will cause infinte loops in files without trailer

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for (i = lXRefBuf - nTokenLen; i >= 0; i--)
    {
        if (strncmp(m_buffer.GetBuffer() + i, pszToken, nTokenLen) == 0)
        {
            break;
        }
    }

    if (!i)
    {
        PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
    }

    // James McGill 18.02.2011, offset read position to the EOF marker if it is not the last thing in the file
    device.Device()->Seek(-((streamoff)lXRefBuf - i) - m_lLastEOFOffset, ios_base::end);
}

// Peter Petrov 23 December 2008
void PdfParser::FindToken2(const PdfRefCountedInputDevice& device, const char* pszToken, const size_t lRange, size_t searchEnd)
{
    device.Device()->Seek(searchEnd, ios_base::beg);

    streamoff nFileSize = device.Device()->Tell();
    if (nFileSize == -1)
    {
        PODOFO_RAISE_ERROR_INFO(
            EPdfError::NoXRef,
            "Failed to seek to EOF when looking for xref");
    }

    size_t lXRefBuf = std::min(static_cast<size_t>(nFileSize), lRange);
    size_t nTokenLen = strlen(pszToken);

    device.Device()->Seek(-(streamoff)lXRefBuf, ios_base::cur);
    if (device.Device()->Read(m_buffer.GetBuffer(), lXRefBuf) != lXRefBuf && !device.Device()->Eof())
        PODOFO_RAISE_ERROR(EPdfError::NoXRef);

    m_buffer.GetBuffer()[lXRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    ssize_t i; // Do not use an unsigned variable here
    for (i = lXRefBuf - nTokenLen; i >= 0; i--)
    {
        if (strncmp(m_buffer.GetBuffer() + i, pszToken, nTokenLen) == 0)
            break;
    }

    if (!i)
    {
        PODOFO_RAISE_ERROR(EPdfError::InternalLogic);
    }

    device.Device()->Seek(searchEnd - ((streamoff)lXRefBuf - i), ios_base::beg);
}

const PdfString& PdfParser::GetDocumentId()
{
    if (!m_pTrailer->GetDictionary().HasKey("ID"))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidEncryptionDict, "No document ID found in trailer.");

    return m_pTrailer->GetDictionary().GetKey("ID")->GetArray()[0].GetString();
}

void PdfParser::UpdateDocumentVersion()
{
    if (m_pTrailer->IsDictionary() && m_pTrailer->GetDictionary().HasKey("Root"))
    {
        PdfObject* pCatalog = m_pTrailer->GetDictionary().GetKey("Root");
        if (pCatalog->IsReference())
        {
            pCatalog = m_vecObjects->GetObject(pCatalog->GetReference());
        }

        if (pCatalog
            && pCatalog->IsDictionary()
            && pCatalog->GetDictionary().HasKey("Version"))
        {
            auto& version = pCatalog->GetDictionary().MustGetKey("Version");
            for (int i = 0; i <= MAX_PDF_VERSION_STRING_INDEX; i++)
            {
                if (IsStrictParsing() && !version.IsName())
                {
                    // Version must be of type name, according to PDF Specification
                    PODOFO_RAISE_ERROR(EPdfError::InvalidName);
                }

                if (version.IsName() && version.GetName().GetString() == s_szPdfVersionNums[i])
                {
                    PdfError::LogMessage(LogSeverity::Information,
                        "Updating version from %s to %s",
                        s_szPdfVersionNums[static_cast<int>(m_ePdfVersion)],
                        s_szPdfVersionNums[i]);
                    m_ePdfVersion = static_cast<PdfVersion>(i);
                    break;
                }
            }
        }
    }
}

void PdfParser::ResizeOffsets(size_t nNewSize)
{
    // allow caller to specify a max object count to avoid very slow load times on large documents
    if (nNewSize > nMaxNumIndirectObjects)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "nNewSize is greater than m_nMaxObjects.");
    }

    m_entries.resize(nNewSize);
}

void PdfParser::CheckEOFMarker(const PdfRefCountedInputDevice& device)
{
    // Check for the existence of the EOF marker
    m_lLastEOFOffset = 0;
    const char* pszEOFToken = "%%EOF";
    const size_t nEOFTokenLen = 5;
    char pszBuff[nEOFTokenLen + 1];

    device.Device()->Seek(-static_cast<int>(nEOFTokenLen), ios_base::end);
    if (IsStrictParsing())
    {
        // For strict mode EOF marker must be at the very end of the file
        if (static_cast<size_t>(device.Device()->Read(pszBuff, nEOFTokenLen)) != nEOFTokenLen
            && !device.Device()->Eof())
            PODOFO_RAISE_ERROR(EPdfError::NoEOFToken);

        if (strncmp(pszBuff, pszEOFToken, nEOFTokenLen) != 0)
            PODOFO_RAISE_ERROR(EPdfError::NoEOFToken);
    }
    else
    {
        // Search for the Marker from the end of the file
        ssize_t lCurrentPos = device.Device()->Tell();

        bool bFound = false;
        while (lCurrentPos >= 0)
        {
            if (static_cast<size_t>(device.Device()->Read(pszBuff, nEOFTokenLen)) != nEOFTokenLen)
                PODOFO_RAISE_ERROR(EPdfError::NoEOFToken);

            if (strncmp(pszBuff, pszEOFToken, nEOFTokenLen) == 0)
            {
                bFound = true;
                break;
            }

            --lCurrentPos;
            device.Device()->Seek(lCurrentPos, ios_base::beg);
        }

        // Try and deal with garbage by offsetting the buffer reads in PdfParser from now on
        if (bFound)
            m_lLastEOFOffset = (m_nFileSize - (device.Device()->Tell() - 1)) + nEOFTokenLen;
        else
            PODOFO_RAISE_ERROR(EPdfError::NoEOFToken);
    }
}

const PdfObject& PdfParser::GetTrailer() const
{
    if (m_pTrailer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pTrailer;
}

bool PdfParser::IsLinearized() const
{
    return m_pLinearization != nullptr;
}

bool PdfParser::IsEncrypted() const
{
    return m_pEncrypt != nullptr;
}

std::unique_ptr<PdfEncrypt> PdfParser::TakeEncrypt()
{
    return std::move(m_pEncrypt);
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
