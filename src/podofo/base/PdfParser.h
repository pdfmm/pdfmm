/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PARSER_H
#define PDF_PARSER_H

#include <set>
#include <map>
#include "PdfDefines.h"
#include "PdfParserObject.h"
#include "PdfXRefEntry.h"
#include "PdfVecObjects.h"
#include "PdfTokenizer.h"

namespace PoDoFo {

typedef std::map<int,PdfObject*>    TMapObjects;
typedef TMapObjects::iterator       TIMapObjects;
typedef TMapObjects::const_iterator TCIMapObjects;

class PdfEncrypt;
class PdfString;
class PdfParserObject;

/**
 * PdfParser reads a PDF file into memory. 
 * The file can be modified in memory and written back using
 * the PdfWriter class.
 * Most PDF features are supported
 */
class PODOFO_API PdfParser
{
    friend class PdfDocument;
    friend class PdfWriter;

public:
    /** Create a new PdfParser object
     *  You have to open a PDF file using ParseFile later.
     *  \param pVecObjects vector to write the parsed PdfObjects to
     *
     *  \see ParseFile  
     */
    PdfParser(PdfVecObjects& pVecObjects);

    /** Delete the PdfParser and all PdfObjects
     */
    ~PdfParser();

    /** Open a PDF file and parse it.
     *
     *  \param pszFilename filename of the file which is going to be parsed
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accessed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( EPdfError::InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword() with the correct password in this case.
     *  
     *  \see SetPassword
     */
    void ParseFile(const std::string_view& filename, bool bLoadOnDemand = true );

    /** Open a PDF file and parse it.
     *
     *  \param pBuffer buffer containing a PDF file in memory
     *  \param lLen length of the buffer containing the PDF file
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accessed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( EPdfError::InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword() with the correct password in this case.
     *  
     *  \see SetPassword
     */
    void ParseBuffer(const std::string_view& filename, bool bLoadOnDemand = true );

    /** Open a PDF file and parse it.
     *
     *  \param rDevice the input device to read from
     *  \param bLoadOnDemand If true all objects will be read from the file at
     *                       the time they are accessed first.
     *                       If false all objects will be read immediately.
     *                       This is faster if you do not need the complete PDF 
     *                       file in memory.
     *
     *
     *  This might throw a PdfError( EPdfError::InvalidPassword ) exception
     *  if a password is required to read this PDF.
     *  Call SetPassword() with the correct password in this case.
     *  
     *  \see SetPassword
     */
    void Parse( const PdfRefCountedInputDevice & rDevice, bool bLoadOnDemand = true );

    /** Get the file format version of the pdf
     *  \returns the file format version as string
     */
    const char* GetPdfVersionString() const;

    /** \returns whether the parsed document contains linearization tables
     */
    bool IsLinearized() const;

    /** 
     * \returns true if this PdfWriter creates an encrypted PDF file
     */
    bool IsEncrypted() const;

    /** 
     * Gives the encryption object from the parser. The internal handle will be set
     * to nullptr and the ownership of the object is given to the caller.
     *
     * Only call this if you need access to the encryption object
     * before deleting the parser.
     *
     * \returns the parser's encryption object, or nullptr if the read PDF file was not encrypted.
     */
    std::unique_ptr<PdfEncrypt> TakeEncrypt();

    const PdfObject& GetTrailer() const;

public:
    /** If you try to open an encrypted PDF file, which requires
     *  a password to open, PoDoFo will throw a PdfError( EPdfError::InvalidPassword )
     *  exception.
     *
     *  If you got such an exception, you have to set a password
     *  which should be used for opening the PDF.
     *
     *  The usual way will be to ask the user for the password
     *  and set the password using this method.
     *
     *  PdfParser will immediately continue to read the PDF file.
     *
     *  \param password a user or owner password which can be used to open an encrypted PDF file
     *                   If the password is invalid, a PdfError( EPdfError::InvalidPassword ) exception is thrown!
     */
    inline void SetPassword(const std::string_view& password) { m_password = password; }

    /**
     * Retrieve the number of incremental updates that
     * have been applied to the last parsed PDF file.
     *
     * 0 means no update has been applied.
     *
     * \returns the number of incremental updates to the parsed PDF.
     */
    inline int GetNumberOfIncrementalUpdates() const { return m_nIncrementalUpdates; }

    /** Get a reference to the sorted internal objects vector.
     *  \returns the internal objects vector.
     */
    inline const PdfVecObjects* GetObjects() const { return m_vecObjects; }

    /** Get the file format version of the pdf
     *  \returns the file format version as enum
     */
    inline PdfVersion GetPdfVersion() const { return m_ePdfVersion; }

    /** \returns true if this PdfParser loads all objects on demand at
     *                the time they are accessed first.
     *                The default is to load all object immediately.
     *                In this case false is returned.
     */
    inline bool GetLoadOnDemand() const { return m_bLoadOnDemand; }

    /** \returns the length of the file
     */
    size_t GetFileSize() const { return m_nFileSize; }

    /**
     * \returns the parsers encryption object or nullptr if the read PDF file was not encrypted
     */
    inline const PdfEncrypt* GetEncrypt() const { return m_pEncrypt.get(); }

    /**
     * \returns true if strict parsing mode is enabled
     *
     * \see SetStringParsing
     */
    inline bool IsStrictParsing() const { return m_bStrictParsing; }

    /**
     * Enable/disable strict parsing mode.
     * Strict parsing is by default disabled.
     *
     * If you enable strict parsing, PoDoFo will fail
     * on a few more common PDF failures. Please note
     * that PoDoFo's parser is by default very strict
     * already and does not recover from e.g. wrong XREF
     * tables.
     *
     * \param bStrict new setting for strict parsing mode.
     */
    inline void SetStrictParsing( bool bStrict ) { m_bStrictParsing = bStrict; }

    /**
     * \return if broken objects are ignored while parsing
     */
    inline bool GetIgnoreBrokenObjects() const { return m_bIgnoreBrokenObjects; }

    /**
     * Specify if the parser should ignore broken
     * objects, i.e. XRef entries that do not point
     * to valid objects.
     *
     * Default is to ignore broken objects and
     * to not throw an exception if one is found.
     *
     * \param bBroken if true broken objects will be ignored
     */
    inline void SetIgnoreBrokenObjects( bool bBroken ) { m_bIgnoreBrokenObjects = bBroken; }

    inline size_t GetXRefOffset() const { return m_nXRefOffset; }

    inline bool HasXRefStream() const { return m_HasXRefStream; }

private:
    /** Searches backwards from the end of the file
     *  and tries to find a token.
     *  The current file is positioned right after the token.
     * 
     *  \param pszToken a token to find
     *  \param lRange range in bytes in which to search
     *                beginning at the end of the file
     */
    void FindToken(const PdfRefCountedInputDevice& device, const char* pszToken, size_t lRange );

    // Peter Petrov 23 December 2008
    /** Searches backwards from the specified position of the file
     *  and tries to find a token.
     *  The current file is positioned right after the token.
     * 
     *  \param pszToken a token to find
     *  \param lRange range in bytes in which to search
     *                beginning at the specified position of the file
     *  \param searchEnd specifies position 
     */
    void FindToken2(const PdfRefCountedInputDevice& device, const char* pszToken, size_t lRange, size_t searchEnd );

    /** Reads the xref sections and the trailers of the file
     *  in the correct order in the memory
     *  and takes care for linearized pdf files.
     */
    void ReadDocumentStructure(const PdfRefCountedInputDevice& rDevice);

    /** hecks whether this pdf is linearized or not.
     *  Initializes the linearization directory on success.
     */
    void HasLinearizationDict(const PdfRefCountedInputDevice& device);

    /** Merge the information of this trailer object
     *  in the parsers main trailer object.
     *  \param pTrailer take the keys to merge from this dictionary.
     */
    void MergeTrailer( const PdfObject* pTrailer );

    /** Read the trailer directory at the end of the file.
     */
    void ReadTrailer(const PdfRefCountedInputDevice& device);

    /** Looks for a startxref entry at the current file position
     *  and saves its byteoffset to pXRefOffset.
     *  \param pXRefOffset store the byte offset of the xref section into this variable.
     */
    void ReadXRef(const PdfRefCountedInputDevice& device, size_t* pXRefOffset );

    /** Reads the xref table from a pdf file.
     *  If there is no xref table, ReadXRefStreamContents() is called.
     *  \param lOffset read the table from this offset
     *  \param bPositionAtEnd if true the xref table is not read, but the 
     *                        file stream is positioned directly 
     *                        after the table, which allows reading
     *                        a following trailer dictionary.
     */
    void ReadXRefContents(const PdfRefCountedInputDevice& device, size_t lOffset, bool bPositionAtEnd = false );

    /** Read a xref subsection
     *  
     *  Throws EPdfError::NoXref if the number of objects read was not
     *  the number specified by the subsection header (as passed in
     *  `nNumObjects').
     *
     *  \param nFirstObject object number of the first object
     *  \param nNumObjects  how many objects should be read from this section
     */
    void ReadXRefSubsection(const PdfRefCountedInputDevice& device, int64_t & nFirstObject, int64_t & nNumObjects );

    /** Reads an XRef stream contents object
     *  \param lOffset read the stream from this offset
     *  \param bReadOnlyTrailer only the trailer is skipped over, the contents
     *         of the xref stream are not parsed
     */
    void ReadXRefStreamContents(const PdfRefCountedInputDevice& device, size_t lOffset, bool bReadOnlyTrailer );

    /** Reads all objects from the pdf into memory
     *  from the offsets listed in m_vecOffsets.
     *
     *  If required an encryption object is setup first.
     *
     *  The actual reading happens in ReadObjectsInternal()
     *  either if no encryption is required or a correct
     *  encryption object was initialized from SetPassword.
     */
    void ReadObjects(const PdfRefCountedInputDevice& device);

    /** Reads all objects from the pdf into memory
     *  from the offsets listed in m_vecOffsets.
     *
     *  Requires a correctly setup PdfEncrypt object
     *  with correct password.
     *
     *  This method is called from ReadObjects
     *  or SetPassword.
     *
     *  \see ReadObjects
     *  \see SetPassword
     */
    void ReadObjectsInternal(const PdfRefCountedInputDevice& device);

    /** Read the object with index nIndex from the object stream nObjNo
     *  and push it on the objects vector m_vecOffsets.
     *
     *  All objects are read from this stream and the stream object
     *  is free'd from memory. Further calls who try to read from the
     *  same stream simply do nothing.
     *
     *  \param nObjNo object number of the stream object
     *  \param nIndex index of the object which should be parsed
     *
     */
    void ReadCompressedObjectFromStream(uint32_t nObjNo, int nIndex);

    /** Checks the magic number at the start of the pdf file
     *  and sets the m_ePdfVersion member to the correct version
     *  of the pdf file.
     *
     *  \returns true if this is a pdf file, otherwise false
     */
    bool IsPdfFile(const PdfRefCountedInputDevice& device);

    void ReadNextTrailer(const PdfRefCountedInputDevice& device);


    /** Checks for the existence of the %%EOF marker at the end of the file.
     *  When strict mode is off it will also attempt to setup the parser to ignore
     *  any garbage after the last %%EOF marker.
     *  Simply raises an error if there is a problem with the marker.
     *
     */
    void CheckEOFMarker(const PdfRefCountedInputDevice& device);

    /** Initializes all private members
     *  with their initial values.
     */
    void Reset();

    /** Small helper method to retrieve the document id from the trailer
     *
     *  \returns the document id of this PDF document
     */
    const PdfString & GetDocumentId();

    /** Determines the correct version of the PDF
     *  from the document catalog (if available),
     *  as PDF > 1.4 allows updating the version.
     *
     *  If no catalog dictionary is present or no /Version
     *  key is available, the version from the file header will
     *  be used.
     */
    void UpdateDocumentVersion();


    /** Resize the internal structure m_offsets in a safe manner.
     *  The limit for the maximum number of indirect objects in a PDF file is checked by this method.
     *  The maximum is 2^23-1 (8.388.607). 
     *
     *  \param nNewSize new size of the vector
     */
    void ResizeOffsets(size_t nNewSize );
    
private:
    PdfRefCountedBuffer m_buffer;
    PdfTokenizer m_tokenizer;

    PdfVersion   m_ePdfVersion;
    bool          m_bLoadOnDemand;

    size_t        m_magicOffset;
    bool          m_HasXRefStream;
    size_t        m_nXRefOffset;
    unsigned m_nNumObjects;
    size_t        m_nXRefLinearizedOffset;
    size_t        m_nFileSize;
    size_t        m_lLastEOFOffset;

    TVecEntries   m_entries;
    PdfVecObjects* m_vecObjects;

    std::unique_ptr<PdfParserObject> m_pTrailer;
    std::unique_ptr<PdfParserObject> m_pLinearization;
    std::unique_ptr<PdfEncrypt> m_pEncrypt;

    std::string m_password;

    std::set<int> m_setObjectStreams;

    bool          m_bStrictParsing;
    bool          m_bIgnoreBrokenObjects;

    int           m_nIncrementalUpdates;
    int           m_nRecursionDepth;
    
    std::set<size_t> m_visitedXRefOffsets;
};

};

#endif // PDF_PARSER_H
