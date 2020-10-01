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

#include "PdfFilter.h"

#include <sstream>

#include <doc/PdfDocument.h>
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfFiltersPrivate.h"
#include "PdfOutputStream.h"
#include "PdfDefinesPrivate.h"

using namespace std;
using namespace PoDoFo;

// All known filters
static const char* aszFilters[] = {
    "ASCIIHexDecode",
    "ASCII85Decode",
    "LZWDecode",
    "FlateDecode",
    "RunLengthDecode",
    "CCITTFaxDecode", 
    "JBIG2Decode",
    "DCTDecode",
    "JPXDecode",
    "Crypt",
    NULL
};

static const char* aszShortFilters[] = {
    "AHx",
    "A85",
    "LZW",
    "Fl",
    "RL",
    "CCF", 
    "", ///< There is no shortname for JBIG2Decode
    "DCT",
    "", ///< There is no shortname for JPXDecode 
    "", ///< There is no shortname for Crypt
    NULL
};

/** Create a filter that is a PdfOutputStream.
 *
 *  All data written to this stream is encoded using a
 *  filter and written to another PdfOutputStream.
 *
 *  The passed output stream is owned by this PdfOutputStream
 *  and deleted along with it.
 */
class PdfFilteredEncodeStream : public PdfOutputStream
{
public:
    /** Create a filtered output stream.
     *
     *  All data written to this stream is encoded using the passed filter type
     *  and written to the passed output stream which will be deleted 
     *  by this PdfFilteredEncodeStream.
     *
     *  \param pOutputStream write all data to this output stream after encoding the data.
     *  \param eFilter use this filter for encoding.
     *  \param bOwnStream if true pOutputStream will be deleted along with this filter
     */
    PdfFilteredEncodeStream( PdfOutputStream* pOutputStream, const EPdfFilter eFilter, bool bOwnStream )
        : m_pOutputStream( pOutputStream )
    {
        m_filter = PdfFilterFactory::Create( eFilter );

        if( !m_filter.get() )
            PODOFO_RAISE_ERROR( EPdfError::UnsupportedFilter );

        m_filter->BeginEncode( pOutputStream );

        if( !bOwnStream )
            m_pOutputStream = NULL;
    }

    virtual ~PdfFilteredEncodeStream()
    {
        delete m_pOutputStream;
    }

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     */
    void WriteImpl( const char* pBuffer, size_t lLen ) override
    {
        m_filter->EncodeBlock( pBuffer, lLen );
    }

    void Close() override
    {
        m_filter->EndEncode();
    }

private:
    PdfOutputStream*         m_pOutputStream;
    std::unique_ptr<PdfFilter> m_filter;
};

/** Create a filter that is a PdfOutputStream.
 *
 *  All data written to this stream is decoded using a
 *  filter and written to another PdfOutputStream.
 *
 *  The passed output stream is owned by this PdfOutputStream
 *  and deleted along with it (optionally, see constructor).
 */
class PdfFilteredDecodeStream : public PdfOutputStream
{
public:
    /** Create a filtered output stream.
     *
     *  All data written to this stream is decoded using the passed filter type
     *  and written to the passed output stream which will be deleted 
     *  by this PdfFilteredDecodeStream if the parameter bOwnStream is true.
     *
     *  \param pOutputStream write all data to this output stream after decoding the data.
     *         The PdfOutputStream is deleted along with this object if bOwnStream is true.
     *  \param eFilter use this filter for decoding.
     *  \param bOwnStream if true pOutputStream will be deleted along with this stream
     *  \param pDecodeParms additional parameters for decoding
     */
    PdfFilteredDecodeStream( PdfOutputStream* pOutputStream, const EPdfFilter eFilter, bool bOwnStream,
                             const PdfDictionary* pDecodeParms = NULL )
        : m_pOutputStream( pOutputStream ), m_bFilterFailed( false )
    {
        m_filter = PdfFilterFactory::Create( eFilter );
        if( !m_filter.get() ) 
        {
            PODOFO_RAISE_ERROR( EPdfError::UnsupportedFilter );
        }

        m_filter->BeginDecode( pOutputStream, pDecodeParms );

        if( !bOwnStream )
            m_pOutputStream = NULL;
    }

    virtual ~PdfFilteredDecodeStream()
    {
        delete m_pOutputStream;
    }

    /** Write data to the output stream
     *  
     *  \param pBuffer the data is read from this buffer
     *  \param lLen    the size of the buffer 
     */
    void WriteImpl(const char* pBuffer, size_t lLen) override
    {
        try
        {
            m_filter->DecodeBlock( pBuffer, lLen );
        }
        catch( PdfError & e ) 
        {
            e.AddToCallstack( __FILE__, __LINE__ );
            m_bFilterFailed = true;
            throw;
        }
    }

    void Close() override
    {
        try
        {
            if( !m_bFilterFailed ) 
            {
                m_filter->EndDecode();
            }
        }
        catch( PdfError & e )
        {
            std::ostringstream oss;
            oss << "PdfFilter::EndDecode() failed in filter of type "
                << PdfFilterFactory::FilterTypeToName( m_filter->GetType() ) << ".\n";
            e.AddToCallstack( __FILE__, __LINE__, oss.str() );
            m_bFilterFailed = true;
            throw;
        }

    }

private:
    PdfOutputStream*         m_pOutputStream;
    std::unique_ptr<PdfFilter> m_filter;
    bool                     m_bFilterFailed;
};


// -----------------------------------------------------
// Actual PdfFilter code
// -----------------------------------------------------


PdfFilter::PdfFilter() 
    : m_pOutputStream( NULL )
{
}

void PdfFilter::Encode( const char* pInBuffer, size_t lInLen, char** ppOutBuffer, size_t* plOutLen ) const
{
    if( !this->CanEncode() )
    {
        PODOFO_RAISE_ERROR( EPdfError::UnsupportedFilter );
    }

    PdfMemoryOutputStream stream;

    const_cast<PdfFilter*>(this)->BeginEncode( &stream );
    const_cast<PdfFilter*>(this)->EncodeBlock( pInBuffer, lInLen );
    const_cast<PdfFilter*>(this)->EndEncode();

    *ppOutBuffer = stream.TakeBuffer();
    *plOutLen    = stream.GetLength();
}

void PdfFilter::Decode( const char* pInBuffer, size_t lInLen, char** ppOutBuffer, size_t* plOutLen,
                        const PdfDictionary* pDecodeParms ) const
{
    if( !this->CanDecode() )
    {
        PODOFO_RAISE_ERROR( EPdfError::UnsupportedFilter );
    }

    PdfMemoryOutputStream stream;

    const_cast<PdfFilter*>(this)->BeginDecode( &stream, pDecodeParms );
    const_cast<PdfFilter*>(this)->DecodeBlock( pInBuffer, lInLen );
    const_cast<PdfFilter*>(this)->EndDecode();

    *ppOutBuffer = stream.TakeBuffer();
    *plOutLen    = stream.GetLength();
}

// -----------------------------------------------------
// PdfFilterFactory code
// -----------------------------------------------------

PdfFilterFactory::PdfFilterFactory()
{
}

std::unique_ptr<PdfFilter> PdfFilterFactory::Create( const EPdfFilter eFilter ) 
{
    PdfFilter* pFilter = NULL;
    switch( eFilter )
    {
        case EPdfFilter::None:
            break;

        case EPdfFilter::ASCIIHexDecode:
            pFilter = new PdfHexFilter();
            break;
            
        case EPdfFilter::ASCII85Decode:
            pFilter = new PdfAscii85Filter();
            break;
            
        case EPdfFilter::LZWDecode:
            pFilter = new PdfLZWFilter();
            break;
            
        case EPdfFilter::FlateDecode:
            pFilter = new PdfFlateFilter();
            break;
            
        case EPdfFilter::RunLengthDecode:
            pFilter = new PdfRLEFilter();
            break;
            
        case EPdfFilter::DCTDecode:
#ifdef PODOFO_HAVE_JPEG_LIB
            pFilter = new PdfDCTFilter();
            break;
#else
            break;
#endif // PODOFO_HAVE_JPEG_LIB

        case EPdfFilter::CCITTFaxDecode:
#ifdef PODOFO_HAVE_TIFF_LIB
            pFilter = new PdfCCITTFilter();
            break;
#else
            break;
#endif // PODOFO_HAVE_TIFF_LIB


        case EPdfFilter::JBIG2Decode:
        case EPdfFilter::JPXDecode:
        case EPdfFilter::Crypt:
        default:
            break;
    }

    return std::unique_ptr<PdfFilter>(pFilter);
}

unique_ptr<PdfOutputStream> PdfFilterFactory::CreateEncodeStream(const TVecFilters& filters, PdfOutputStream & pStream)
{
    TVecFilters::const_iterator it = filters.begin();

    PODOFO_RAISE_LOGIC_IF( !filters.size(), "Cannot create an EncodeStream from an empty list of filters" );

    auto pFilter = new PdfFilteredEncodeStream( &pStream, *it, false );
    ++it;

    while( it != filters.end() ) 
    {
        pFilter = new PdfFilteredEncodeStream( pFilter, *it, true );
        ++it;
    }

    return unique_ptr<PdfOutputStream>(pFilter);
}

unique_ptr<PdfOutputStream> PdfFilterFactory::CreateDecodeStream( const TVecFilters & filters, PdfOutputStream & pStream,
                                                       const PdfDictionary* pDictionary ) 
{
    TVecFilters::const_reverse_iterator it = filters.rbegin();

    PODOFO_RAISE_LOGIC_IF( !filters.size(), "Cannot create an DecodeStream from an empty list of filters" );

    // TODO: support arrays and indirect objects here and the short name /DP
    if( pDictionary && pDictionary->HasKey( "DecodeParms" ) && pDictionary->GetKey( "DecodeParms" )->IsDictionary() )
        pDictionary = &(pDictionary->GetKey( "DecodeParms" )->GetDictionary());

    PdfFilteredDecodeStream* pFilterStream = new PdfFilteredDecodeStream(&pStream, *it, false, pDictionary);
    ++it;

    while( it != filters.rend() ) 
    {
        pFilterStream = new PdfFilteredDecodeStream( pFilterStream, *it, true, pDictionary );
        ++it;
    }

    return unique_ptr<PdfOutputStream>(pFilterStream);
}

EPdfFilter PdfFilterFactory::FilterNameToType( const PdfName & name, bool bSupportShortNames )
{
    int i = 0;

    while( aszFilters[i] )
    {
        if( name == aszFilters[i] )
            return static_cast<EPdfFilter>(i);
        
        ++i;
    }

    if( bSupportShortNames )
    {
        i = 0;
        while( aszShortFilters[i] )
        {
            if( name == aszShortFilters[i] )
                return static_cast<EPdfFilter>(i);
            
            ++i;
        }        
    }

    PODOFO_RAISE_ERROR_INFO( EPdfError::UnsupportedFilter, name.GetString().c_str() );
}

const char* PdfFilterFactory::FilterTypeToName( EPdfFilter eFilter )
{
    return aszFilters[static_cast<int>(eFilter)];
}

TVecFilters PdfFilterFactory::CreateFilterList( const PdfObject* pObject )
{
    TVecFilters filters;

    const PdfObject* pObj    = NULL;

    if( pObject->IsDictionary() && pObject->GetDictionary().HasKey( "Filter" ) )
        pObj = pObject->GetDictionary().GetKey( "Filter" );
    else if( pObject->IsArray() )
        pObj = pObject;
    else if( pObject->IsName() ) 
        pObj = pObject;


    if (!pObj)
	// Object had no /Filter key . Return a null filter list.
	return filters;

    if( pObj->IsName() ) 
        filters.push_back( PdfFilterFactory::FilterNameToType( pObj->GetName() ) );
    else if( pObj->IsArray() ) 
    {
        TCIVariantList it = pObj->GetArray().begin();

        while( it != pObj->GetArray().end() )
        {
            if ( (*it).IsName() )
			{
                filters.push_back( PdfFilterFactory::FilterNameToType( (*it).GetName() ) );
            }
            else if ( (*it).IsReference() )
            {
                PdfObject* pFilter = pObject->GetDocument()->GetObjects().GetObject( (*it).GetReference() );
                if( pFilter == NULL ) 
                {
                    PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Filter array contained unexpected reference" );
                }

                filters.push_back( PdfFilterFactory::FilterNameToType( pFilter->GetName() ) );
            }
            else 
            {
                PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Filter array contained unexpected non-name type" );
			}
                
            ++it;
        }
    }

    return filters;
}

void PdfFilter::BeginEncode(PdfOutputStream* pOutput)
{
    PODOFO_RAISE_LOGIC_IF(m_pOutputStream, "BeginEncode() on failed filter or without EndEncode()");
    m_pOutputStream = pOutput;

    try
    {
        BeginEncodeImpl();
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EncodeBlock(const char* pBuffer, size_t lLen)
{
    PODOFO_RAISE_LOGIC_IF(!m_pOutputStream, "EncodeBlock() without BeginEncode() or on failed filter");

    try
    {
        EncodeBlockImpl(pBuffer, lLen);
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EndEncode()
{
    PODOFO_RAISE_LOGIC_IF(!m_pOutputStream, "EndEncode() without BeginEncode() or on failed filter");

    try
    {
        EndEncodeImpl();
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }

    m_pOutputStream->Close();
    m_pOutputStream = NULL;
}

void PdfFilter::BeginDecode(PdfOutputStream* pOutput, const PdfDictionary* pDecodeParms)
{
    PODOFO_RAISE_LOGIC_IF(m_pOutputStream, "BeginDecode() on failed filter or without EndDecode()");
    m_pOutputStream = pOutput;

    try
    {
        BeginDecodeImpl(pDecodeParms);
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::DecodeBlock(const char* pBuffer, size_t lLen)
{
    PODOFO_RAISE_LOGIC_IF(!m_pOutputStream, "DecodeBlock() without BeginDecode() or on failed filter")

    try
    {
        DecodeBlockImpl(pBuffer, lLen);
    }
    catch (...)
    {
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
}

void PdfFilter::EndDecode()
{
    PODOFO_RAISE_LOGIC_IF(!m_pOutputStream, "EndDecode() without BeginDecode() or on failed filter")

    try
    {
        EndDecodeImpl();
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__);
        // Clean up and close stream
        this->FailEncodeDecode();
        throw;
    }
    try
    { // introduced to fix issue #58
        if (m_pOutputStream)
        {
            m_pOutputStream->Close();
            m_pOutputStream = NULL;
        }
    }
    catch (PdfError& e)
    {
        e.AddToCallstack(__FILE__, __LINE__, "Exception caught closing filter's output stream.\n");
        // Closing stream failed, just get rid of it
        m_pOutputStream = NULL;
        throw;
    }
}

void PdfFilter::FailEncodeDecode()
{
    if (m_pOutputStream != NULL) // OC 19.08.2010 BugFix: Sometimes FailEncodeDecode() is called twice
        m_pOutputStream->Close(); // mabri: issue #58 seems fixed without exception safety here ...
    m_pOutputStream = NULL;
}

PdfFilter::~PdfFilter()
{
    // Whoops! Didn't call EndEncode() before destroying the filter!
    // Note that we can't do this for the user, since EndEncode() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    assert(!m_pOutputStream);
}
