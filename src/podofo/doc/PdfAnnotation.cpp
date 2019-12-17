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

#include "PdfAnnotation.h"

#include "base/PdfVecObjects.h"
#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfDate.h"

#include "PdfAction.h"
#include "PdfFileSpec.h"
#include "PdfPage.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"
#include "PdfXObject.h"

namespace PoDoFo {

const long  PdfAnnotation::s_lNumActions = 27;
const char* PdfAnnotation::s_names[] = {
    "Text",                       // - supported
    "Link",
    "FreeText",       // PDF 1.3  // - supported
    "Line",           // PDF 1.3  // - supported
    "Square",         // PDF 1.3
    "Circle",         // PDF 1.3
    "Polygon",        // PDF 1.5
    "PolyLine",       // PDF 1.5
    "Highlight",      // PDF 1.3
    "Underline",      // PDF 1.3
    "Squiggly",       // PDF 1.4
    "StrikeOut",      // PDF 1.3
    "Stamp",          // PDF 1.3
    "Caret",          // PDF 1.5
    "Ink",            // PDF 1.3
    "Popup",          // PDF 1.3
    "FileAttachment", // PDF 1.3
    "Sound",          // PDF 1.2
    "Movie",          // PDF 1.2
    "Widget",         // PDF 1.2  // - supported
    "Screen",         // PDF 1.5
    "PrinterMark",    // PDF 1.4
    "TrapNet",        // PDF 1.3
    "Watermark",      // PDF 1.6
    "3D",             // PDF 1.6
    "RichMedia",      // PDF 1.7 ADBE ExtensionLevel 3 ALX: Petr P. Petrov
    "WebMedia",       // PDF 1.7 IPDF ExtensionLevel 1
    NULL
};

static PdfName GetAppearanceName( EPdfAnnotationAppearance eAppearance );

PdfAnnotation::PdfAnnotation( PdfPage* pPage, EPdfAnnotation eAnnot, const PdfRect & rRect, PdfVecObjects* pParent )
    : PdfElement( "Annot", pParent ), m_eAnnotation( eAnnot ), m_pAction( NULL ), m_pFileSpec( NULL ), m_pPage( pPage )
{
    PdfVariant    rect;
    PdfDate       date;

    const PdfName name( TypeNameForIndex( (int)eAnnot, s_names, s_lNumActions ) );

    if( !name.GetLength() )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    rRect.ToVariant( rect );
    PdfString sDate = date.ToString();
    
    this->GetObject()->GetDictionary().AddKey( PdfName::KeySubtype, name );
    this->GetObject()->GetDictionary().AddKey( PdfName::KeyRect, rect );
    this->GetObject()->GetDictionary().AddKey( "P", pPage->GetObject()->GetIndirectReference() );
    this->GetObject()->GetDictionary().AddKey( "M", sDate );
}

PdfAnnotation::PdfAnnotation( PdfObject* pObject, PdfPage* pPage )
    : PdfElement( "Annot", pObject ), m_eAnnotation( EPdfAnnotation::Unknown ), m_pAction( NULL ), m_pFileSpec( NULL ), m_pPage( pPage )
{
    m_eAnnotation = static_cast<EPdfAnnotation>(TypeNameToIndex( this->GetObject()->GetDictionary().GetKeyAsName( PdfName::KeySubtype ).GetString().c_str(), s_names, s_lNumActions, (int)EPdfAnnotation::Unknown ));
}

PdfAnnotation::~PdfAnnotation()
{
    delete m_pAction;
    delete m_pFileSpec;
}

PdfRect PdfAnnotation::GetRect() const
{
   if( this->GetObject()->GetDictionary().HasKey( PdfName::KeyRect ) )
        return PdfRect( this->GetObject()->GetDictionary().GetKey( PdfName::KeyRect )->GetArray() );

   return PdfRect();
}

void PdfAnnotation::SetRect(const PdfRect & rRect)
{
    PdfVariant rect;
    rRect.ToVariant( rect );
    this->GetObject()->GetDictionary().AddKey( PdfName::KeyRect, rect );
}

void SetAppearanceStreamForObject( PdfObject* pForObject, PdfXObject* xobj, EPdfAnnotationAppearance eAppearance, const PdfName & state )
{
    // Setting an object as appearance stream requires osme resources to be created
    xobj->EnsureResourcesInitialized();

    PdfDictionary dict;
    PdfDictionary internal;
    PdfName name;

    if( !pForObject || !xobj )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if( eAppearance == EPdfAnnotationAppearance::Rollover )
    {
        name = "R";
    }
    else if( eAppearance == EPdfAnnotationAppearance::Down )
    {
        name = "D";
    }
    else // EPdfAnnotationAppearance::Normal
    {
        name = "N";
    }

    if( pForObject->GetDictionary().HasKey( "AP" ) )
    {
        PdfObject* objAP = pForObject->GetDictionary().GetKey( "AP" );
        if( objAP->GetDataType() == EPdfDataType::Reference )
        {
            if( !objAP->GetOwner() )
            {
                PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
            }

            objAP = objAP->GetOwner()->GetObject( objAP->GetReference() );
            if( !objAP )
            {
                PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
            }
        }

        if( objAP->GetDataType() != EPdfDataType::Dictionary )
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
        }

        if( !state.GetLength() )
        {
            // allow overwrite only reference by a reference
            if( objAP->GetDictionary().HasKey( name ) && objAP->GetDictionary().GetKey( name )->GetDataType() != EPdfDataType::Reference )
            {
                PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
            }

            objAP->GetDictionary().AddKey( name, xobj->GetObject()->GetIndirectReference() );
        }
        else
        {
            // when the state is defined, then the appearance is expected to be a dictionary
            if( objAP->GetDictionary().HasKey( name ) && objAP->GetDictionary().GetKey( name )->GetDataType() != EPdfDataType::Dictionary )
            {
                PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
            }

            if( objAP->GetDictionary().HasKey( name ) )
            {
                objAP->GetDictionary().GetKey( name )->GetDictionary().AddKey( state, xobj->GetObject()->GetIndirectReference() );
            }
            else
            {
                internal.AddKey( state, xobj->GetObject()->GetIndirectReference() );
                objAP->GetDictionary().AddKey( name, internal );
            }
        }
    }
    else
    {
        if( !state.GetLength() )
        {
            dict.AddKey( name, xobj->GetObject()->GetIndirectReference() );
            pForObject->GetDictionary().AddKey( "AP", dict );
        }
        else
        {
            internal.AddKey( state, xobj->GetObject()->GetIndirectReference() );
            dict.AddKey( name, internal );
            pForObject->GetDictionary().AddKey( "AP", dict );
        }
    }

    if( state.GetLength() )
    {
        if( !pForObject->GetDictionary().HasKey( "AS" ) )
        {
            pForObject->GetDictionary().AddKey( "AS", state );
        }
    }
}

void PdfAnnotation::SetAppearanceStream( PdfXObject* pObject, EPdfAnnotationAppearance eAppearance, const PdfName & state )
{
    SetAppearanceStreamForObject( this->GetObject(), pObject, eAppearance, state );
}

bool PdfAnnotation::HasAppearanceStream() const
{
    return this->GetObject()->GetDictionary().HasKey( "AP" );
}

PdfObject * PdfAnnotation::GetAppearanceDictionary()
{
    return GetObject()->GetDictionary().FindKey("AP");
}

PdfObject * PdfAnnotation::GetAppearanceStream(EPdfAnnotationAppearance eAppearance, const PdfName & state)
{
    PdfObject *apObj = GetAppearanceDictionary();
    if (apObj == NULL)
        return NULL;

    PdfName apName = GetAppearanceName(eAppearance);
    PdfObject *apObjInn = apObj->GetDictionary().FindKey(apName);
    if (apObjInn == NULL)
        return NULL;

    if (state.GetLength() == 0)
        return apObjInn;

    return apObjInn->GetDictionary().FindKey(state);
}

void PdfAnnotation::SetFlags(EPdfAnnotationFlags uiFlags)
{
    this->GetObject()->GetDictionary().AddKey( "F", PdfVariant( static_cast<int64_t>(uiFlags) ) );
}

EPdfAnnotationFlags PdfAnnotation::GetFlags() const
{
    if( this->GetObject()->GetDictionary().HasKey( "F" ) )
        return static_cast<EPdfAnnotationFlags>(this->GetObject()->GetDictionary().GetKey( "F" )->GetNumber());

    return EPdfAnnotationFlags::None;
}

void PdfAnnotation::SetBorderStyle( double dHCorner, double dVCorner, double dWidth )
{
    this->SetBorderStyle( dHCorner, dVCorner, dWidth, PdfArray() );
}

void PdfAnnotation::SetBorderStyle( double dHCorner, double dVCorner, double dWidth, const PdfArray & rStrokeStyle )
{
    // TODO : Support for Border style for PDF Vers > 1.0
    PdfArray aValues;

    aValues.push_back(dHCorner);
    aValues.push_back(dVCorner);
    aValues.push_back(dWidth);
    if( rStrokeStyle.size() )
        aValues.push_back(rStrokeStyle);

    this->GetObject()->GetDictionary().AddKey( "Border", aValues );
}

void PdfAnnotation::SetTitle( const PdfString & sTitle )
{
    this->GetObject()->GetDictionary().AddKey( "T", sTitle );
}

PdfString PdfAnnotation::GetTitle() const
{
    if( this->GetObject()->GetDictionary().HasKey( "T" ) )
        return this->GetObject()->GetDictionary().GetKey( "T" )->GetString();

    return PdfString();
}

void PdfAnnotation::SetContents( const PdfString & sContents )
{
    this->GetObject()->GetDictionary().AddKey( "Contents", sContents );
}

PdfString PdfAnnotation::GetContents() const
{
    if( this->GetObject()->GetDictionary().HasKey( "Contents" ) )
        return this->GetObject()->GetDictionary().GetKey( "Contents" )->GetString();

    return PdfString();
}

void PdfAnnotation::SetDestination( const PdfDestination & rDestination )
{
    rDestination.AddToDictionary( this->GetObject()->GetDictionary() );
}

PdfDestination PdfAnnotation::GetDestination( PdfDocument* pDoc ) const
{
    return PdfDestination(const_cast<PdfAnnotation &>(*this).GetObject()->GetDictionary().GetKey( "Dest" ), pDoc );
}

bool PdfAnnotation::HasDestination() const
{
    return this->GetObject()->GetDictionary().HasKey( "Dest" );
}

void PdfAnnotation::SetAction( const PdfAction & rAction )
{
    if( m_pAction )
        delete m_pAction;

    m_pAction = new PdfAction( rAction );
    this->GetObject()->GetDictionary().AddKey( "A", m_pAction->GetObject()->GetIndirectReference() );
}

PdfAction* PdfAnnotation::GetAction() const
{
    if( !m_pAction && HasAction() )
        const_cast<PdfAnnotation*>(this)->m_pAction = new PdfAction( this->GetObject()->GetIndirectKey( "A" ) );

    return m_pAction;
}

bool PdfAnnotation::HasAction() const
{
    return this->GetObject()->GetDictionary().HasKey( "A" );
}

void PdfAnnotation::SetOpen( bool b )
{
    this->GetObject()->GetDictionary().AddKey( "Open", b );
}

bool PdfAnnotation::GetOpen() const
{
    if( this->GetObject()->GetDictionary().HasKey( "Open" ) )
        return this->GetObject()->GetDictionary().GetKey( "Open" )->GetBool();

    return false;
}

bool PdfAnnotation::HasFileAttachement() const
{
    return this->GetObject()->GetDictionary().HasKey( "FS" );
}

void PdfAnnotation::SetFileAttachement( const PdfFileSpec & rFileSpec )
{
    if( m_pFileSpec )
        delete m_pFileSpec;

    m_pFileSpec = new PdfFileSpec( rFileSpec );
    this->GetObject()->GetDictionary().AddKey( "FS", m_pFileSpec->GetObject()->GetIndirectReference() );
}

PdfFileSpec* PdfAnnotation::GetFileAttachement() const
{
    if( !m_pFileSpec && HasFileAttachement() )
        const_cast<PdfAnnotation*>(this)->m_pFileSpec = new PdfFileSpec( this->GetObject()->GetIndirectKey( "FS" ) );

    return m_pFileSpec;
}

PdfArray PdfAnnotation::GetQuadPoints() const
{
    if( this->GetObject()->GetDictionary().HasKey( "QuadPoints" ) )
        return PdfArray( this->GetObject()->GetDictionary().GetKey( "QuadPoints" )->GetArray() );

    return PdfArray();
}

void PdfAnnotation::SetQuadPoints( const PdfArray & rQuadPoints )
{
    if ( m_eAnnotation != EPdfAnnotation::Highlight &&
         m_eAnnotation != EPdfAnnotation::Underline &&
	 m_eAnnotation != EPdfAnnotation::Squiggly  &&
	 m_eAnnotation != EPdfAnnotation::StrikeOut )
        PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "Must be a text markup annotation (highlight, underline, squiggly or strikeout) to set quad points" );

    this->GetObject()->GetDictionary().AddKey( "QuadPoints", rQuadPoints );
}

PdfArray PdfAnnotation::GetColor() const
{
    if( this->GetObject()->GetDictionary().HasKey( "C" ) )
        return PdfArray( this->GetObject()->GetDictionary().GetKey( "C" )->GetArray() );
    return PdfArray();
}

void PdfAnnotation::SetColor( double r, double g, double b )
{
    PdfArray c;
    c.push_back( PdfVariant( r ) );
    c.push_back( PdfVariant( g ) );
    c.push_back( PdfVariant( b ) );
    this->GetObject()->GetDictionary().AddKey( "C", c );
}
void PdfAnnotation::SetColor( double C, double M, double Y, double K ) 
{
    PdfArray c;
    c.push_back( PdfVariant( C ) );
    c.push_back( PdfVariant( M ) );
    c.push_back( PdfVariant( Y ) );
    c.push_back( PdfVariant( K ) );
    this->GetObject()->GetDictionary().AddKey( "C", c );
}

void PdfAnnotation::SetColor( double gray ) 
{
    PdfArray c;
    c.push_back( PdfVariant( gray ) );
    this->GetObject()->GetDictionary().AddKey( "C", c );
}

void PdfAnnotation::SetColor() 
{
    PdfArray c;
    this->GetObject()->GetDictionary().AddKey( "C", c );
}

PdfName GetAppearanceName( EPdfAnnotationAppearance eAppearance )
{
    switch ( eAppearance )
    {
        case PoDoFo::EPdfAnnotationAppearance::Normal:
            return PdfName( "N" );
        case PoDoFo::EPdfAnnotationAppearance::Rollover:
            return PdfName( "R" );
        case PoDoFo::EPdfAnnotationAppearance::Down:
            return PdfName( "D" );
        default:
            PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "Invalid appearance type" );
    }
}
};
