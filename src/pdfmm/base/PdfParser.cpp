/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfParser.h"

#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfMemoryObjectStream.h"
#include "PdfObjectStreamParser.h"
#include "PdfOutputDevice.h"
#include "PdfObjectStream.h"
#include "PdfVariant.h"
#include "PdfXRefStreamParserObject.h"

#include <algorithm>

constexpr unsigned PDF_VERSION_LENGHT = 3;
constexpr unsigned PDF_MAGIC_LENGHT = 8;
constexpr unsigned PDF_XREF_ENTRY_SIZE = 20;
constexpr unsigned PDF_XREF_BUF = 512;
constexpr unsigned MAX_XREF_SESSION_COUNT = 512;

using namespace std;
using namespace mm;

static bool CheckEOL(char e1, char e2);
static bool CheckXRefEntryType(char c);
static bool ReadMagicWord(char ch, unsigned& cursoridx);

static unsigned s_MaxObjectCount = (1U << 23) - 1;

class PdfRecursionGuard
{
  // RAII recursion guard ensures m_nRecursionDepth is always decremented
  // because the destructor is always called when control leaves a method
  // via return or an exception.
  // see http://en.cppreference.com/w/cpp/language/raii

  // It's used like this in PdfParser methods
  // PdfRecursionGuard guard(m_RecursionDepth);

public:
    PdfRecursionGuard(unsigned& recursionDepth)
        : m_RecursionDepth(recursionDepth)
    {
        // be careful changing this limit - overflow limits depend on the OS, linker settings, and how much stack space compiler allocates
        // 500 limit prevents overflow on Win7 with VC++ 2005 with default linker stack size (1000 caused overflow with same compiler/OS)
        constexpr unsigned MaxRecursionDepth = 500;

        m_RecursionDepth++;

        if (m_RecursionDepth > MaxRecursionDepth)
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

PdfParser::PdfParser(PdfIndirectObjectList& objects) :
    m_buffer(std::make_shared<charbuff>(PdfTokenizer::BufferSize)),
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
    m_XRefLinearizedOffset = 0;
    m_LastEOFOffset = 0;

    m_Trailer = nullptr;
    m_entries.Clear();
    m_ObjectStreams.clear();

    m_Encrypt = nullptr;

    m_IgnoreBrokenObjects = true;
    m_IncrementalUpdateCount = 0;
    m_RecursionDepth = 0;
}

void PdfParser::Parse(PdfInputDevice& device, bool loadOnDemand)
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
        PDFMM_PUSH_FRAME_INFO(e, "Unable to load objects from file");
        throw e;
    }
}

void PdfParser::ReadDocumentStructure(PdfInputDevice& device)
{
    // position at the end of the file to search the xref table.
    device.Seek(0, ios_base::end);
    m_FileSize = device.Tell();

    // Validate the eof marker and when not in strict mode accept garbage after it
    try
    {
        CheckEOFMarker(device);
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME_INFO(e, "EOF marker could not be found");
        throw e;
    }

    try
    {
        FindXRef(device, &m_XRefOffset);
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME_INFO(e, "Unable to find startxref entry in file");
        throw e;
    }

    try
    {
        // We begin read the first XRef content, without
        // trying to read first the trailer alone as done
        // previously. This is caused by the fact that
        // the trailer of the last incremental update
        // can't be find along the way close to the "startxref"
        // line in case of linearized PDFs. See ISO 32000-1:2008
        // "F.3.11 Main Cross-Reference and Trailer"
        // https://stackoverflow.com/a/70564329/213871
        ReadXRefContents(device, m_XRefOffset);
    }
    catch (PdfError& e)
    {
        PDFMM_PUSH_FRAME_INFO(e, "Unable to load xref entries");
        throw e;
    }

    int64_t entriesCount;
    if (m_Trailer != nullptr && m_Trailer->IsDictionary()
        && (entriesCount = m_Trailer->GetDictionary().FindKeyAs<int64_t>(PdfName::KeySize, -1)) >= 0
        && m_entries.GetSize() > (unsigned)entriesCount)
    {
        // Total number of xref entries to read is greater than the /Size
        // specified in the trailer if any. That's an error unless we're
        // trying to recover from a missing /Size entry.
        PdfError::LogMessage(PdfLogSeverity::Warning,
            "There are more objects {} in this XRef "
            "table than specified in the size key of the trailer directory ({})!",
            m_entries.GetSize(), entriesCount);
    }
}

bool PdfParser::IsPdfFile(PdfInputDevice& device)
{
    unsigned i = 0;
    device.Seek(0, ios_base::beg);
    while (true)
    {
        char ch;
        if (!device.TryGetChar(ch))
            return false;

        if (ReadMagicWord(ch, i))
            break;
    }

    char version[PDF_VERSION_LENGHT];
    if (device.Read(version, PDF_VERSION_LENGHT) != PDF_VERSION_LENGHT)
        return false;

    m_magicOffset = device.Tell() - PDF_MAGIC_LENGHT;
    // try to determine the excact PDF version of the file
    for (i = 0; i <= MAX_PDF_VERSION_STRING_INDEX; i++)
    {
        if (std::strncmp(version, s_PdfVersionNums[i], PDF_VERSION_LENGHT) == 0)
        {
            m_PdfVersion = static_cast<PdfVersion>(i);
            return true;
        }
    }

    return false;
}

void PdfParser::MergeTrailer(const PdfObject& trailer)
{
    PDFMM_ASSERT(m_Trailer != nullptr);

    // Only update keys, if not already present
    auto obj = trailer.GetDictionary().GetKey(PdfName::KeySize);
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey(PdfName::KeySize))
        m_Trailer->GetDictionary().AddKey(PdfName::KeySize, *obj);

    obj = trailer.GetDictionary().GetKey("Root");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Root"))
        m_Trailer->GetDictionary().AddKey("Root", *obj);

    obj = trailer.GetDictionary().GetKey("Encrypt");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Encrypt"))
        m_Trailer->GetDictionary().AddKey("Encrypt", *obj);

    obj = trailer.GetDictionary().GetKey("Info");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("Info"))
        m_Trailer->GetDictionary().AddKey("Info", *obj);

    obj = trailer.GetDictionary().GetKey("ID");
    if (obj != nullptr && !m_Trailer->GetDictionary().HasKey("ID"))
        m_Trailer->GetDictionary().AddKey("ID", *obj);
}

void PdfParser::ReadNextTrailer(PdfInputDevice& device)
{
    PdfRecursionGuard guard(m_RecursionDepth);
    if (m_tokenizer.IsNextToken(device, "trailer"))
    {
        auto trailer = new PdfParserObject(m_Objects->GetDocument(), device);

        try
        {
            // Ignore the encryption in the trailer as the trailer may not be encrypted
            trailer->ParseFile(nullptr, true);
        }
        catch (PdfError& e)
        {
            PDFMM_PUSH_FRAME_INFO(e, "The linearized trailer was found in the file, but contains errors");
            delete trailer;
            throw e;
        }

        unique_ptr<PdfParserObject> trailerTemp;
        if (m_Trailer == nullptr)
        {
            m_Trailer.reset(trailer);
        }
        else
        {
            trailerTemp.reset(trailer);
            // now merge the information of this trailer with the main documents trailer
            MergeTrailer(*trailer);
        }

        if (trailer->GetDictionary().HasKey("XRefStm"))
        {
            // Whenever we read a XRefStm key, 
            // we know that the file was updated.
            if (!trailer->GetDictionary().HasKey("Prev"))
                m_IncrementalUpdateCount++;

            try
            {
                ReadXRefStreamContents(device, static_cast<size_t>(trailer->GetDictionary().FindKeyAs<int64_t>("XRefStm", 0)), false);
            }
            catch (PdfError& e)
            {
                PDFMM_PUSH_FRAME_INFO(e, "Unable to load /XRefStm xref stream");
                throw e;
            }
        }

        if (trailer->GetDictionary().HasKey("Prev"))
        {
            // Whenever we read a Prev key, 
            // we know that the file was updated.
            m_IncrementalUpdateCount++;

            try
            {
                size_t offset = static_cast<size_t>(trailer->GetDictionary().FindKeyAs<int64_t>("Prev", 0));

                if (m_visitedXRefOffsets.find(offset) == m_visitedXRefOffsets.end())
                    ReadXRefContents(device, offset);
                else
                    PdfError::LogMessage(PdfLogSeverity::Warning, "XRef contents at offset {} requested twice, skipping the second read",
                        static_cast<int64_t>(offset));
            }
            catch (PdfError& e)
            {
                PDFMM_PUSH_FRAME_INFO(e, "Unable to load /Prev xref entries");
                throw e;
            }
        }
    }
    else // Else belongs to IsNextToken("trailer") and not to HasKey("Prev")
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::NoTrailer);
    }
}

void PdfParser::FindXRef(PdfInputDevice& device, size_t* xRefOffset)
{
    // ISO32000-1:2008, 7.5.5 File Trailer "Conforming readers should read a PDF file from its end"
    FindTokenBackward(device, "startxref", PDF_XREF_BUF);
    if (!m_tokenizer.IsNextToken(device, "startxref"))
    {
        // Could be non-standard startref
        if (!m_StrictParsing)
        {
            FindTokenBackward(device, "startref", PDF_XREF_BUF);
            if (!m_tokenizer.IsNextToken(device, "startref"))
                PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
        else
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
        }
    }

    // Support also files with whitespace offset before magic start
    *xRefOffset = (size_t)m_tokenizer.ReadNextNumber(device) + m_magicOffset;
}

void PdfParser::ReadXRefContents(PdfInputDevice& device, size_t offset, bool positionAtEnd)
{
    PdfRecursionGuard guard(m_RecursionDepth);

    int64_t firstObject = 0;
    int64_t objectCount = 0;

    if (m_visitedXRefOffsets.find(offset) != m_visitedXRefOffsets.end())
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
            "Cycle in xref structure. Offset {} already visited", offset);
    }
    else
    {
        m_visitedXRefOffsets.insert(offset);
    }

    size_t currPosition = device.Tell();
    device.Seek(0, ios_base::end);
    size_t fileSize = device.Tell();
    device.Seek(currPosition, ios_base::beg);

    if (offset > fileSize)
    {
        // Invalid "startxref"
         // ignore returned value and get offset from the device
        FindXRef(device, &offset);
        offset = device.Tell();
        // TODO: hard coded value "4"
        m_buffer->resize(PDF_XREF_BUF * 4);
        FindToken2(device, "xref", PDF_XREF_BUF * 4, offset);
        m_buffer->resize(PDF_XREF_BUF);
        offset = device.Tell();
        m_XRefOffset = offset;
    }
    else
    {
        device.Seek(offset);
    }

    string_view token;
    if (!m_tokenizer.TryReadNextToken(device, token))
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    if (token != "xref")
    {
        // Found linearized 1.3-pdf's with trailer-info in xref-stream
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
    for (unsigned xrefSectionCount = 0; ; xrefSectionCount++)
    {
        if (xrefSectionCount == MAX_XREF_SESSION_COUNT)
            PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);

        try
        {
            // something like PeekNextToken()
            PdfTokenType tokenType;
            string_view readToken;
            bool gotToken = m_tokenizer.TryReadNextToken(device, readToken, tokenType);
            if (gotToken)
            {
                m_tokenizer.EnqueueToken(readToken, tokenType);
                if (readToken == "trailer")
                    break;
            }

            firstObject = m_tokenizer.ReadNextNumber(device);
            objectCount = m_tokenizer.ReadNextNumber(device);

#ifdef PDFMM_VERBOSE_DEBUG
            PdfError::LogMessage(PdfLogSeverity::Debug, "Reading numbers: {} {}", firstObject, objectCount);
#endif // PDFMM_VERBOSE_DEBUG

            if (positionAtEnd)
                device.Seek(static_cast<streamoff>(objectCount * PDF_XREF_ENTRY_SIZE), ios_base::cur);
            else
                ReadXRefSubsection(device, firstObject, objectCount);
        }
        catch (PdfError& e)
        {
            if (e == PdfErrorCode::NoNumber || e == PdfErrorCode::InvalidXRef || e == PdfErrorCode::UnexpectedEOF)
            {
                break;
            }
            else
            {
                PDFMM_PUSH_FRAME(e);
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
            PDFMM_PUSH_FRAME(e);
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

void PdfParser::ReadXRefSubsection(PdfInputDevice& device, int64_t& firstObject, int64_t& objectCount)
{
#ifdef PDFMM_VERBOSE_DEBUG
    PdfError::LogMessage(PdfLogSeverity::Debug, "Reading XRef Section: {} {} Objects", firstObject, objectCount);
#endif // PDFMM_VERBOSE_DEBUG 

    if (firstObject < 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: first object is negative");
    if (objectCount < 0)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "ReadXRefSubsection: object count is negative");

    m_entries.Enlarge((uint64_t)(firstObject + objectCount));

    // consume all whitespaces
    int charcode;
    while (m_tokenizer.IsWhitespace((charcode = device.Look())))
        (void)device.GetChar();

    unsigned index = 0;
    char* buffer = m_buffer->data();
    while (index < objectCount && device.Read(buffer, PDF_XREF_ENTRY_SIZE) == PDF_XREF_ENTRY_SIZE)
    {
        char empty1;
        char empty2;
        buffer[PDF_XREF_ENTRY_SIZE] = '\0';

        unsigned objIndex = static_cast<unsigned>(firstObject + index);

        auto& entry = m_entries[objIndex];
        if (objIndex < m_entries.GetSize() && !entry.Parsed)
        {
            uint64_t variant = 0;
            uint32_t generation = 0;
            char chType = 0;

            // XRefEntry is defined in PDF spec section 7.5.4 Cross-Reference Table as
            // nnnnnnnnnn ggggg n eol
            // nnnnnnnnnn is 10-digit offset number with max value 9999999999 (bigger than 2**32 = 4GB)
            // ggggg is a 5-digit generation number with max value 99999 (smaller than 2**17)
            // eol is a 2-character end-of-line sequence
            int read = sscanf(buffer, "%10" SCNu64 " %5" SCNu32 " %c%c%c",
                &variant, &generation, &chType, &empty1, &empty2);

            if (!CheckXRefEntryType(chType))
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef, "Invalid used keyword, must be eiter 'n' or 'f'");

            XRefEntryType type = XRefEntryTypeFromChar(chType);

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

        index++;
    }

    if (index != (unsigned)objectCount)
    {
        PdfError::LogMessage(PdfLogSeverity::Warning, "Count of readobject is {}. Expected {}", index, objectCount);
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);
    }
}

void PdfParser::ReadXRefStreamContents(PdfInputDevice& device, size_t offset, bool readOnlyTrailer)
{
    PdfRecursionGuard guard(m_RecursionDepth);

    device.Seek(offset);
    auto xrefObjTrailer = new PdfXRefStreamParserObject(m_Objects->GetDocument(), device, m_entries);
    try
    {
        xrefObjTrailer->Parse();
    }
    catch (PdfError& ex)
    {
        PDFMM_PUSH_FRAME_INFO(ex, "The linearized trailer was found in the file, but contains errors");
        delete xrefObjTrailer;
        throw ex;
    }

    unique_ptr<PdfXRefStreamParserObject> xrefObjectTemp;
    if (m_Trailer == nullptr)
    {
        m_Trailer.reset(xrefObjTrailer);
    }
    else
    {
        xrefObjectTemp.reset(xrefObjTrailer);
        MergeTrailer(*xrefObjTrailer);
    }

    if (readOnlyTrailer)
        return;

    xrefObjTrailer->ReadXRefTable();

    // Check for a previous XRefStm or xref table
    size_t previousOffset;
    if (xrefObjTrailer->TryGetPreviousOffset(previousOffset) && previousOffset != offset)
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
                PDFMM_PUSH_FRAME(e);
                throw e;
            }
        }
    }
}

void PdfParser::ReadObjects(PdfInputDevice& device)
{
    // Check for encryption and make sure that the encryption object
    // is loaded before all other objects
    PdfObject* encrypt = m_Trailer->GetDictionary().GetKey("Encrypt");
    if (encrypt != nullptr && !encrypt->IsNull())
    {
#ifdef PDFMM_VERBOSE_DEBUG
        PdfError::LogMessage(PdfLogSeverity::Debug, "The PDF file is encrypted");
#endif // PDFMM_VERBOSE_DEBUG

        if (encrypt->IsReference())
        {
            unsigned i = encrypt->GetReference().ObjectNumber();
            if (i <= 0 || i >= m_entries.GetSize())
            {
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                    "Encryption dictionary references a nonexistent object {} {} R",
                    encrypt->GetReference().ObjectNumber(), encrypt->GetReference().GenerationNumber());
            }

            unique_ptr<PdfParserObject> obj(new PdfParserObject(m_Objects->GetDocument(), device, (ssize_t)m_entries[i].Offset));
            obj->SetLoadOnDemand(false); // Never load this on demand, as we will use it immediately
            try
            {
                obj->ParseFile(nullptr); // The encryption dictionary is not encrypted
                // Never add the encryption dictionary to m_Objects
                // we create a new one, if we need it for writing
                // m_Objects->push_back( obj );
                m_entries[i].Parsed = false;
                m_Encrypt = PdfEncrypt::CreatePdfEncrypt(*obj);
            }
            catch (PdfError& e)
            {
                PDFMM_PUSH_FRAME_INFO(e, "Error while loading object {} {} R",
                    obj->GetIndirectReference().ObjectNumber(),
                    obj->GetIndirectReference().GenerationNumber());
                throw e;

            }
        }
        else if (encrypt->IsDictionary())
        {
            m_Encrypt = PdfEncrypt::CreatePdfEncrypt(*encrypt);
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict,
                "The encryption entry in the trailer is neither an object nor a reference");
        }

        // Generate encryption keys
        bool isAuthenticated = m_Encrypt->Authenticate(m_password, this->GetDocumentId());
        if (!isAuthenticated)
        {
            // authentication failed so we need a password from the user.
            // The user can set the password using PdfParser::SetPassword
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidPassword, "A password is required to read this PDF file");
        }
    }

    ReadObjectsInternal(device);
}

void PdfParser::ReadObjectsInternal(PdfInputDevice& device)
{
    // Read objects
    for (unsigned i = 0; i < m_entries.GetSize(); i++)
    {
        PdfXRefEntry& entry = m_entries[i];
#ifdef PDFMM_VERBOSE_DEBUG
        cerr << "ReadObjectsInteral\t" << i << " "
            << (entry.Parsed ? "parsed" : "unparsed") << " "
            << entry.Offset << " "
            << entry.Generation << endl;
#endif
        if (entry.Parsed)
        {
            switch (entry.Type)
            {
                case XRefEntryType::InUse:
                {
                    if (entry.Offset > 0)
                    {
                        unique_ptr<PdfParserObject> obj(new PdfParserObject(m_Objects->GetDocument(), device, (ssize_t)entry.Offset));
                        obj->SetLoadOnDemand(m_LoadOnDemand);
                        PdfReference reference(i, (uint16_t)entry.Generation);
                        try
                        {
                            obj->ParseFile(m_Encrypt.get());
                            if (obj->GetIndirectReference() != reference)
                            {
                                if (m_StrictParsing)
                                {
                                    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
                                        "Found object with reference {} different than reported {} in XRef sections",
                                        obj->GetIndirectReference().ToString(), reference.ToString());
                                }
                                else
                                {
                                    PdfError::LogMessage(PdfLogSeverity::Warning,
                                        "Found object with reference {} different than reported {} in XRef sections",
                                        obj->GetIndirectReference().ToString(), reference.ToString());
                                }
                            }

                            if (m_Encrypt != nullptr && obj->IsDictionary())
                            {
                                auto typeObj = obj->GetDictionary().GetKey(PdfName::KeyType);
                                if (typeObj != nullptr && typeObj->IsName() && typeObj->GetName() == "XRef")
                                {
                                    // XRef is never encrypted
                                    obj.reset(new PdfParserObject(m_Objects->GetDocument(), device, (ssize_t)entry.Offset));
                                    obj->SetLoadOnDemand(m_LoadOnDemand);
                                    obj->ParseFile(nullptr);
                                }
                            }

                            m_Objects->PushObject(reference, obj.release());
                        }
                        catch (PdfError& e)
                        {
                            if (m_IgnoreBrokenObjects)
                            {
                                PdfError::LogMessage(PdfLogSeverity::Error, "Error while loading object {} {} R, Offset={}, Index={}",
                                    obj->GetIndirectReference().ObjectNumber(),
                                    obj->GetIndirectReference().GenerationNumber(),
                                    entry.Offset, i);
                                m_Objects->SafeAddFreeObject(reference);
                            }
                            else
                            {
                                PDFMM_PUSH_FRAME_INFO(e, "Error while loading object {} {} R, Offset={}, Index={}",
                                    obj->GetIndirectReference().ObjectNumber(),
                                    obj->GetIndirectReference().GenerationNumber(),
                                    entry.Offset, i);
                                throw e;
                            }
                        }
                    }
                    else if (entry.Generation == 0)
                    {
                        PDFMM_ASSERT(entry.Offset == 0);
                        // There are broken PDFs which add objects with 'n' 
                        // and 0 offset and 0 generation number
                        // to the xref table instead of using free objects
                        // treating them as free objects
                        if (m_StrictParsing)
                        {
                            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidXRef,
                                "Found object with 0 offset which should be 'f' instead of 'n'");
                        }
                        else
                        {
                            PdfError::LogMessage(PdfLogSeverity::Warning,
                                "Treating object {} 0 R as a free object", i);
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
        // Because PdfIndirectObjectList relies on a unbroken range, fill the free list more
        // robustly from all places which are either free or unparsed
    }

    // all normal objects including object streams are available now,
    // we can parse the object streams safely now.
    //
    // Note that even if demand loading is enabled we still currently read all
    // objects from the stream into memory then free the stream.
    //
    for (unsigned i = 0; i < m_entries.GetSize(); i++)
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
            auto obj = dynamic_cast<PdfParserObject*>(objToLoad);
            obj->ForceStreamParse();
        }
    }

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
        if (m_IgnoreBrokenObjects)
        {
            PdfError::LogMessage(PdfLogSeverity::Error, "Loading of object {} 0 R failed!", objNo);
            return;
        }
        else
        {
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Loading of object {} 0 R failed!", objNo);
        }
    }

    PdfObjectStreamParser::ObjectIdList list;
    for (unsigned i = 0; i < m_entries.GetSize(); i++)
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

void PdfParser::FindTokenBackward(PdfInputDevice& device, const char* token, size_t range)
{
    // Offset read position to the EOF marker if it is not the last thing in the file
    device.Seek(-(streamoff)m_LastEOFOffset, ios_base::end);

    char* buffer = m_buffer->data();
    streamoff fileSize = device.Tell();
    if (fileSize == -1)
    {
        PDFMM_RAISE_ERROR_INFO(
            PdfErrorCode::NoXRef,
            "Failed to seek to EOF when looking for xref");
    }

    size_t xRefBuf = std::min(static_cast<size_t>(fileSize), range);
    size_t tokenLen = strlen(token);

    device.Seek(-(streamoff)xRefBuf, ios_base::cur);
    if (device.Read(buffer, xRefBuf) != xRefBuf && !device.Eof())
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    buffer[xRefBuf] = '\0';

    ssize_t i; // Do not make this unsigned, this will cause infinte loops in files without trailer

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    for (i = xRefBuf - tokenLen; i >= 0; i--)
    {
        if (std::strncmp(buffer + i, token, tokenLen) == 0)
            break;
    }

    if (i == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);

    // Ooffset read position to the EOF marker if it is not the last thing in the file
    device.Seek(-((streamoff)xRefBuf - i) - m_LastEOFOffset, ios_base::end);
}

// CHECK-ME: This function is fishy or bad named
void PdfParser::FindToken2(PdfInputDevice& device, const char* token, const size_t range, size_t searchEnd)
{
    device.Seek(searchEnd, ios_base::beg);

    streamoff fileSize = device.Tell();
    if (fileSize == -1)
    {
        PDFMM_RAISE_ERROR_INFO(
            PdfErrorCode::NoXRef,
            "Failed to seek to EOF when looking for xref");
    }

    size_t xRefBuf = std::min(static_cast<size_t>(fileSize), range);
    size_t tokenLen = strlen(token);
    char* buffer = m_buffer->data();

    device.Seek(-(streamoff)xRefBuf, ios_base::cur);
    if (device.Read(buffer, xRefBuf) != xRefBuf && !device.Eof())
        PDFMM_RAISE_ERROR(PdfErrorCode::NoXRef);

    buffer[xRefBuf] = '\0';

    // search backwards in the buffer in case the buffer contains null bytes
    // because it is right after a stream (can't use strstr for this reason)
    ssize_t i; // Do not use an unsigned variable here
    for (i = xRefBuf - tokenLen; i >= 0; i--)
    {
        if (std::strncmp(buffer + i, token, tokenLen) == 0)
            break;
    }

    if (i == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InternalLogic);

    device.Seek(searchEnd - ((streamoff)xRefBuf - i), ios_base::beg);
}

const PdfString& PdfParser::GetDocumentId()
{
    if (!m_Trailer->GetDictionary().HasKey("ID"))
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidEncryptionDict, "No document ID found in trailer");

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

                if (version.IsName()
                    && version.GetName().GetString() == s_PdfVersionNums[i]
                    && m_PdfVersion != static_cast<PdfVersion>(i))
                {
                    PdfError::LogMessage(PdfLogSeverity::Information,
                        "Updating version from {} to {}",
                        s_PdfVersionNums[static_cast<int>(m_PdfVersion)],
                        s_PdfVersionNums[i]);
                    m_PdfVersion = static_cast<PdfVersion>(i);
                    break;
                }
            }
        }
    }
}

void PdfParser::CheckEOFMarker(PdfInputDevice& device)
{
    // Check for the existence of the EOF marker
    m_LastEOFOffset = 0;
    const char* EOFToken = "%%EOF";
    constexpr size_t EOFTokenLen = 5;
    char buff[EOFTokenLen + 1];

    device.Seek(-static_cast<int>(EOFTokenLen), ios_base::end);
    if (IsStrictParsing())
    {
        // For strict mode EOF marker must be at the very end of the file
        if (static_cast<size_t>(device.Read(buff, EOFTokenLen)) != EOFTokenLen
            && !device.Eof())
            PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);

        if (std::strncmp(buff, EOFToken, EOFTokenLen) != 0)
            PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);
    }
    else
    {
        // Search for the Marker from the end of the file
        ssize_t currentPos = device.Tell();

        bool found = false;
        while (currentPos >= 0)
        {
            if (static_cast<size_t>(device.Read(buff, EOFTokenLen)) != EOFTokenLen)
                PDFMM_RAISE_ERROR(PdfErrorCode::NoEOFToken);

            if (std::strncmp(buff, EOFToken, EOFTokenLen) == 0)
            {
                found = true;
                break;
            }

            currentPos--;
            device.Seek(currentPos, ios_base::beg);
        }

        // Try and deal with garbage by offsetting the buffer reads in PdfParser from now on
        if (found)
            m_LastEOFOffset = (m_FileSize - (device.Tell() - 1)) + EOFTokenLen;
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

bool PdfParser::IsEncrypted() const
{
    return m_Encrypt != nullptr;
}

unique_ptr<PdfEncrypt> PdfParser::TakeEncrypt()
{
    return std::move(m_Encrypt);
}

unsigned PdfParser::GetMaxObjectCount()
{
    return s_MaxObjectCount;
}

void PdfParser::SetMaxObjectCount(unsigned maxObjectCount)
{
    s_MaxObjectCount = maxObjectCount;
}

// Read magic word keeping cursor
bool ReadMagicWord(char ch, unsigned& cursoridx)
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
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InternalLogic, "Unexpected flow");
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
