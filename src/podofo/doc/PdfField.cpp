/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#include "PdfField.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"

#include "PdfAcroForm.h"
#include "PdfDocument.h"
#include "PdfPainter.h"
#include "PdfPage.h"
#include "PdfStreamedDocument.h"
#include "PdfXObject.h"
#include "PdfSignatureField.h"

#include <sstream>

namespace PoDoFo {

PdfField::PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : m_pObject( pWidget ? pWidget->GetObject() : NULL ), m_pWidget( pWidget ), m_eField( eField )
{
    if (m_pObject == NULL)
        m_pObject = pParent->GetDocument()->GetObjects()->CreateObject();

    Init( pParent );
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : m_eField( eField )
{
    m_pWidget = pPage->CreateAnnotation( ePdfAnnotation_Widget, rRect );
    m_pObject = m_pWidget->GetObject();

    Init( pParent );
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : m_eField( eField )    
{
    m_pWidget = pPage->CreateAnnotation( ePdfAnnotation_Widget, rRect );
    m_pObject = m_pWidget->GetObject();

    Init( pDoc->GetAcroForm() );
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : m_eField( eField )
{
    m_pWidget = pPage->CreateAnnotation( ePdfAnnotation_Widget, rRect );
    m_pObject = m_pWidget->GetObject();

    Init( pDoc->GetAcroForm() );
}

PdfField::PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent, PdfDocument* pDoc)
    : m_pObject( pWidget->GetObject() ), m_pWidget( pWidget ), m_eField( eField )
{
    Init( pParent );
    PdfObject* pFields = pParent->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
    if( pFields && pFields->IsReference())  {
       PdfObject *pRefFld = pDoc->GetObjects()->GetObject(pFields->GetReference());
       if(pRefFld)
         pRefFld->GetArray().push_back( m_pObject->Reference() );
    }
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc, bool bAppearanceNone)
    :  m_eField( eField )
{
   m_pWidget = pPage->CreateAnnotation( ePdfAnnotation_Widget, rRect );
   m_pObject = m_pWidget->GetObject();

   Init( 
	pDoc->GetAcroForm(true, 
			  bAppearanceNone ? 
			  ePdfAcroFormDefaultAppearance_None
			  : ePdfAcroFormDefaultAppearance_BlackText12pt ));
}

PdfField::PdfField( EPdfField eField, PdfObject *pObject, PdfAnnotation *pWidget )
    : m_pObject( pObject ), m_pWidget( pWidget ), m_eField( eField )
{
}

PdfField::PdfField( const PdfField & rhs )
    : m_pObject( NULL ), m_pWidget( NULL ), m_eField( ePdfField_Unknown )
{
    this->operator=( rhs );
}

PdfField * PdfField::CreateField( PdfObject *pObject )
{
    return createField( pObject, NULL );
}

PdfField * PdfField::CreateField( PdfAnnotation *pWidget )
{
    return createField( pWidget->GetObject(), pWidget);
}

PdfField * PdfField::createField( PdfObject *pObject, PdfAnnotation *pWidget )
{
    EPdfField type = GetFieldType( *pObject );
    switch ( type )
    {
    case ePdfField_Unknown:
        return new PdfField( pObject, pWidget );
    case ePdfField_PushButton:
        return new PdfPushButton( pObject, pWidget );
    case ePdfField_CheckBox:
        return new PdfCheckBox( pObject, pWidget );
    case ePdfField_RadioButton:
        return new PdfRadioButton( pObject, pWidget );
    case ePdfField_TextField:
        return new PdfTextField( pObject, pWidget );
    case ePdfField_ComboBox:
        return new PdfComboBox( pObject, pWidget );
    case ePdfField_ListBox:
        return new PdfListBox( pObject, pWidget );
    case ePdfField_Signature:
        return new PdfSignatureField( pObject, pWidget );
    default:
        PODOFO_RAISE_ERROR(ePdfError_InvalidEnumValue);
    }
}

EPdfField PdfField::GetFieldType(const PdfObject & rObject)
{
    EPdfField eField = ePdfField_Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    const PdfObject *pFT = rObject.GetDictionary().FindKeyParent("FT");
    if (!pFT)
        return ePdfField_Unknown;

    const PdfName fieldType = pFT->GetName();
    if (fieldType == PdfName("Btn"))
    {
        pdf_int64 flags;
        PdfField::GetFieldFlags( rObject , flags );

        if ( ( flags & PdfButton::ePdfButton_PushButton ) == PdfButton::ePdfButton_PushButton)
        {
            eField = ePdfField_PushButton;
        }
        else if ( ( flags & PdfButton::ePdfButton_Radio ) == PdfButton::ePdfButton_Radio)
        {
            eField = ePdfField_RadioButton;
        }
        else
        {
            eField = ePdfField_CheckBox;
        }
    }
    else if (fieldType == PdfName("Tx"))
    {
        eField = ePdfField_TextField;
    }
    else if (fieldType == PdfName("Ch"))
    {
        pdf_int64 flags;
        PdfField::GetFieldFlags(rObject, flags);

        if ( ( flags & PdfListField::ePdfListField_Combo ) == PdfListField::ePdfListField_Combo )
        {
            eField = ePdfField_ComboBox;
        }
        else
        {
            eField = ePdfField_ListBox;
        }
    }
    else if (fieldType == PdfName("Sig"))
    {
        eField = ePdfField_Signature;
    }

    return eField;
}

void PdfField::Init( PdfAcroForm* pParent )
{
    // Insert into the parents kids array
    PdfObject* pFields = pParent->GetObject()->GetDictionary().GetKey( PdfName("Fields") );
    if( pFields ) 
    {
        if(!pFields->IsReference() ) 
	{
            pFields->GetArray().push_back( m_pObject->Reference() );
	}
    }
    else
    {
        PODOFO_RAISE_ERROR( ePdfError_NoObject );
    }

    switch( m_eField ) 
    {
        case ePdfField_PushButton:
        case ePdfField_CheckBox:
        case ePdfField_RadioButton:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Btn") );
            break;
        case ePdfField_TextField:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Tx") );
            break;
        case ePdfField_ComboBox:
        case ePdfField_ListBox:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Ch") );
            break;
        case ePdfField_Signature:
            m_pObject->GetDictionary().AddKey( PdfName("FT"), PdfName("Sig") );
            break;


        case ePdfField_Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( ePdfError_InternalLogic );
        }
        break;
    }

    // Create a unique fieldname, because Acrobat Reader crashes if the field has no field name 
    std::ostringstream out;
    PdfLocaleImbue(out);
    out << "podofo_field_" << m_pObject->Reference().ObjectNumber();
}

PdfField::PdfField( PdfObject* pObject, PdfAnnotation* pWidget )
    : m_pObject( pObject ), m_pWidget( pWidget ), m_eField( ePdfField_Unknown )
{
    m_eField = GetFieldType( *pObject );
}

PdfObject* PdfField::GetAppearanceCharacteristics( bool bCreate ) const
{
    PdfObject* pMK = NULL;

    if( !m_pObject->GetDictionary().HasKey( PdfName("MK") ) && bCreate )
    {
        PdfDictionary dictionary;
        const_cast<PdfField*>(this)->m_pObject->GetDictionary().AddKey( PdfName("MK"), dictionary );
    }

    pMK = m_pObject->GetDictionary().GetKey( PdfName("MK") );

    return pMK;
}

void PdfField::SetFieldFlag( long lValue, bool bSet )
{
    pdf_int64 lCur = 0;

    if( m_pObject->GetDictionary().HasKey( PdfName("Ff") ) )
        lCur = m_pObject->GetDictionary().GetKey( PdfName("Ff") )->GetNumber();
    
    if( bSet )
        lCur |= lValue;
    else
    {
        if( (lCur & lValue) == lValue )
            lCur ^= lValue;
    }

    m_pObject->GetDictionary().AddKey( PdfName("Ff"), lCur );
}

bool PdfField::GetFieldFlag( long lValue, bool bDefault ) const
{
    pdf_int64 flag;
    if ( !GetFieldFlags( *m_pObject, flag) )
        return bDefault;

    return ( flag & lValue ) == lValue;
}


bool PdfField::GetFieldFlags( const PdfObject & rObject, pdf_int64 & lValue )
{
    const PdfDictionary &rDict = rObject.GetDictionary();

    const PdfObject *pFlagsObject = rDict.GetKey( "Ff" );
    PdfObject *pParentObect;
    if( pFlagsObject == NULL && ( ( pParentObect = rObject.GetIndirectKey( "Parent" ) ) == NULL
        || ( pFlagsObject = pParentObect->GetDictionary().GetKey( "Ff" ) ) == NULL ) )
    {
        lValue = 0;
        return false;
    }

    lValue = pFlagsObject->GetNumber();
    return true;
}

void PdfField::SetHighlightingMode( EPdfHighlightingMode eMode )
{
    PdfName value;

    switch( eMode ) 
    {
        case ePdfHighlightingMode_None:
            value = PdfName("N");
            break;
        case ePdfHighlightingMode_Invert:
            value = PdfName("I");
            break;
        case ePdfHighlightingMode_InvertOutline:
            value = PdfName("O");
            break;
        case ePdfHighlightingMode_Push:
            value = PdfName("P");
            break;
        case ePdfHighlightingMode_Unknown:
        default:
            PODOFO_RAISE_ERROR( ePdfError_InvalidName );
            break;
    }

    m_pObject->GetDictionary().AddKey( PdfName("H"), value );
}

EPdfHighlightingMode PdfField::GetHighlightingMode() const
{
    EPdfHighlightingMode eMode = ePdfHighlightingMode_Invert;

    if( m_pObject->GetDictionary().HasKey( PdfName("H") ) )
    {
        PdfName value = m_pObject->GetDictionary().GetKey( PdfName("H") )->GetName();
        if( value == PdfName("N") )
            return ePdfHighlightingMode_None;
        else if( value == PdfName("I") )
            return ePdfHighlightingMode_Invert;
        else if( value == PdfName("O") )
            return ePdfHighlightingMode_InvertOutline;
        else if( value == PdfName("P") )
            return ePdfHighlightingMode_Push;
    }

    return eMode;
}

void PdfField::SetBorderColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dGray )
{
    PdfArray array;
    array.push_back( dGray );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dRed, double dGreen, double dBlue )
{
    PdfArray array;
    array.push_back( dRed );
    array.push_back( dGreen );
    array.push_back( dBlue );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dCyan, double dMagenta, double dYellow, double dBlack )
{
    PdfArray array;
    array.push_back( dCyan );
    array.push_back( dMagenta );
    array.push_back( dYellow );
    array.push_back( dBlack );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBackgroundColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dGray )
{
    PdfArray array;
    array.push_back( dGray );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dRed, double dGreen, double dBlue )
{
    PdfArray array;
    array.push_back( dRed );
    array.push_back( dGreen );
    array.push_back( dBlue );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dCyan, double dMagenta, double dYellow, double dBlack )
{
    PdfArray array;
    array.push_back( dCyan );
    array.push_back( dMagenta );
    array.push_back( dYellow );
    array.push_back( dBlack );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetFieldName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("T"), rsName );
}

PdfString PdfField::GetFieldName() const
{
    PdfObject *name = m_pObject->GetDictionary().FindKeyParent( "T" );
    if( !name )
        return PdfString::StringNull;

    return name->GetString();
}

PdfString PdfField::GetFullFieldName(bool escapePartialNames) const
{
    std::string ret;
    PdfObject *object = m_pObject;
    while ( object )
    {
        PdfDictionary &dict = object->GetDictionary();
        PdfObject *nameObj = dict.GetKey( "T" );
        if ( nameObj != NULL )
        {
            std::string name = nameObj->GetString().GetStringUtf8();

            if (escapePartialNames)
            {
                // According to ISO 32000-1:2008, "12.7.3.2 Field Names":
                // "Because the PERIOD is used as a separator for fully
                // qualified names, a partial name shall not contain a
                // PERIOD character."
                // In case the partial name still has periods (effectively
                // violating the standard and Pdf Reference) the fullname
                // would be unintelligible, let's escape them with double
                // dots "..", example "parent.partial..name"
                size_t currpos = 0;
                while ((currpos = name.find('.', currpos)) != std::string::npos)
                {
                    name.replace(currpos, 1, "..");
                    currpos += 2;
                }
            }

            if ( ret.length() == 0 )
                ret = name;
            else
                ret.append(".").append(name);
        }
        object = dict.FindKey( "Parent" );
    }

    return PdfString::FromUtf8String(ret);
}

void PdfField::SetAlternateName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TU"), rsName );
}

PdfString PdfField::GetAlternateName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("TU" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("TU" ) )->GetString();

    return PdfString::StringNull;
}

void PdfField::SetMappingName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TM"), rsName );
}

PdfString PdfField::GetMappingName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("TM" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("TM" ) )->GetString();

    return PdfString::StringNull;
}

void PdfField::AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction ) 
{
    if( !m_pObject->GetDictionary().HasKey( PdfName("AA") ) ) 
        m_pObject->GetDictionary().AddKey( PdfName("AA"), PdfDictionary() );

    PdfObject* pAA = m_pObject->GetDictionary().GetKey( PdfName("AA") );
    pAA->GetDictionary().AddKey( rsName, rAction.GetObject()->Reference() );
}

/////////////////////////////////////////////////////////////////////////////

PdfButton::PdfButton( EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget )
    : PdfField( eField, pObject, pWidget )
{
}

PdfButton::PdfButton( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( eField, pWidget, pParent )
{
}

PdfButton::PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( eField, pPage, rRect, pParent )
{
}

PdfButton::PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( eField, pPage, rRect, pDoc )
{
}

PdfButton::PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfField( eField, pPage, rRect, pDoc )
{
}

PdfButton::PdfButton( const PdfField & rhs )
    : PdfField( rhs )
{
}

void PdfButton::SetCaption( const PdfString & rsText )
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("CA"), rsText );
}

const PdfString PdfButton::GetCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( false );
    
    if( pMK && pMK->GetDictionary().HasKey( PdfName("CA") ) )
        return pMK->GetDictionary().GetKey( PdfName("CA") )->GetString();

    return PdfString::StringNull;
}

/////////////////////////////////////////////////////////////////////////////

PdfPushButton::PdfPushButton( PdfObject* pObject, PdfAnnotation* pWidget )
    : PdfButton( ePdfField_PushButton, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfPushButton::PdfPushButton( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfButton( ePdfField_PushButton, pWidget, pParent )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfButton( ePdfField_PushButton, pPage, rRect, pParent )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfButton( ePdfField_PushButton, pPage, rRect, pDoc )
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfButton( ePdfField_PushButton, pPage, rRect, pDoc )
{
    Init();
}

PdfPushButton::PdfPushButton( const PdfField & rhs )
    : PdfButton( rhs )
{
    if( this->GetType() != ePdfField_CheckBox )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Field cannot be converted into a PdfPushButton" );
    }
}

void PdfPushButton::Init() 
{
    // make a push button
    this->SetFieldFlag( static_cast<int>(ePdfButton_PushButton), true );
    //m_pWidget->SetFlags( 4 );

    /*
    m_pObject->GetDictionary().AddKey( PdfName("H"), PdfName("I") );
    if( !m_pWidget->HasAppearanceStream() )
    {
        // Create the default appearance stream
        PdfRect    rect( 0.0, 0.0, m_pWidget->GetRect().GetWidth(), m_pWidget->GetRect().GetHeight() );
        PdfXObject xObjOff( rect, m_pObject->GetOwner() );
        PdfXObject xObjYes( rect, m_pObject->GetOwner() );
        PdfPainter painter;
        
        painter.SetPage( &xObjOff );
        painter.SetColor( 0.5, 0.5, 0.5 );
        painter.FillRect( 0, xObjOff.GetPageSize().GetHeight(), xObjOff.GetPageSize().GetWidth(), xObjOff.GetPageSize().GetHeight()  );
        painter.FinishPage();

        painter.SetPage( &xObjYes );
        painter.SetColor( 1.0, 0.0, 0.0 );
        painter.FillRect( 0, xObjYes.GetPageSize().GetHeight(), xObjYes.GetPageSize().GetWidth(), xObjYes.GetPageSize().GetHeight()  );
        painter.FinishPage();


        PdfDictionary dict;
        PdfDictionary internal;

        internal.AddKey( "On", xObjYes.GetObject()->Reference() );
        internal.AddKey( "Off", xObjOff.GetObject()->Reference() );
    
        dict.AddKey( "N", internal );

        m_pWidget->GetObject()->GetDictionary().AddKey( "AP", dict );
        m_pWidget->GetObject()->GetDictionary().AddKey( "AS", PdfName("Off") );

        //pWidget->SetAppearanceStream( &xObj );
   }
    */
}

void PdfPushButton::SetRolloverCaption( const PdfString & rsText )
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("RC"), rsText );
}

const PdfString PdfPushButton::GetRolloverCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( false );
    
    if( pMK && pMK->GetDictionary().HasKey( PdfName("RC") ) )
        return pMK->GetDictionary().GetKey( PdfName("RC") )->GetString();

    return PdfString::StringNull;
}

void PdfPushButton::SetAlternateCaption( const PdfString & rsText )
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("AC"), rsText );

}

const PdfString PdfPushButton::GetAlternateCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics( false );
    
    if( pMK && pMK->GetDictionary().HasKey( PdfName("AC") ) )
        return pMK->GetDictionary().GetKey( PdfName("AC") )->GetString();

    return PdfString::StringNull;
}

/////////////////////////////////////////////////////////////////////////////

PdfCheckBox::PdfCheckBox( PdfObject* pObject, PdfAnnotation* pWidget )
    : PdfButton( ePdfField_CheckBox, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfCheckBox::PdfCheckBox( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfButton( ePdfField_CheckBox, pWidget, pParent )
{
}

PdfCheckBox::PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfButton( ePdfField_CheckBox, pPage, rRect, pParent )
{
}

PdfCheckBox::PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfButton( ePdfField_CheckBox, pPage, rRect, pDoc )
{
}

PdfCheckBox::PdfCheckBox( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfButton( ePdfField_CheckBox, pPage, rRect, pDoc )
{
}

PdfCheckBox::PdfCheckBox( const PdfField & rhs )
    : PdfButton( rhs )
{
    if( this->GetType() != ePdfField_CheckBox )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Field cannot be converted into a PdfCheckBox" );
    }
}

void PdfCheckBox::AddAppearanceStream( const PdfName & rName, const PdfReference & rReference )
{
    if( !m_pObject->GetDictionary().HasKey( PdfName("AP") ) )
        m_pObject->GetDictionary().AddKey( PdfName("AP"), PdfDictionary() );

    if( !m_pObject->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().HasKey( PdfName("N") ) )
        m_pObject->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().AddKey( PdfName("N"), PdfDictionary() );

    m_pObject->GetDictionary().GetKey( PdfName("AP") )->
        GetDictionary().GetKey( PdfName("N") )->GetDictionary().AddKey( rName, rReference );
}

void PdfCheckBox::SetAppearanceChecked( const PdfXObject & rXObject )
{
    this->AddAppearanceStream( PdfName("Yes"), rXObject.GetObject()->Reference() );
}

void PdfCheckBox::SetAppearanceUnchecked( const PdfXObject & rXObject )
{
    this->AddAppearanceStream( PdfName("Off"), rXObject.GetObject()->Reference() );
}

void PdfCheckBox::SetChecked( bool bChecked )
{
    m_pObject->GetDictionary().AddKey( PdfName("V"), (bChecked ? PdfName("Yes") : PdfName("Off")) );
    m_pObject->GetDictionary().AddKey( PdfName("AS"), (bChecked ? PdfName("Yes") : PdfName("Off")) );
}

bool PdfCheckBox::IsChecked() const
{
    PdfDictionary dic = m_pObject->GetDictionary();

    if (dic.HasKey(PdfName("V"))) {
        PdfName name = dic.GetKey( PdfName("V") )->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
     } else if (dic.HasKey(PdfName("AS"))) {
        PdfName name = dic.GetKey( PdfName("AS") )->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
     }

    return false;
}

/////////////////////////////////////////////////////////////////////////////

PdfTextField::PdfTextField( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfField( ePdfField_TextField, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfTextField::PdfTextField( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( ePdfField_TextField, pWidget, pParent )
{
    Init();
}

PdfTextField::PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( ePdfField_TextField, pPage, rRect, pParent )
{
    Init();
}

PdfTextField::PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( ePdfField_TextField, pPage, rRect, pDoc )
{
    Init();
}

PdfTextField::PdfTextField( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfField( ePdfField_TextField, pPage, rRect, pDoc )
{
    Init();
}

PdfTextField::PdfTextField( const PdfField & rhs )
    : PdfField( rhs )
{
    if( this->GetType() != ePdfField_TextField )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Field cannot be converted into a PdfTextField" );
    }
}

void PdfTextField::Init()
{
    if( !m_pObject->GetDictionary().HasKey( PdfName("DS") ) )
        m_pObject->GetDictionary().AddKey( PdfName("DS"), PdfString("font: 12pt Helvetica") );
}

void PdfTextField::SetText( const PdfString & rsText )
{
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");

    // if rsText is longer than maxlen, truncate it
    pdf_long nMax = this->GetMaxLen();
    if( nMax != -1 && rsText.GetLength() > nMax )
        m_pObject->GetDictionary().AddKey( key, PdfString( rsText.GetString(), nMax ) );
    else
        m_pObject->GetDictionary().AddKey( key, rsText );
}

PdfString PdfTextField::GetText() const
{
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");
    PdfString str;

    if( m_pObject->GetDictionary().HasKey( key ) )
        str = m_pObject->GetDictionary().GetKey( key )->GetString();

    return str;
}

void PdfTextField::SetMaxLen( pdf_long nMaxLen )
{
    m_pObject->GetDictionary().AddKey( PdfName("MaxLen"), static_cast<pdf_int64>(nMaxLen) );
}

pdf_long  PdfTextField::GetMaxLen() const
{
    return static_cast<pdf_long>(m_pObject->GetDictionary().HasKey( PdfName("MaxLen") ) ? 
                                 m_pObject->GetDictionary().GetKey( PdfName("MaxLen") )->GetNumber() : -1);
}

/////////////////////////////////////////////////////////////////////////////

PdfListField::PdfListField( EPdfField eField, PdfObject *pObject, PdfAnnotation * pWidget )
    : PdfField( eField, pObject, pWidget )
{

}

PdfListField::PdfListField( EPdfField eField, PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfField( eField, pWidget, pParent )
{

}

PdfListField::PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfField( eField, pPage, rRect, pParent )
{

}

PdfListField::PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfField( eField, pPage, rRect, pDoc )
{

}

PdfListField::PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfField( eField, pPage, rRect, pDoc )
{

}

PdfListField::PdfListField( const PdfField & rhs ) 
    : PdfField( rhs )
{
}

void PdfListField::InsertItem( const PdfString & rsValue, const PdfString & rsDisplayName )
{
    PdfVariant var;
    PdfArray   opt;

    if( rsDisplayName == PdfString::StringNull ) 
        var = rsValue;
    else
    {
        PdfArray array;
        array.push_back( rsValue );
        array.push_back( rsDisplayName );

        var = array;
    }

    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();

    // TODO: Sorting
    opt.push_back( var );
    m_pObject->GetDictionary().AddKey( PdfName("Opt"), opt );

    /*
    m_pObject->GetDictionary().AddKey( PdfName("V"), rsValue );

    PdfArray array;
    array.push_back( 0L );
    m_pObject->GetDictionary().AddKey( PdfName("I"), array );
    */
}

void PdfListField::RemoveItem( int nIndex )
{
    PdfArray   opt;

    if( m_pObject->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = m_pObject->GetDictionary().GetKey( PdfName("Opt") )->GetArray();
    
    if( nIndex < 0 || nIndex > static_cast<int>(opt.size()) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    opt.erase( opt.begin() + nIndex );
    m_pObject->GetDictionary().AddKey( PdfName("Opt"), opt );
}

const PdfString PdfListField::GetItem( int nIndex ) const
{
    PdfObject *opt = m_pObject->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return PdfString::StringNull;

    PdfArray &optArray = opt->GetArray();
    if ( nIndex < 0 || nIndex >= static_cast<int>( optArray.size() ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    PdfObject &item = optArray[nIndex];
    if( item.IsArray() )
    {
        PdfArray &itemArray = item.GetArray();
        if( itemArray.size() < 2 ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }
        else
            return itemArray[0].GetString();
    }

    return item.GetString();
}

const PdfString PdfListField::GetItemDisplayText( int nIndex ) const
{
    PdfObject *opt = m_pObject->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return PdfString::StringNull;

    PdfArray &optArray = opt->GetArray();
    if ( nIndex < 0 || nIndex >= static_cast<int>( optArray.size() ) )
    {
        PODOFO_RAISE_ERROR( ePdfError_ValueOutOfRange );
    }

    PdfObject &item = optArray[nIndex];
    if( item.IsArray() )
    {
        PdfArray &itemArray = item.GetArray();
        if( itemArray.size() < 2 ) 
        {
            PODOFO_RAISE_ERROR( ePdfError_InvalidDataType );
        }
        else
            return itemArray[1].GetString();
    }

    return item.GetString();
}

size_t PdfListField::GetItemCount() const
{
    PdfObject *opt = m_pObject->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return 0;
    
    return opt->GetArray().size();
}

void PdfListField::SetSelectedItem( int nIndex )
{
    PdfString selected = this->GetItem( nIndex );

    m_pObject->GetDictionary().AddKey( PdfName("V"), selected );
}

int PdfListField::GetSelectedItem() const
{
    PdfObject *valueObj = m_pObject->GetDictionary().FindKey( "V" ); 
    if( valueObj && !( valueObj->IsString() || valueObj->IsHexString() ) )
        return -1;

    PdfString value = valueObj->GetString();
    PdfObject *opt = m_pObject->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return -1;

    PdfArray &optArray = opt->GetArray();
    for (int i = 0; i < static_cast<int>( optArray.size() ); i++)
    {
        if ( optArray[i].GetString() == value )
            return i;
    }

    return -1;
}


/////////////////////////////////////////////////////////////////////////////

PdfComboBox::PdfComboBox( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfListField( ePdfField_ComboBox, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfComboBox::PdfComboBox( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfListField( ePdfField_ComboBox, pWidget, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfComboBox::PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfListField( ePdfField_ComboBox, pPage, rRect, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfComboBox::PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfListField( ePdfField_ComboBox, pPage, rRect, pDoc )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfComboBox::PdfComboBox( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfListField( ePdfField_ComboBox, pPage, rRect, pDoc )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfComboBox::PdfComboBox( const PdfField & rhs )
    : PdfListField( rhs )
{
    if( this->GetType() != ePdfField_ComboBox )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Field cannot be converted into a PdfTextField" );
    }
}

/////////////////////////////////////////////////////////////////////////////

PdfListBox::PdfListBox( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfListField( ePdfField_ListBox, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfListBox::PdfListBox( PdfAnnotation* pWidget, PdfAcroForm* pParent )
    : PdfListField( ePdfField_ListBox, pWidget, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfListBox::PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfAcroForm* pParent )
    : PdfListField( ePdfField_ListBox, pPage, rRect, pParent )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfListBox::PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc )
    : PdfListField( ePdfField_ListBox, pPage, rRect, pDoc )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfListBox::PdfListBox( PdfPage* pPage, const PdfRect & rRect, PdfStreamedDocument* pDoc )
    : PdfListField( ePdfField_ListBox, pPage, rRect, pDoc )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );        
    m_pWidget->SetBorderStyle( 0.0, 0.0, 1.0 );
}

PdfListBox::PdfListBox( const PdfField & rhs )
    : PdfListField( rhs )
{
    if( this->GetType() != ePdfField_ListBox )
    {
        PODOFO_RAISE_ERROR_INFO( ePdfError_InvalidDataType, "Field cannot be converted into a PdfTextField" );
    }
}

PdfRadioButton::PdfRadioButton( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfButton( ePdfField_RadioButton, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfRadioButton::PdfRadioButton( PdfAnnotation * pWidget, PdfAcroForm * pParent )
    : PdfButton( ePdfField_RadioButton, pWidget, pParent )
{
}

PdfRadioButton::PdfRadioButton( PdfPage * pPage, const PdfRect & rRect, PdfAcroForm * pParent )
    : PdfButton( ePdfField_RadioButton, pPage, rRect, pParent )
{
}

PdfRadioButton::PdfRadioButton( PdfPage * pPage, const PdfRect & rRect, PdfDocument * pDoc )
    : PdfButton( ePdfField_RadioButton, pPage, rRect, pDoc )
{
}

PdfRadioButton::PdfRadioButton( PdfPage * pPage, const PdfRect & rRect, PdfStreamedDocument * pDoc )
    : PdfButton( ePdfField_RadioButton, pPage, rRect, pDoc )
{
}

PdfRadioButton::PdfRadioButton( const PdfField & rhs )
    : PdfButton( rhs )
{
}

};
