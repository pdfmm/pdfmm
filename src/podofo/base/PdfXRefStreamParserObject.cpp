/***************************************************************************
 *   Copyright (C) 2009 by Dominik Seichter                                *
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

#include "PdfXRefStreamParserObject.h"

#include "PdfParser.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfStream.h"
#include "PdfVariant.h"

#include <limits>

using namespace std;
using namespace PoDoFo;

PdfXRefStreamParserObject::PdfXRefStreamParserObject(PdfVecObjects* pCreator, const PdfRefCountedInputDevice & rDevice, 
                                                     const PdfRefCountedBuffer & rBuffer, TVecOffsets* pOffsets )
    : PdfParserObject( pCreator, rDevice, rBuffer ), m_lNextOffset(-1), m_pOffsets( pOffsets )
{

}

void PdfXRefStreamParserObject::Parse()
{
    // Ignore the encryption in the XREF as the XREF stream must no be encrypted (see PDF Reference 3.4.7)
    this->ParseFile( NULL );

    // Do some very basic error checking
    if( !this->GetDictionary().HasKey( PdfName::KeyType ) )
    {
        PODOFO_RAISE_ERROR( EPdfError::NoXRef );
    } 

    PdfObject* pObj = this->GetDictionary().GetKey( PdfName::KeyType );
    if( !pObj->IsName() || ( pObj->GetName() != "XRef" ) )
    {
        PODOFO_RAISE_ERROR( EPdfError::NoXRef );
    }

    if( !this->GetDictionary().HasKey( PdfName::KeySize ) 
        || !this->GetDictionary().HasKey( "W" ) )
    {
        PODOFO_RAISE_ERROR( EPdfError::NoXRef );
    } 

    if( !this->HasStreamToParse() )
    {
        PODOFO_RAISE_ERROR( EPdfError::NoXRef );
    }

    if( this->GetDictionary().HasKey("Prev") )
    {
        m_lNextOffset = static_cast<ssize_t>(this->GetDictionary().GetKeyAsNumber( "Prev", 0 ));
    }
}

void PdfXRefStreamParserObject::ReadXRefTable() 
{
    int64_t lSize = this->GetDictionary().GetKeyAsNumber( PdfName::KeySize, 0 );
    auto &arr = *(this->GetDictionary().GetKey( "W" ));

    // The pdf reference states that W is always an array with 3 entries
    // all of them have to be integers
    if( !arr.IsArray() || arr.GetArray().size() != 3 )
    {
        PODOFO_RAISE_ERROR( EPdfError::NoXRef );
    }


    int64_t nW[W_ARRAY_SIZE] = { 0, 0, 0 };
    for( int i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( !arr.GetArray()[i].IsNumber() )
        {
            PODOFO_RAISE_ERROR( EPdfError::NoXRef );
        }

        nW[i] = static_cast<int64_t>(arr.GetArray()[i].GetNumber());
    }

    std::vector<int64_t> vecIndeces;
    GetIndeces( vecIndeces, static_cast<int64_t>(lSize) );

    ParseStream( nW, vecIndeces );
}

void PdfXRefStreamParserObject::ParseStream( const int64_t nW[W_ARRAY_SIZE], const std::vector<int64_t> & rvecIndeces )
{
    for(int64_t nLengthSum = 0, i = 0; i < W_ARRAY_SIZE; i++ )
    {
        if ( nW[i] < 0 )
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::NoXRef,
                                    "Negative field length in XRef stream" );
        }
        if ( std::numeric_limits<int64_t>::max() - nLengthSum < nW[i] )
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::NoXRef,
                                    "Invalid entry length in XRef stream" );
        }
        else
        {
            nLengthSum += nW[i];
        }
    }

    const size_t entryLen  = static_cast<size_t>(nW[0] + nW[1] + nW[2]);

    unique_ptr<char> buffer;
    size_t lBufferLen;
    this->GetOrCreateStream().GetFilteredCopy(buffer, lBufferLen);
    
    std::vector<int64_t>::const_iterator it = rvecIndeces.begin();
    char* pBuffer = buffer.get();
    while( it != rvecIndeces.end() )
    {
        int64_t nFirstObj = *it++;
        int64_t nCount = *it++;

        while( nCount > 0 )
        {
            if((size_t)(pBuffer - buffer.get()) >= lBufferLen )
                PODOFO_RAISE_ERROR_INFO( EPdfError::NoXRef, "Invalid count in XRef stream" );

            if ( nFirstObj >= 0 && nFirstObj < static_cast<int64_t>(m_pOffsets->size()) 
                 && ! (*m_pOffsets)[static_cast<int>(nFirstObj)].bParsed)
            {
	            ReadXRefStreamEntry( pBuffer, lBufferLen, nW, static_cast<int>(nFirstObj) );
            }

			nFirstObj++ ;
            pBuffer += entryLen;
            --nCount;
        }
    }
}

void PdfXRefStreamParserObject::GetIndeces( std::vector<int64_t> & rvecIndeces, int64_t size ) 
{
    // get the first object number in this crossref stream.
    // it is not required to have an index key though.
    if( this->GetDictionary().HasKey( "Index" ) )
    {
        auto& arr = *(this->GetDictionary().GetKey( "Index" ));
        if( !arr.IsArray() )
        {
            PODOFO_RAISE_ERROR( EPdfError::NoXRef );
        }

        TCIVariantList it = arr.GetArray().begin();
        while ( it != arr.GetArray().end() )
        {
            rvecIndeces.push_back( (*it).GetNumber() );
            ++it;
        }
    }
    else
    {
        // Default
        rvecIndeces.push_back( static_cast<int64_t>(0) );
        rvecIndeces.push_back( size );
    }

    // vecIndeces must be a multiple of 2
    if( rvecIndeces.size() % 2 != 0)
    {
        PODOFO_RAISE_ERROR( EPdfError::NoXRef );
    }
}

void PdfXRefStreamParserObject::ReadXRefStreamEntry( char* pBuffer, size_t, const int64_t lW[W_ARRAY_SIZE], int nObjNo )
{
    int              i;
    int64_t        z;
    unsigned long    nData[W_ARRAY_SIZE];

    for( i=0;i<W_ARRAY_SIZE;i++ )
    {
        if( lW[i] > W_MAX_BYTES )
        {
            PdfError::LogMessage( ELogSeverity::Error, 
                                  "The XRef stream dictionary has an entry in /W of size %i.\nThe maximum supported value is %i.", 
                                  lW[i], W_MAX_BYTES );

            PODOFO_RAISE_ERROR( EPdfError::InvalidXRefStream );
        }
        
        nData[i] = 0;
        for( z=W_MAX_BYTES-lW[i];z<W_MAX_BYTES;z++ )
        {
            nData[i] = (nData[i] << 8) + static_cast<unsigned char>(*pBuffer);
            ++pBuffer;
        }
    }


    //printf("OBJ=%i nData = [ %i %i %i ]\n", nObjNo, static_cast<int>(nData[0]), static_cast<int>(nData[1]), static_cast<int>(nData[2]) );
    PdfXRefEntry &entry = (*m_pOffsets)[nObjNo];
    entry.bParsed = true;

    // TABLE 3.15 Additional entries specific to a cross - reference stream dictionary
    // /W array: "If the first element is zero, the type field is not present, and it defaults to type 1"
    switch( lW[0] == 0 ? 1 : nData[0] ) // nData[0] contains the type information of this entry
    {
        // TABLE 3.16 Entries in a cross-reference stream
        case 0:
            // a free object
            entry.lOffset     = nData[1];
            entry.lGeneration = nData[2];
            entry.eType       = EXRefEntryType::Free;
            break;
        case 1:
            // normal uncompressed object
            entry.lOffset     = nData[1];
            entry.lGeneration = nData[2];
            entry.eType       = EXRefEntryType::InUse;
            break;
        case 2:
            // object that is part of an object stream
            entry.lOffset     = nData[2]; // index in the object stream
            entry.lGeneration = nData[1]; // object number of the stream
            entry.eType       = EXRefEntryType::Compressed;
            break;
        default:
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidXRefType );
        }
    }
    //printf("m_offsets = [ %i %i %c ]\n", entry.lOffset, entry.lGeneration, entry.cUsed );
}

bool PdfXRefStreamParserObject::TryGetPreviousOffset(size_t &previousOffset) const
{
    bool ret = m_lNextOffset != -1;
    previousOffset = ret ? (size_t)m_lNextOffset : 0;
    return ret;
}
