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

using namespace std;
using namespace PoDoFo;

PdfWriter::PdfWriter(PdfVecObjects* pVecObjects, const PdfObject* pTrailer, EPdfVersion version) :
    m_vecObjects(pVecObjects),
    m_pTrailer(pTrailer),
    m_eVersion(version),
    m_UseXRefStream(false),
    m_pEncrypt(NULL),
    m_saveOptions(PdfSaveOptions::None),
    m_eWriteMode(EPdfWriteMode::Compact),
    m_lPrevXRefOffset(0),
    m_bIncrementalUpdate(false),
    m_rewriteXRefTable(false),
    m_lFirstInXRef(0),
    m_lLinearizedOffset(0),
    m_lLinearizedLastOffset(0),
    m_lTrailerOffset(0)
{
}

PdfWriter::PdfWriter( PdfParser* pParser )
    : PdfWriter(pParser->m_vecObjects, new PdfObject(*pParser->GetTrailer()), pParser->GetPdfVersion())
{
}

PdfWriter::PdfWriter( PdfVecObjects* pVecObjects, const PdfObject* pTrailer )
    : PdfWriter(pVecObjects, new PdfObject(*pTrailer), PdfVersionDefault)
{
}

PdfWriter::PdfWriter(PdfVecObjects* pVecObjects)
    : PdfWriter(pVecObjects, new PdfObject(), PdfVersionDefault)
{
}

void PdfWriter::SetIncrementalUpdate(bool rewriteXRefTable)
{
    m_bIncrementalUpdate = true;
    m_rewriteXRefTable = rewriteXRefTable;
}

const char* PoDoFo::PdfWriter::GetPdfVersionString() const
{
    return s_szPdfVersionNums[static_cast<int>(m_eVersion)];
}

PdfWriter::~PdfWriter()
{
    delete m_pTrailer;
    delete m_pEncrypt;

    m_pTrailer     = NULL;
    m_vecObjects   = NULL;
}

void PdfWriter::Write(PdfOutputDevice& device)
{
    CreateFileIdentifier( m_identifier, m_pTrailer, &m_originalIdentifier );

    // setup encrypt dictionary
    if( m_pEncrypt )
    {
        m_pEncrypt->GenerateEncryptionKey( m_identifier );

        // Add our own Encryption dictionary
        m_pEncryptObj.reset(m_vecObjects->CreateObject());
        m_pEncrypt->CreateEncryptionDictionary( m_pEncryptObj->GetDictionary() );
    }

    unique_ptr<PdfXRef> pXRef;
    if (m_UseXRefStream)
        pXRef.reset(new PdfXRefStream(*this, *m_vecObjects));
    else
        pXRef.reset(new PdfXRef(*this));

    try
    {
        if( !m_bIncrementalUpdate )
            WritePdfHeader(device);

        WritePdfObjects(device, *m_vecObjects, *pXRef);

        if ( m_bIncrementalUpdate )
            pXRef->SetFirstEmptyBlock();

        pXRef->Write(device);
    }
    catch( PdfError & e )
    {   
        // P.Zent: Delete Encryption dictionary (cannot be reused)
        if(m_pEncryptObj)
        {
            m_vecObjects->RemoveObject(m_pEncryptObj->GetIndirectReference());
            m_pEncryptObj = nullptr;
        }
        
        e.AddToCallstack( __FILE__, __LINE__ );
        throw e;
    }
    
    // P.Zent: Delete Encryption dictionary (cannot be reused)
    if(m_pEncryptObj)
    {
        m_vecObjects->RemoveObject(m_pEncryptObj->GetIndirectReference());
        m_pEncryptObj = nullptr;
    }
}

void PdfWriter::WritePdfHeader(PdfOutputDevice& device)
{
    device.Print( "%s\n%%%s", s_szPdfVersions[static_cast<int>(m_eVersion)], PDF_MAGIC );
}

void PdfWriter::WritePdfObjects(PdfOutputDevice& device, const PdfVecObjects& vecObjects, PdfXRef& xref)
{
    TCIVecObjects itObjects, itObjectsEnd = vecObjects.end();

    for(PdfObject* pObject : vecObjects )
    {
	    if( m_bIncrementalUpdate )
        {
            if(!pObject->IsDirty())
            {
                if (m_rewriteXRefTable)
                {
                    PdfParserObject* parserObject = dynamic_cast<PdfParserObject*>(pObject);
                    if (parserObject != nullptr)
                    {
                        // Try to see if we can just write the reference to previous entry
                        // without rewriting the entry

                        // the reference looks like "0 0 R", while the object identifier like "0 0 obj", thus add two letters
                        size_t objRefLength = pObject->GetIndirectReference().ToString().length() + 2;

                        // the offset points just after the "0 0 obj" string
                        if (parserObject->GetOffset() - objRefLength > 0)
                        {
                            xref.AddInUseObject(pObject->GetIndirectReference(), parserObject->GetOffset() - objRefLength);
                            continue;
                        }
                    }
                }
                else
                {
                    // The object will not be output in the XRef entries but it will be
                    // counted in trailer's /Size
                    xref.AddInUseObject(pObject->GetIndirectReference(), std::nullopt);
                    continue;
                }
            }
        }

        xref.AddInUseObject( pObject->GetIndirectReference(), device.Tell());

        if (!xref.ShouldSkipWrite(pObject->GetIndirectReference()))
        {
            // Also make sure that we do not encrypt the encryption dictionary!
            pObject->Write(device, m_eWriteMode,
                pObject == m_pEncryptObj.get() ? nullptr : m_pEncrypt);
        }
    }

    for(auto& freeObjectRef : vecObjects.GetFreeObjects())
    {
        xref.AddFreeObject(freeObjectRef);
    }
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

        if (!m_rewriteXRefTable && m_lPrevXRefOffset > 0)
        {
            PdfVariant value(m_lPrevXRefOffset);
            pTrailer->GetDictionary().AddKey("Prev", value);
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

void PdfWriter::SetUseXRefStream(bool bStream)
{
    if (bStream && m_eVersion < EPdfVersion::V1_5)
        this->SetPdfVersion(EPdfVersion::V1_5);

    m_UseXRefStream = bStream;
}
