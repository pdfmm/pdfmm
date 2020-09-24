/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#include "PdfWriter.h"

#include "PdfData.h"
#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfObject.h"
#include "PdfParser.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVariant.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfDefinesPrivate.h"

#define PDF_MAGIC           "\xe2\xe3\xcf\xd3\n"
// 10 spaces
#define LINEARIZATION_PADDING "          " 

#include <iostream>
#include <stdlib.h>

namespace PoDoFo {

PdfWriter::PdfWriter( PdfParser* pParser )
    : m_bXRefStream( false ), m_pEncrypt( NULL ), 
      m_pEncryptObj( NULL ), 
      m_eWriteMode( EPdfWriteMode::Compact ),
      m_lPrevXRefOffset( 0 ),
      m_bIncrementalUpdate( false ),
      m_lFirstInXRef( 0 ),
      m_lLinearizedOffset(0),
      m_lLinearizedLastOffset(0),
      m_lTrailerOffset(0)
{
    if( !(pParser && pParser->GetTrailer()) )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_eVersion     = pParser->GetPdfVersion();
    m_pTrailer     = new PdfObject( *(pParser->GetTrailer()) );
    m_vecObjects   = pParser->m_vecObjects;
}

PdfWriter::PdfWriter( PdfVecObjects* pVecObjects, const PdfObject* pTrailer )
    : m_bXRefStream( false ), m_pEncrypt( NULL ), 
      m_pEncryptObj( NULL ), 
      m_eWriteMode( EPdfWriteMode::Compact ),
      m_lPrevXRefOffset( 0 ),
      m_bIncrementalUpdate( false ),
      m_lFirstInXRef( 0 ),
      m_lLinearizedOffset(0),
      m_lLinearizedLastOffset(0),
      m_lTrailerOffset(0)
{
    if( !pVecObjects || !pTrailer )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_eVersion     = PdfVersionDefault;
    m_pTrailer     = new PdfObject( *pTrailer );
    m_vecObjects   = pVecObjects;
}

PdfWriter::PdfWriter( PdfVecObjects* pVecObjects )
    : m_bXRefStream( false ), m_pEncrypt( NULL ), 
      m_pEncryptObj( NULL ), 
      m_eWriteMode( EPdfWriteMode::Compact ),
      m_lPrevXRefOffset( 0 ),
      m_bIncrementalUpdate( false ),
      m_lFirstInXRef( 0 ),
      m_lLinearizedOffset(0),
      m_lLinearizedLastOffset(0),
      m_lTrailerOffset(0)
{
    m_eVersion     = PdfVersionDefault;
    m_pTrailer     = new PdfObject();
    m_vecObjects   = pVecObjects;
}

PdfWriter::~PdfWriter()
{
    delete m_pTrailer;
    delete m_pEncrypt;

    m_pTrailer     = NULL;
    m_vecObjects   = NULL;
}

void PdfWriter::Write( const char* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}

#ifdef _WIN32
void PdfWriter::Write( const wchar_t* pszFilename )
{
    PdfOutputDevice device( pszFilename );

    this->Write( &device );
}
#endif // _WIN32

void PdfWriter::Write( PdfOutputDevice* pDevice )
{
    this->Write( pDevice, false );
}

void PdfWriter::Write( PdfOutputDevice* pDevice, bool bRewriteXRefTable )
{
    CreateFileIdentifier( m_identifier, m_pTrailer, &m_originalIdentifier );

    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // setup encrypt dictionary
    if( m_pEncrypt )
    {
        m_pEncrypt->GenerateEncryptionKey( m_identifier );

        // Add our own Encryption dictionary
        m_pEncryptObj = m_vecObjects->CreateObject();
        m_pEncrypt->CreateEncryptionDictionary( m_pEncryptObj->GetDictionary() );
    }

    PdfXRef* pXRef = m_bXRefStream ? new PdfXRefStream( m_vecObjects, this ) : new PdfXRef();

    try {
        if( !m_bIncrementalUpdate )
            WritePdfHeader( pDevice );

        WritePdfObjects( pDevice, *m_vecObjects, pXRef, bRewriteXRefTable );

        if( m_bIncrementalUpdate )
            pXRef->SetFirstEmptyBlock();

        pXRef->Write( pDevice );
        
        // XRef streams contain the trailer in the XRef
        if( !m_bXRefStream ) 
        {
            PdfObject  trailer;
            
            // if we have a dummy offset we write also a prev entry to the trailer
            FillTrailerObject( &trailer, pXRef->GetSize(), false );
            
            pDevice->Print("trailer\n");
            trailer.Write(*pDevice, m_eWriteMode, nullptr); // Do not encrypt the trailer dictionary!!!
        }
        
        pDevice->Print( "startxref\n%" PDF_FORMAT_UINT64 "\n%%%%EOF\n", pXRef->GetOffset() );
        delete pXRef;
    } catch( PdfError & e ) {
        // Make sure pXRef is always deleted
        delete pXRef;
        
        // P.Zent: Delete Encryption dictionary (cannot be reused)
        if(m_pEncryptObj) {
            m_vecObjects->RemoveObject(m_pEncryptObj->GetIndirectReference());
            delete m_pEncryptObj;
        }
        
        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }
    
    // P.Zent: Delete Encryption dictionary (cannot be reused)
    if(m_pEncryptObj) {
        m_vecObjects->RemoveObject(m_pEncryptObj->GetIndirectReference());
        delete m_pEncryptObj;
    }
}

void PdfWriter::WriteUpdate( PdfOutputDevice* pDevice, PdfInputDevice* pSourceInputDevice, bool bRewriteXRefTable )
{
    if( !pDevice )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // make sure it's set that this is an incremental update
    m_bIncrementalUpdate = true;

    // the source device can be NULL, then the output device
    // is positioned at the end of the original file by the caller
    if( pSourceInputDevice )
    {
        // copy the original file content first
        size_t uBufferLen = 65535;
        char *pBuffer;

        while( pBuffer = reinterpret_cast<char *>( podofo_malloc( sizeof( char ) * uBufferLen) ), !pBuffer )
        {
            uBufferLen = uBufferLen / 2;
            if( !uBufferLen )
                break;
        }

        if( !pBuffer )
            PODOFO_RAISE_ERROR (EPdfError::OutOfMemory);

        try {
            pSourceInputDevice->Seek(0);

            while( !pSourceInputDevice->Eof() )
            {
                size_t didRead = pSourceInputDevice->Read( pBuffer, uBufferLen );
                if( didRead > 0)
                    pDevice->Write( pBuffer, didRead );
            }

            podofo_free( pBuffer );
        } catch( PdfError & e ) {
            podofo_free( pBuffer );
            throw e;
        }
    }

    // then write the changes
    this->Write (pDevice, bRewriteXRefTable );
}

void PdfWriter::WritePdfHeader( PdfOutputDevice* pDevice )
{
    pDevice->Print( "%s\n%%%s", s_szPdfVersions[static_cast<int>(m_eVersion)], PDF_MAGIC );
}

void PdfWriter::WritePdfObjects( PdfOutputDevice* pDevice, const PdfVecObjects& vecObjects, PdfXRef* pXref, bool bRewriteXRefTable )
{
    TCIVecObjects itObjects, itObjectsEnd = vecObjects.end();

    for( itObjects = vecObjects.begin(); itObjects !=  itObjectsEnd; ++itObjects )
    {
        PdfObject *pObject = *itObjects;
	    if( m_bIncrementalUpdate )
        {
            if(!pObject->IsDirty())
            {
                bool canSkip = !bRewriteXRefTable;
                if( bRewriteXRefTable )
                {
                    const PdfParserObject *parserObject = dynamic_cast<const PdfParserObject *>(pObject);
                    if (parserObject)
                    {
                        // the reference looks like "0 0 R", while the object identifier like "0 0 obj", thus add two letters
                        size_t objRefLength = pObject->GetIndirectReference().ToString().length() + 2;

                        // the offset points just after the "0 0 obj" string
                        if (parserObject->GetOffset() - objRefLength > 0)
                        {
                            pXref->AddObject(pObject->GetIndirectReference(), parserObject->GetOffset() - objRefLength, true);
                            canSkip = true;
                        }
                    }
                }

                if( canSkip )
                    continue;
            }
        }

        pXref->AddObject( pObject->GetIndirectReference(), pDevice->Tell(), true );

        // Make sure that we do not encrypt the encryption dictionary!
        pObject->Write(*pDevice, m_eWriteMode, 
                              pObject == m_pEncryptObj ? nullptr : m_pEncrypt);
    }

    TCIPdfReferenceList itFree, itFreeEnd = vecObjects.GetFreeObjects().end();
    for( itFree = vecObjects.GetFreeObjects().begin(); itFree != itFreeEnd; ++itFree )
    {
        pXref->AddObject( *itFree, 0, false );
    }
}

void PdfWriter::WriteToBuffer( char** ppBuffer, size_t* pulLen )
{
    PdfOutputDevice device;

    if( !pulLen )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    this->Write( &device );

    *pulLen = device.GetLength();
    *ppBuffer = static_cast<char*>(podofo_calloc( *pulLen, sizeof(char) ));
    if( !*ppBuffer )
    {
        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }

    PdfOutputDevice memDevice( *ppBuffer, *pulLen );
    this->Write( &memDevice );
}

void PdfWriter::FillTrailerObject( PdfObject* pTrailer, size_t lSize, bool bOnlySizeKey ) const
{
    pTrailer->GetDictionary().AddKey( PdfName::KeySize, static_cast<int64_t>(lSize) );

    if( !bOnlySizeKey ) 
    {
        if( m_pTrailer->GetDictionary().HasKey( "Root" ) )
            pTrailer->GetDictionary().AddKey( "Root", m_pTrailer->GetDictionary().GetKey( "Root" ) );
        /*
          DominikS: It makes no sense to simple copy an encryption key
                    Either we have no encryption or we encrypt again by ourselves
        if( m_pTrailer->GetDictionary().HasKey( "Encrypt" ) )
            pTrailer->GetDictionary().AddKey( "Encrypt", m_pTrailer->GetDictionary().GetKey( "Encrypt" ) );
        */
        if( m_pTrailer->GetDictionary().HasKey( "Info" ) )
            pTrailer->GetDictionary().AddKey( "Info", m_pTrailer->GetDictionary().GetKey( "Info" ) );


        if( m_pEncryptObj ) 
            pTrailer->GetDictionary().AddKey( PdfName("Encrypt"), m_pEncryptObj->GetIndirectReference() );

        PdfArray array;
        // The ID is the same unless the PDF was incrementally updated
        if( m_bIncrementalUpdate && m_originalIdentifier.IsValid() && m_originalIdentifier.GetLength() > 0 )
        {
            array.push_back( m_originalIdentifier );
        }
        else
        {
            array.push_back( m_identifier );
        }
        array.push_back( m_identifier );

        // finally add the key to the trailer dictionary
        pTrailer->GetDictionary().AddKey( "ID", array );

        if( m_lPrevXRefOffset > 0 )
        {
            PdfVariant value( m_lPrevXRefOffset );

            pTrailer->GetDictionary().AddKey( "Prev", value );
        }
    }
}

void PdfWriter::CreateFileIdentifier( PdfString & identifier, const PdfObject* pTrailer, PdfString* pOriginalIdentifier ) const
{
    PdfOutputDevice length;
    PdfObject*      pInfo;
    char*           pBuffer;
    bool            bOriginalIdentifierFound = false;
    
    if( pOriginalIdentifier && pTrailer->GetDictionary().HasKey( "ID" ))
    {
        const PdfObject* idObj = pTrailer->GetDictionary().GetKey("ID");
        // The PDF spec, section 7.5.5, implies that the ID may be
        // indirect as long as the PDF is not encrypted. Handle that
        // case.
        if ( idObj->IsReference() ) {
            idObj = m_vecObjects->GetObject( idObj->GetReference() );
        }

        TCIVariantList it = idObj->GetArray().begin();
        const PdfString* str;
        if( it != idObj->GetArray().end() &&
            it->TryGetString(str) && str->IsHex() )
        {
            *pOriginalIdentifier = it->GetString();
            bOriginalIdentifierFound = true;
        }
    }

    // create a dictionary with some unique information.
    // This dictionary is based on the PDF files information
    // dictionary if it exists.
    if( pTrailer->GetDictionary().HasKey("Info") )
    {
        const PdfReference & rRef = pTrailer->GetDictionary().GetKey( "Info" )->GetReference();
        const PdfObject* pObj = m_vecObjects->GetObject( rRef );

        if( pObj ) 
        {
            pInfo = new PdfObject( *pObj );
        }
        else
        {
            std::ostringstream oss;
            oss << "Error while retrieving info dictionary: " 
                << rRef.ObjectNumber() << " " 
                << rRef.GenerationNumber() << " R" << std::endl;
            PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidHandle, oss.str().c_str() );
        }
    }
    else 
    {
        PdfDate   date;
        PdfString dateString = date.ToString();

        pInfo = new PdfObject();
        pInfo->GetDictionary().AddKey( "CreationDate", dateString );
        pInfo->GetDictionary().AddKey( "Creator", PdfString("PoDoFo") );
        pInfo->GetDictionary().AddKey( "Producer", PdfString("PoDoFo") );
    }
    
    pInfo->GetDictionary().AddKey( "Location", PdfString("SOMEFILENAME") );

    pInfo->Write(length, m_eWriteMode, nullptr);

    pBuffer = static_cast<char*>(podofo_calloc( length.GetLength(), sizeof(char) ));
    if( !pBuffer )
    {
        delete pInfo;
        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }

    PdfOutputDevice device( pBuffer, length.GetLength() );
    pInfo->Write(device, m_eWriteMode, nullptr);

    // calculate the MD5 Sum
    identifier = PdfEncryptMD5Base::GetMD5String( reinterpret_cast<unsigned char*>(pBuffer),
                                           static_cast<unsigned int>(length.GetLength()) );
    podofo_free( pBuffer );

    delete pInfo;

    if( pOriginalIdentifier && !bOriginalIdentifierFound )
        *pOriginalIdentifier = identifier;
}

void PdfWriter::SetEncrypted( const PdfEncrypt & rEncrypt )
{
	delete m_pEncrypt;
	m_pEncrypt = PdfEncrypt::CreatePdfEncrypt( rEncrypt );
}

};

