/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#include <algorithm>
#include <deque>
#include <iostream>

#include "PdfMemDocument.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfObject.h"
#include "base/PdfParserObject.h"
#include "base/PdfStream.h"
#include "base/PdfVecObjects.h"

#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNamesTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"

using namespace std;

using namespace PoDoFo;

PdfMemDocument::PdfMemDocument()
    : PdfDocument(), m_pEncrypt( NULL ), m_pParser( NULL ), m_bSoureHasXRefStream( false ), m_lPrevXRefOffset( -1 )
{
    m_eVersion    = PdfVersionDefault;
    m_eWriteMode  = PdfWriteModeDefault;
    m_bLinearized = false;
    m_eSourceVersion = m_eVersion;
}

PdfMemDocument::PdfMemDocument(bool bOnlyTrailer)
    : PdfDocument(bOnlyTrailer), m_pEncrypt( NULL ), m_pParser( NULL ), m_bSoureHasXRefStream( false ), m_lPrevXRefOffset( -1 )
{
    m_eVersion    = PdfVersionDefault;
    m_eWriteMode  = PdfWriteModeDefault;
    m_bLinearized = false;
    m_eSourceVersion = m_eVersion;
}

PdfMemDocument::PdfMemDocument(const string_view& filename)
    : PdfDocument(), m_pEncrypt( NULL ), m_pParser( NULL ), m_bSoureHasXRefStream( false ), m_lPrevXRefOffset( -1 )
{
    this->Load(filename);
}

PdfMemDocument::~PdfMemDocument()
{
    this->Clear();
}

void PdfMemDocument::Clear() 
{
    if( m_pEncrypt ) 
    {
        delete m_pEncrypt;
        m_pEncrypt = NULL;
    }

    if( m_pParser ) 
    {
        delete m_pParser;
        m_pParser = NULL;
    }

    m_eWriteMode  = PdfWriteModeDefault;

    m_bSoureHasXRefStream = false;
    m_lPrevXRefOffset = -1;

    GetObjects().SetCanReuseObjectNumbers( true );

    PdfDocument::Clear();
}

void PdfMemDocument::InitFromParser( PdfParser* pParser )
{
    m_eVersion     = pParser->GetPdfVersion();
    m_bLinearized  = pParser->IsLinearized();
    m_eSourceVersion = m_eVersion;
    m_bSoureHasXRefStream = pParser->HasXRefStream();
    m_lPrevXRefOffset = pParser->GetXRefOffset();

    PdfObject* pTrailer = new PdfObject( *(pParser->GetTrailer()) );
    this->SetTrailer ( pTrailer ); // Set immediately as trailer
                                   // so that pTrailer has an owner
                                   // and GetIndirectKey will work

    if(PdfError::DebugEnabled())
    {
        PdfRefCountedBuffer buf;
        PdfOutputDevice debug( &buf );
        pTrailer->GetVariant().Write( debug, m_eWriteMode, nullptr );
        debug.Write("\n", 1);
        size_t siz = buf.GetSize();
        char*  ptr = buf.GetBuffer();
        PdfError::LogMessage(ELogSeverity::Information, "%.*s", siz, ptr);
    }

    PdfObject* pCatalog = pTrailer->GetIndirectKey( "Root" );
    if( !pCatalog )
        PODOFO_RAISE_ERROR_INFO( EPdfError::NoObject, "Catalog object not found!" );

    PdfObject* pInfo = pTrailer->GetIndirectKey( "Info" );
    PdfInfo*   pInfoObj;
    if( !pInfo ) 
    {
        pInfoObj = new PdfInfo( &PdfDocument::GetObjects() );
        pTrailer->GetDictionary().AddKey( "Info", pInfoObj->GetObject()->GetIndirectReference() );
    }
    else 
        pInfoObj = new PdfInfo( pInfo );

    if( pParser->IsEncrypted() ) 
    {
        // All PdfParser instances have a pointer to a PdfEncrypt object.
        // So we have to take ownership of it (command the parser to give it).
        delete m_pEncrypt;
        m_pEncrypt = pParser->TakeEncrypt();
    }

    this->SetCatalog ( pCatalog );
    this->SetInfo    ( pInfoObj );

    InitPagesTree();

    // Delete the temporary pdfparser object.
    // It is only set to m_pParser so that SetPassword can work
    delete m_pParser;
    m_pParser = NULL;
}

void PdfMemDocument::Load(const string_view& filename)
{
    if(filename.length() == 0)
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( &PdfDocument::GetObjects() );
    try
    {
        m_pParser->ParseFile(filename.data(), true );
        InitFromParser( m_pParser );
    } catch (PdfError& e)
    {
        if ( e.GetError() != EPdfError::InvalidPassword )
        {
            Clear(); // avoid m_pParser leak (issue #49)
            e.AddToCallstack( __FILE__, __LINE__, "Handler fixes issue #49" );
        }
        throw;
    }
}

void PdfMemDocument::LoadFromBuffer( const char* pBuffer, long lLen)
{
    if( !pBuffer || !lLen )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( &PdfDocument::GetObjects() );
    m_pParser->ParseFile( pBuffer, lLen, true );
    InitFromParser( m_pParser );
}

void PdfMemDocument::LoadFromDevice( const PdfRefCountedInputDevice & rDevice)
{
    this->Clear();

    // Call parse file instead of using the constructor
    // so that m_pParser is initialized for encrypted documents
    m_pParser = new PdfParser( &PdfDocument::GetObjects() );
    m_pParser->ParseFile( rDevice, true );
    InitFromParser( m_pParser );
}

/** Add a vendor-specific extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
void PdfMemDocument::AddPdfExtension( const char* ns, int64_t level ) {
    
    if (!this->HasPdfExtension(ns, level)) {
        
        PdfObject* pExtensions = this->GetCatalog()->GetIndirectKey("Extensions");
        PdfDictionary newExtension;
        
        newExtension.AddKey("BaseVersion", PdfName(s_szPdfVersionNums[(int)m_eVersion]));
        newExtension.AddKey("ExtensionLevel", PdfVariant(level));
        
        if (pExtensions && pExtensions->IsDictionary()) {
            
            pExtensions->GetDictionary().AddKey(ns, newExtension);
            
        } else {
            
            PdfDictionary extensions;
            extensions.AddKey(ns, newExtension);
            this->GetCatalog()->GetDictionary().AddKey("Extensions", extensions);
        }
    }
}

/** Checks whether the documents is tagged to imlpement a vendor-specific
 *  extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
bool PdfMemDocument::HasPdfExtension( const char* ns, int64_t level ) const {
    
    PdfObject* pExtensions = this->GetCatalog()->GetIndirectKey("Extensions");
    
    if (pExtensions) {
        
        PdfObject* pExtension = pExtensions->GetIndirectKey(ns);
        
        if (pExtension) {
            
            PdfObject* pLevel = pExtension->GetIndirectKey("ExtensionLevel");
            
            if (pLevel && pLevel->IsNumber() && pLevel->GetNumber() == level)
                return true;
        }
    }
    
    return false;
}

/** Return the list of all vendor-specific extensions to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
std::vector<PdfExtension> PdfMemDocument::GetPdfExtensions() const {
    
    std::vector<PdfExtension> result;
    
    PdfObject* pExtensions = this->GetCatalog()->GetIndirectKey("Extensions");

    if (pExtensions)
    {
        // Loop through all declared extensions
        for (TKeyMap::const_iterator it = pExtensions->GetDictionary().begin(); it != pExtensions->GetDictionary().end(); ++it) {

            PdfObject *bv = it->second.GetIndirectKey("BaseVersion");
            PdfObject *el = it->second.GetIndirectKey("ExtensionLevel");
            
            if (bv && el && bv->IsName() && el->IsNumber()) {

                // Convert BaseVersion name to EPdfVersion
                for(int i=0; i<=MAX_PDF_VERSION_STRING_INDEX; i++) {
                    if(bv->GetName().GetString() == s_szPdfVersionNums[i]) {
                        result.push_back(PdfExtension(it->first.GetString().c_str(), static_cast<EPdfVersion>(i), el->GetNumber()));
                    }
                }
            }
        }
    }
    
    return result;
}
    

    
/** Remove a vendor-specific extension to the current PDF version.
 *  \param ns  namespace of the extension
 *  \param level  level of the extension
 */
void PdfMemDocument::RemovePdfExtension( const char* ns, int64_t level ) {
    
    if (this->HasPdfExtension(ns, level))
        this->GetCatalog()->GetIndirectKey("Extensions")->GetDictionary().RemoveKey("ns");
}

void PdfMemDocument::SetPassword( const std::string_view& sPassword )
{
    PODOFO_RAISE_LOGIC_IF( !m_pParser, "SetPassword called without reading a PDF file." );

    m_pParser->SetPassword( sPassword );
    InitFromParser( m_pParser );
}

void PdfMemDocument::Write(const std::string_view& filename, PdfSaveOptions options)
{
    PdfOutputDevice device(filename);
    this->Write(device, options);
}

void PdfMemDocument::Write(PdfOutputDevice& device, PdfSaveOptions options)
{
     // makes sure pending subset-fonts are embedded
    m_fontCache.EmbedSubsetFonts();

    PdfWriter writer( &(this->GetObjects()), this->GetTrailer() );
    writer.SetPdfVersion( this->GetPdfVersion() );
    writer.SetSaveOptions(options);
    writer.SetWriteMode( m_eWriteMode );

    if( m_pEncrypt ) 
        writer.SetEncrypted( *m_pEncrypt );

    writer.Write(device);    
}

void PdfMemDocument::WriteUpdate(const string_view& filename, PdfSaveOptions options)
{
    PdfOutputDevice device(filename, false);
    this->WriteUpdate(device, options);
}

void PdfMemDocument::WriteUpdate(PdfOutputDevice& device, PdfSaveOptions options)
{
    // makes sure pending subset-fonts are embedded
    m_fontCache.EmbedSubsetFonts();
    PdfWriter writer( &(this->GetObjects()), this->GetTrailer() );
    writer.SetSaveOptions(options);
    writer.SetPdfVersion( this->GetPdfVersion() );
    writer.SetWriteMode( m_eWriteMode );
    writer.SetPrevXRefOffset(m_lPrevXRefOffset);

    bool bRewriteXRefTable = m_bLinearized || m_bSoureHasXRefStream;
    writer.SetIncrementalUpdate(bRewriteXRefTable);

    if( m_pEncrypt ) 
        writer.SetEncrypted( *m_pEncrypt );

    if( m_eSourceVersion < this->GetPdfVersion() && this->GetCatalog() && this->GetCatalog()->IsDictionary() )
    {
        if( this->GetPdfVersion() < EPdfVersion::V1_0 || this->GetPdfVersion() > EPdfVersion::V1_7 )
            PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );

        this->GetCatalog()->GetDictionary().AddKey( PdfName( "Version" ), PdfName( s_szPdfVersionNums[(int)this->GetPdfVersion()] ) );
    }

    try
    {
        writer.Write(device);
    }
    catch( PdfError & e )
    {
        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }
}

PdfObject* PdfMemDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    return this->GetCatalog()->GetIndirectKey( PdfName( pszName ) );
}

void PdfMemDocument::DeletePages( int inFirstPage, int inNumPages )
{
    for( int i = 0 ; i < inNumPages ; i++ )
    {
        this->GetPagesTree()->DeletePage( inFirstPage ) ;
    }
}

const PdfMemDocument & PdfMemDocument::InsertPages( const PdfMemDocument & rDoc, int inFirstPage, int inNumPages )
{
    /*
      This function works a bit different than one might expect. 
      Rather than copying one page at a time - we copy the ENTIRE document
      and then delete the pages we aren't interested in.
      
      We do this because 
      1) SIGNIFICANTLY simplifies the process
      2) Guarantees that shared objects aren't copied multiple times
      3) offers MUCH faster performance for the common cases
      
      HOWEVER: because PoDoFo doesn't currently do any sort of "object garbage collection" during
      a Write() - we will end up with larger documents, since the data from unused pages
      will also be in there.
    */

    // calculate preliminary "left" and "right" page ranges to delete
    // then offset them based on where the pages were inserted
    // NOTE: some of this will change if/when we support insertion at locations
    //       OTHER than the end of the document!
    int leftStartPage = 0 ;
    int leftCount = inFirstPage ;
    int rightStartPage = inFirstPage + inNumPages ;
    int rightCount = rDoc.GetPageCount() - rightStartPage ;
    int pageOffset = this->GetPageCount();	

    leftStartPage += pageOffset ;
    rightStartPage += pageOffset ;
    
    // append in the whole document
    this->Append( rDoc );

    // delete
    if( rightCount > 0 )
        this->DeletePages( rightStartPage, rightCount ) ;
    if( leftCount > 0 )
        this->DeletePages( leftStartPage, leftCount ) ;
    
    return *this;
}

void PdfMemDocument::SetEncrypted( const std::string & userPassword, const std::string & ownerPassword, 
                                   EPdfPermissions protection, EPdfEncryptAlgorithm eAlgorithm,
                                   EPdfKeyLength eKeyLength )
{
    delete m_pEncrypt;
	m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( userPassword, ownerPassword, protection, eAlgorithm, eKeyLength );
}

void PdfMemDocument::SetEncrypted( const PdfEncrypt & pEncrypt )
{
    delete m_pEncrypt;
    m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( pEncrypt );
}

PdfFont* PdfMemDocument::GetFont( PdfObject* pObject )
{
    return m_fontCache.GetFont( pObject );
}

void PdfMemDocument::FreeObjectMemory( const PdfReference & rRef, bool bForce )
{
    FreeObjectMemory( this->GetObjects().GetObject( rRef ), bForce );
}

void PdfMemDocument::FreeObjectMemory( PdfObject* pObj, bool bForce )
{
    if( !pObj ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
    
    PdfParserObject* pParserObject = dynamic_cast<PdfParserObject*>(pObj);
    if( !pParserObject ) 
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidHandle, 
                                 "FreeObjectMemory works only on classes of type PdfParserObject." );
    }

    pParserObject->FreeObjectMemory( bForce );
}

bool PdfMemDocument::IsPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsPrintAllowed() : true;
}

bool PdfMemDocument::IsEditAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditAllowed() : true;
}

bool PdfMemDocument::IsCopyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsCopyAllowed() : true;
}

bool PdfMemDocument::IsEditNotesAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditNotesAllowed() : true;
}

bool PdfMemDocument::IsFillAndSignAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsFillAndSignAllowed() : true;
}

bool PdfMemDocument::IsAccessibilityAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsAccessibilityAllowed() : true;
}

bool PdfMemDocument::IsDocAssemblyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsDocAssemblyAllowed() : true;
}

bool PdfMemDocument::IsHighPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsHighPrintAllowed() : true;
}
