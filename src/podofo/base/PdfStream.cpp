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

#include "PdfStream.h"

#include <doc/PdfDocument.h>
#include "PdfArray.h"
#include "PdfFilter.h"
#include "PdfInputStream.h"
#include "PdfOutputStream.h"
#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"
#include "PdfDictionary.h"

#include <iostream>

#include <stdlib.h>

using namespace std;
using namespace PoDoFo;

enum EPdfFilter PdfStream::eDefaultFilter = EPdfFilter::FlateDecode;

PdfStream::PdfStream( PdfObject* pParent )
    : m_pParent( pParent ), m_bAppend( false )
{
}

PdfStream::~PdfStream() { }

void PdfStream::GetFilteredCopy( PdfOutputStream* pStream ) const
{
    TVecFilters vecFilters = PdfFilterFactory::CreateFilterList( m_pParent );
    if( vecFilters.size() )
    {
        auto pDecodeStream = PdfFilterFactory::CreateDecodeStream( vecFilters, *pStream,
            m_pParent ? &m_pParent->GetDictionary() : nullptr );

        pDecodeStream->Write( const_cast<char*>(this->GetInternalBuffer()), this->GetInternalBufferSize() );
        pDecodeStream->Close();
    }
    else
    {
        pStream->Write(this->GetInternalBuffer(), this->GetInternalBufferSize() );
    }
}

void PdfStream::GetFilteredCopy(unique_ptr<char>& buffer, size_t& len) const
{
    TVecFilters vecFilters = PdfFilterFactory::CreateFilterList( m_pParent );
    PdfMemoryOutputStream  stream;
    if( vecFilters.size() )
    {
        auto pDecodeStream = PdfFilterFactory::CreateDecodeStream( vecFilters, stream,
            m_pParent ? &m_pParent->GetDictionary() : nullptr);

        pDecodeStream->Write( this->GetInternalBuffer(), this->GetInternalBufferSize() );
        pDecodeStream->Close();
    }
    else
    {
        stream.Write(this->GetInternalBuffer(), this->GetInternalBufferSize());
        stream.Close();
    }

    buffer = unique_ptr<char>(stream.TakeBuffer());
    len = stream.GetLength();
}

const PdfStream & PdfStream::operator=(const PdfStream & rhs)
{
    if (&rhs == this)
        return *this;

    CopyFrom(rhs);
    return (*this);
}

void PdfStream::CopyFrom(const PdfStream &rhs)
{
    PdfMemoryInputStream stream( rhs.GetInternalBuffer(), rhs.GetInternalBufferSize() );
    this->SetRawData( stream );
}

void PdfStream::Set(const string_view& view, const TVecFilters& vecFilters)
{
    Set(view.data(), view.length(), vecFilters);
}

void PdfStream::Set(const char* buffer, size_t len, const TVecFilters & vecFilters )
{
    if (len == 0)
        return;

    this->BeginAppend( vecFilters );
    this->append(buffer, len);
    this->endAppend();
}

void PdfStream::Set(const string_view& view)
{
    Set(view.data(), view.length());
}

void PdfStream::Set(const char* buffer, size_t len)
{
    if (len == 0)
        return;

    this->BeginAppend();
    this->append(buffer, len);
    this->endAppend();
}

void PdfStream::Set( PdfInputStream* pStream )
{
    TVecFilters vecFilters;

    if( eDefaultFilter != EPdfFilter::None )
        vecFilters.push_back( eDefaultFilter );

    this->Set( pStream, vecFilters );
}

void PdfStream::Set( PdfInputStream* pStream, const TVecFilters & vecFilters )
{
    constexpr size_t BUFFER_SIZE = 4096;
    size_t lLen = 0;
    char buffer[BUFFER_SIZE];

    this->BeginAppend( vecFilters );

    bool eof;
    do
    {
        lLen = pStream->Read( buffer, BUFFER_SIZE, eof);
        this->append(buffer, lLen);
    }
    while (!eof);

    this->endAppend();
}
void PdfStream::SetRawData(PdfInputStream& stream, ssize_t len)
{
    SetRawData(stream, len, true);
}

void PdfStream::SetRawData(PdfInputStream& pStream, ssize_t lLen, bool markObjectDirty)
{
    constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t lRead;
    TVecFilters vecEmpty;

    // TODO: DS, give begin append a size hint so that it knows
    //       how many data has to be allocated
    this->BeginAppend( vecEmpty, true, false, markObjectDirty);
    if( lLen < 0 ) 
    {
        bool eof;
        do
        {
            lRead = pStream.Read( buffer, BUFFER_SIZE, eof);
            this->append(buffer, lRead);
        }
        while (!eof);
    }
    else
    {
        bool eof;
        do
        {
            lRead = pStream.Read(buffer, std::min( BUFFER_SIZE, (size_t)lLen), eof);
            lLen -= lRead;
            this->append(buffer, lRead);
        }
        while (lLen > 0 && !eof);
    }

    this->endAppend();
}

void PdfStream::BeginAppend( bool bClearExisting )
{
    TVecFilters vecFilters;

    if( eDefaultFilter != EPdfFilter::None )
        vecFilters.push_back( eDefaultFilter );

    this->BeginAppend( vecFilters, bClearExisting );
}

void PdfStream::BeginAppend(const TVecFilters& vecFilters, bool bClearExisting, bool bDeleteFilters)
{
    BeginAppend(vecFilters, bClearExisting, bDeleteFilters, true);
}

void PdfStream::BeginAppend(const TVecFilters& vecFilters, bool bClearExisting, bool bDeleteFilters, bool markObjectDirty)
{
    PODOFO_RAISE_LOGIC_IF( m_bAppend, "BeginAppend() failed because EndAppend() was not yet called!" );

    if( m_pParent )
    {
        if (markObjectDirty)
        {
            // We must make sure the parent will be set dirty. All methods
            // writing to the stream will call this method first
            m_pParent->SetDirty();
        }

        auto document = m_pParent->GetDocument();
        if (document != nullptr)
            document->GetObjects().BeginAppendStream( this );
    }

    size_t lLen = 0;
    unique_ptr<char> buffer;
    if(!bClearExisting && this->GetLength()) 
        this->GetFilteredCopy(buffer, lLen);

    if ( m_pParent )
    {
        if( vecFilters.size() == 0 )
        {
            if ( bDeleteFilters )
                m_pParent->GetDictionary().RemoveKey( PdfName::KeyFilter );
        }
        else if ( vecFilters.size() == 1 )
        {
            m_pParent->GetDictionary().AddKey( PdfName::KeyFilter, 
                                               PdfName( PdfFilterFactory::FilterTypeToName( vecFilters.front() ) ) );
        }
        else // vecFilters.size() > 1
        {
            PdfArray filters;
            TCIVecFilters it = vecFilters.begin();
            while( it != vecFilters.end() )
            {
                filters.push_back( PdfName( PdfFilterFactory::FilterTypeToName( *it ) ) );
                ++it;
            }
        
            m_pParent->GetDictionary().AddKey( PdfName::KeyFilter, filters );
        }
    }

    this->BeginAppendImpl( vecFilters );
    m_bAppend = true;
    if(buffer)
        this->append(buffer.get(), lLen);
}

void PdfStream::EndAppend()
{
    PODOFO_RAISE_LOGIC_IF(!m_bAppend, "EndAppend() failed because BeginAppend() was not yet called!");
    endAppend();
}

void PdfStream::endAppend()
{
    m_bAppend = false;
    this->EndAppendImpl();

    PdfDocument* document;
    if( m_pParent && (document = m_pParent->GetDocument()) != nullptr )
        document->GetObjects().EndAppendStream( this );
}

void PdfStream::Append(const string_view& view)
{
    Append(view.data(), view.length());
}

void PdfStream::Append(const char* buffer, size_t len)
{
    PODOFO_RAISE_LOGIC_IF(!m_bAppend, "Append() failed because BeginAppend() was not yet called!");
    if (len == 0)
        return;

    append(buffer, len);
}

void PdfStream::append(const char* data, size_t len)
{
    this->AppendImpl(data, len);
}
