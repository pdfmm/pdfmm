/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfContentsTokenizer.h"

#include "PdfCanvas.h"
#include "PdfInputDevice.h"
#include "PdfOutputStream.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"
#include "PdfData.h"
#include "PdfDefinesPrivate.h"

#include <iostream>

using namespace PoDoFo;

PdfContentsTokenizer::PdfContentsTokenizer( PdfCanvas* pCanvas )
    : PdfTokenizer(), m_readingInlineImgData(false)
{
    if( !pCanvas ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    PdfObject* contents = pCanvas->GetContents();
    if (contents == nullptr)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidHandle, "/Contents handle is null");

    if(contents->IsArray())
    {
        PdfArray& contentsArr = contents->GetArray();
        for ( int i = 0; i < contentsArr.GetSize() ; i++ )
        {
            auto &streamObj = contentsArr.FindAt(i);
            m_lstContents.push_back(&streamObj);
        }
    }
    else if (contents->IsDictionary())
    {
        // Pages are allowed to be empty, e.g.:
        //    103 0 obj
        //    <<
        //    /Type /Page
        //    /MediaBox [ 0 0 595 842 ]
        //    /Parent 3 0 R
        //    /Resources <<
        //    /ProcSet [ /PDF ]
        //    >>
        //    /Rotate 0
        //    >>
        //    endobj

        // CLEAN-ME: We shouldn't insert dictionaries that don't have
        // a stream. But the code in this class is pure shit and fails
        // with an empty queue. Fix it so it will work in all cases
        // and re-enable following lines
        //if (contents->HasStream())
            m_lstContents.push_back(contents);
    }
    else
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Page /Contents not stream or array of streams" );
    }

    if (m_lstContents.size() != 0)
    {
        SetCurrentContentsStream( *m_lstContents.front() );
        m_lstContents.pop_front();
    }
}

void PdfContentsTokenizer::SetCurrentContentsStream( PdfObject & pObject )
{
    PdfStream &pStream = pObject.GetOrCreateStream();

	PdfRefCountedBuffer buffer;
    PdfBufferOutputStream stream( &buffer );
    pStream.GetFilteredCopy(&stream);

    m_device = PdfRefCountedInputDevice( buffer.GetBuffer(), buffer.GetSize() );
}

bool PdfContentsTokenizer::GetNextToken( const char*& pszToken , EPdfTokenType* peType )
{
	bool result = PdfTokenizer::GetNextToken(pszToken, peType);
	while (!result) {
		if( !m_lstContents.size() )
			return false;

		SetCurrentContentsStream( *m_lstContents.front() );
		m_lstContents.pop_front();
		result = PdfTokenizer::GetNextToken(pszToken, peType);
	}
	return result;
}


bool PdfContentsTokenizer::ReadNext( EPdfContentsType& reType, const char*& rpszKeyword, PdfVariant & rVariant )
{
    if (m_readingInlineImgData)
        return ReadInlineImgData(reType, rpszKeyword, rVariant);
    EPdfTokenType eTokenType;
    EPdfLiteralDataType eDataType;
    const char*   pszToken;

    // While officially the keyword pointer is undefined if not needed, it
    // costs us practically nothing to zero it (in case someone fails to check
    // the return value and/or reType). Do so. We won't nullify the variant
    // since that has a real cost.
    //rpszKeyword = 0;

    // If we've run out of data in this stream and there's another one to read,
    // switch to reading the next stream.
    //if( m_device.Device() && m_device.Device()->Eof() && m_lstContents.size() )
    //{
    //    SetCurrentContentsStream( m_lstContents.front() );
    //    m_lstContents.pop_front();
    //}

    bool gotToken = this->GetNextToken( pszToken, &eTokenType );
    if ( !gotToken )
    {
        if ( m_lstContents.size() )
        {
        // We ran out of tokens in this stream. Switch to the next stream
        // and try again.
            SetCurrentContentsStream( *m_lstContents.front() );
            m_lstContents.pop_front();
            return ReadNext( reType, rpszKeyword, rVariant );
        }
        else
        {
            // No more content stream tokens to read.
            return false;
        }
    }

    eDataType = this->DetermineDataType( pszToken, eTokenType, rVariant );

    // asume we read a variant unless we discover otherwise later.
    reType = EPdfContentsType::Variant;

    switch( eDataType )
    {
        case EPdfLiteralDataType::Null:
        case EPdfLiteralDataType::Bool:
        case EPdfLiteralDataType::Number:
        case EPdfLiteralDataType::Real:
            // the data was already read into rVariant by the DetermineDataType function
            break;

        case EPdfLiteralDataType::Reference:
        {
            // references are invalid in content streams
            PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "references are invalid in content streams" );
            break;
        }

        case EPdfLiteralDataType::Dictionary:
            this->ReadDictionary( rVariant, nullptr );
            break;
        case EPdfLiteralDataType::Array:
            this->ReadArray( rVariant, nullptr);
            break;
        case EPdfLiteralDataType::String:
            this->ReadString( rVariant, nullptr);
            break;
        case EPdfLiteralDataType::HexString:
            this->ReadHexString(rVariant, nullptr);
            break;
        case EPdfLiteralDataType::Name:
            this->ReadName( rVariant );
            break;

        default:
            // Assume we have a keyword
            reType     = EPdfContentsType::Keyword;
            rpszKeyword = pszToken;
            break;
    }
    std::string idKW ("ID");
    if (reType == EPdfContentsType::Keyword && idKW.compare(rpszKeyword) == 0)
        m_readingInlineImgData = true;
    return true;
}

bool PdfContentsTokenizer::ReadInlineImgData( EPdfContentsType& reType, const char*&, PdfVariant & rVariant )
{
    int  c;
    int64_t  counter  = 0;
    if( !m_device.Device() )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // consume the only whitespace between ID and data
    c = m_device.Device()->Look();
    if( PdfTokenizer::IsWhitespace( c ) )
    {
        c = m_device.Device()->GetChar();
    }

    while((c = m_device.Device()->Look()) != EOF) 
    {
        c = m_device.Device()->GetChar(); 
        if (c=='E' &&  m_device.Device()->Look()=='I') 
        {
            // Consume character
            m_device.Device()->GetChar();
            int w = m_device.Device()->Look();
            if (w==EOF || PdfTokenizer::IsWhitespace(w)) 
            {
                // EI is followed by whitespace => stop
                m_device.Device()->Seek(-2, std::ios::cur); // put back "EI" 
                m_buffer.GetBuffer()[counter] = '\0';
                rVariant = PdfData({ m_buffer.GetBuffer(), static_cast<size_t>(counter) });
                reType = EPdfContentsType::ImageData;
                m_readingInlineImgData = false;
                return true;
            }
            else 
            {
                // no whitespace after EI => do not stop
                m_device.Device()->Seek(-1, std::ios::cur); // put back "I" 
                m_buffer.GetBuffer()[counter] = c;
                ++counter;    
            }
        }
        else 
        {
            m_buffer.GetBuffer()[counter] = c;
            ++counter;
        }
        
        if (counter ==  static_cast<int64_t>(m_buffer.GetSize())) 
        {
            // image is larger than buffer => resize buffer
            m_buffer.Resize(m_buffer.GetSize()*2);
        }
    }
    
    return false;
}
