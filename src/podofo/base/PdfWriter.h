/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_WRITER_H_
#define _PDF_WRITER_H_

#include "PdfDefines.h"
#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"
#include "PdfVecObjects.h"
#include "PdfObject.h"

#include "PdfEncrypt.h"

namespace PoDoFo {

class PdfDictionary;
class PdfName;
class PdfPage;
class PdfPagesTree;
class PdfParser;
class PdfVecObjects;
class PdfXRef;

/** The PdfWriter class writes a list of PdfObjects as PDF file.
 *  The XRef section (which is the required table of contents for any
 *  PDF file) is created automatically.
 *
 *  It does not know about pages but only about PdfObjects.
 *
 *  Most users will want to use PdfDocument.
 */
class PODOFO_API PdfWriter
{
private:
    PdfWriter(PdfVecObjects* pVecObjects, const PdfObject& pTrailer, EPdfVersion version);

public:
    /** Create a new pdf file, from an vector of PdfObjects
     *  and a trailer object.
     *  \param pVecObjects the vector of objects
     *  \param pTrailer a valid trailer object
     */
    PdfWriter(PdfVecObjects& pVecObjects, const PdfObject& pTrailer);

    virtual ~PdfWriter();

    /** Internal implementation of the Write() call with the common code
     *  \param pDevice write to this output device
     *  \param bRewriteXRefTable whether will rewrite whole XRef table (used only if GetIncrementalUpdate() returns true)
     */
    void Write(PdfOutputDevice& device);

    /** Create a XRef stream which is in some case
     *  more compact but requires at least PDF 1.5
     *  Default is false.
     *  \param bStream if true a XRef stream object will be created
     */
    void SetUseXRefStream( bool bStream );
    
    /** Set the written document to be encrypted using a PdfEncrypt object
     *
     *  \param rEncrypt an encryption object which is used to encrypt the written PDF file
     */
    void SetEncrypted( const PdfEncrypt & rEncrypt );


    /** Add required keys to a trailer object
     *  \param pTrailer add keys to this object
     *  \param lSize number of objects in the PDF file
     *  \param bOnlySizeKey write only the size key
     */
    void FillTrailerObject( PdfObject& pTrailer, size_t lSize, bool bOnlySizeKey ) const;

public:
    /** Get the file format version of the pdf
     *  \returns the file format version as string
     */
    const char* GetPdfVersionString() const;

    inline void SetSaveOptions(PdfSaveOptions saveOptions) { m_saveOptions = saveOptions; }

    /** Set the write mode to use when writing the PDF.
     *  \param eWriteMode write mode
     */
    inline void SetWriteMode(EPdfWriteMode eWriteMode) { m_eWriteMode = eWriteMode; }

    /** Get the write mode used for wirting the PDF
     *  \returns the write mode
     */
    inline EPdfWriteMode GetWriteMode() const { return m_eWriteMode; }

    /** Set the PDF Version of the document. Has to be called before Write() to
     *  have an effect.
     *  \param eVersion  version of the pdf document
     */
    inline void SetPdfVersion(EPdfVersion eVersion) { m_eVersion = eVersion; }

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    inline EPdfVersion GetPdfVersion() const { return m_eVersion; }

    /**
     *  \returns wether a XRef stream is used or not
     */
    inline bool GetUseXRefStream() const { return m_UseXRefStream; }

    /** Sets an offset to the previous XRef table. Set it to lower than
     *  or equal to 0, to not write a reference to the previous XRef table.
     *  The default is 0.
     *  \param lPrevXRefOffset the previous XRef table offset
     */
    inline void SetPrevXRefOffset(int64_t lPrevXRefOffset) { m_lPrevXRefOffset = lPrevXRefOffset; }

    /**
     *  \returns offset to the previous XRef table, as previously set
     *     by SetPrevXRefOffset.
     *
     * \see SetPrevXRefOffset
     */
    inline int64_t GetPrevXRefOffset() const { return m_lPrevXRefOffset; }

    /** Set whether writing an incremental update.
     *  Default is false.
     *  \param bIncrementalUpdate if true an incremental update will be written
     */
    void SetIncrementalUpdate(bool rewriteXRefTable);

    /**
     *  \returns whether writing an incremental update
     */
    inline bool GetIncrementalUpdate() const { return m_bIncrementalUpdate; }

    /**
     * \returns true if this PdfWriter creates an encrypted PDF file
     */
    inline bool GetEncrypted() const { return m_pEncrypt != NULL; }

protected:
    /**
     * Create a PdfWriter from a PdfVecObjects
     */
    PdfWriter( PdfVecObjects& pVecObjects );

    /** Writes the pdf header to the current file.
     *  \param pDevice write to this output device
     */       
    void WritePdfHeader(PdfOutputDevice& device);

    /** Write pdf objects to file
     *  \param pDevice write to this output device
     *  \param vecObjects write all objects in this vector to the file
     *  \param pXref add all written objects to this XRefTable
     *  \param bRewriteXRefTable whether will rewrite whole XRef table (used only if GetIncrementalUpdate() returns true)
     */ 
    void WritePdfObjects(PdfOutputDevice& device, const PdfVecObjects& vecObjects, PdfXRef& xref);

    /** Creates a file identifier which is required in several
     *  PDF workflows. 
     *  All values from the files document information dictionary are
     *  used to create a unique MD5 key which is added to the trailer dictionary.
     *
     *  \param identifier write the identifier to this string
     *  \param pTrailer trailer object
     *  \param pOriginalIdentifier write the original identifier (when using incremental update) to this string
     */
    void CreateFileIdentifier( PdfString& identifier, const PdfObject& pTrailer, PdfString* pOriginalIdentifier = nullptr ) const;

    
    const PdfObject& GetTrailer() { return m_Trailer; }
    PdfVecObjects& GetObjects() { return *m_vecObjects; }
    PdfEncrypt* GetEncrypt() { return m_pEncrypt.get(); }
    PdfObject* GetEncryptObj() { return m_pEncryptObj; }
    const PdfString & GetIdentifier() { return m_identifier; }
    void SetIdentifier(const PdfString &identifier) { m_identifier = identifier; }
    void SetEncryptObj(PdfObject* obj);
private:
    PdfVecObjects*  m_vecObjects;
    PdfObject m_Trailer;
    EPdfVersion     m_eVersion;

    bool            m_UseXRefStream;

    std::unique_ptr<PdfEncrypt> m_pEncrypt;    ///< If not NULL encrypt all strings and streams and create an encryption dictionary in the trailer
    PdfObject* m_pEncryptObj; ///< Used to temporarly store the encryption dictionary

    PdfSaveOptions  m_saveOptions;
    EPdfWriteMode   m_eWriteMode;

    PdfString       m_identifier;
    PdfString       m_originalIdentifier; // used for incremental update
    int64_t         m_lPrevXRefOffset;
    bool            m_bIncrementalUpdate;
    bool            m_rewriteXRefTable; // Only used if incremental update
 
    /**
     * This value is required when writing
     * a linearized PDF file.
     * It represents the offset of the whitespace
     * character before the first line in the XRef
     * section.
     */
    size_t            m_lFirstInXRef;
    size_t            m_lLinearizedOffset;
    size_t            m_lLinearizedLastOffset;
    size_t            m_lTrailerOffset;
};

};

#endif // _PDF_WRITER_H_
