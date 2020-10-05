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

using namespace std;
using namespace PoDoFo;

void getFullName(const PdfObject* obj, bool escapePartialNames, string &fullname);

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect)
    : m_eField( eField )    
{
    m_pWidget = pPage->CreateAnnotation( EPdfAnnotation::Widget, rRect );
    m_pObject = m_pWidget->GetObject();
    Init(pPage->GetDocument().GetAcroForm());
}

PdfField::PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfDocument &pDoc, bool insertInAcroform)
    : m_eField(eField), m_pObject(pWidget ? pWidget->GetObject() : nullptr), m_pWidget( pWidget )
{
    auto parent = pDoc.GetAcroForm();
    if (m_pObject == nullptr)
        m_pObject = parent->GetDocument()->GetObjects().CreateObject();

    Init(insertInAcroform ? parent : nullptr);
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc, bool bAppearanceNone)
    :  m_eField( eField )
{
   m_pWidget = pPage->CreateAnnotation( EPdfAnnotation::Widget, rRect );
   m_pObject = m_pWidget->GetObject();

   Init(pDoc->GetAcroForm(true,
			  bAppearanceNone ? 
			  EPdfAcroFormDefaulAppearance::None
			  : EPdfAcroFormDefaulAppearance::BlackText12pt ));
}

PdfField::PdfField( EPdfField eField, PdfObject *pObject, PdfAnnotation *pWidget )
    : m_pObject( pObject ), m_pWidget( pWidget ), m_eField( eField )
{
}

PdfField::PdfField( const PdfField & rhs )
    : m_pObject( NULL ), m_pWidget( NULL ), m_eField( EPdfField::Unknown )
{
    this->operator=( rhs );
}

PdfField * PdfField::CreateField( PdfObject *pObject )
{
    return createField(GetFieldType(*pObject), pObject, NULL );
}

PdfField * PdfField::CreateField( PdfAnnotation *pWidget )
{
    PdfObject *pObject = pWidget->GetObject();
    return createField(GetFieldType(*pObject), pObject, pWidget);
}

PdfField * PdfField::CreateChildField()
{
    return createChildField(nullptr, PdfRect());
}

PdfField * PdfField::CreateChildField(PdfPage &page, const PdfRect &rect)
{
    return createChildField(&page, rect);
}

PdfField * PdfField::createChildField(PdfPage *page, const PdfRect &rect)
{
    EPdfField type = GetType();
    auto doc = m_pObject->GetDocument();
    PdfField *field;
    PdfObject *childObj;
    if (page == nullptr)
    {
        childObj = doc->GetObjects().CreateObject();
        field = createField(type, childObj, nullptr);
    }
    else
    {
        PdfAnnotation *annot = page->CreateAnnotation(EPdfAnnotation::Widget, rect);
        childObj = annot->GetObject();
        field = createField(type, childObj, annot);
    }

    auto &dict = m_pObject->GetDictionary();
    auto kids = dict.FindKey("Kids");
    if (kids == nullptr)
        kids = &dict.AddKey("Kids", PdfArray());

    auto &arr = kids->GetArray();
    arr.push_back(childObj->GetIndirectReference());
    childObj->GetDictionary().AddKey("Parent", m_pObject->GetIndirectReference());
    return field;
}

PdfField * PdfField::createField(EPdfField type, PdfObject *pObject, PdfAnnotation *pWidget )
{
    switch ( type )
    {
    case EPdfField::Unknown:
        return new PdfField( pObject, pWidget );
    case EPdfField::PushButton:
        return new PdfPushButton( pObject, pWidget );
    case EPdfField::CheckBox:
        return new PdfCheckBox( pObject, pWidget );
    case EPdfField::RadioButton:
        return new PdfRadioButton( pObject, pWidget );
    case EPdfField::TextField:
        return new PdfTextField( pObject, pWidget );
    case EPdfField::ComboBox:
        return new PdfComboBox( pObject, pWidget );
    case EPdfField::ListBox:
        return new PdfListBox( pObject, pWidget );
    case EPdfField::Signature:
        return new PdfSignatureField( pObject, pWidget );
    default:
        PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }
}

EPdfField PdfField::GetFieldType(const PdfObject & rObject)
{
    EPdfField eField = EPdfField::Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    const PdfObject *pFT = rObject.GetDictionary().FindKeyParent("FT");
    if (!pFT)
        return EPdfField::Unknown;

    const PdfName fieldType = pFT->GetName();
    if (fieldType == PdfName("Btn"))
    {
        int64_t flags;
        PdfField::GetFieldFlags( rObject , flags );

        if ( ( flags & PdfButton::ePdfButton_PushButton ) == PdfButton::ePdfButton_PushButton)
        {
            eField = EPdfField::PushButton;
        }
        else if ( ( flags & PdfButton::ePdfButton_Radio ) == PdfButton::ePdfButton_Radio)
        {
            eField = EPdfField::RadioButton;
        }
        else
        {
            eField = EPdfField::CheckBox;
        }
    }
    else if (fieldType == PdfName("Tx"))
    {
        eField = EPdfField::TextField;
    }
    else if (fieldType == PdfName("Ch"))
    {
        int64_t flags;
        PdfField::GetFieldFlags(rObject, flags);

        if ( ( flags & PdfListField::ePdfListField_Combo ) == PdfListField::ePdfListField_Combo )
        {
            eField = EPdfField::ComboBox;
        }
        else
        {
            eField = EPdfField::ListBox;
        }
    }
    else if (fieldType == PdfName("Sig"))
    {
        eField = EPdfField::Signature;
    }

    return eField;
}

void PdfField::Init(PdfAcroForm *pParent)
{
    if (pParent != nullptr)
    {
        // Insert into the parents kids array
        PdfArray& fields = pParent->GetFieldsArray();
        fields.push_back(m_pObject->GetIndirectReference());
    }

    PdfDictionary &dict = m_pObject->GetDictionary();
    switch( m_eField ) 
    {
        case EPdfField::CheckBox:
            dict.AddKey(PdfName("FT"), PdfName("Btn"));
            break;
        case EPdfField::PushButton:
            dict.AddKey(PdfName("FT"), PdfName("Btn"));
            dict.AddKey("Ff", PdfObject((int64_t)PdfButton::ePdfButton_PushButton));
            break;
        case EPdfField::RadioButton:
            dict.AddKey( PdfName("FT"), PdfName("Btn") );
            dict.AddKey("Ff", PdfObject((int64_t)(PdfButton::ePdfButton_Radio | PdfButton::ePdfButton_NoToggleOff)));
            break;
        case EPdfField::TextField:
            dict.AddKey( PdfName("FT"), PdfName("Tx") );
            break;
        case EPdfField::ListBox:
            dict.AddKey(PdfName("FT"), PdfName("Ch"));
            break;
        case EPdfField::ComboBox:
            dict.AddKey(PdfName("FT"), PdfName("Ch"));
            dict.AddKey("Ff", PdfObject((int64_t)PdfListField::ePdfListField_Combo));
            break;
        case EPdfField::Signature:
            dict.AddKey( PdfName("FT"), PdfName("Sig") );
            break;

        case EPdfField::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
        break;
    }
}

PdfField::PdfField( PdfObject* pObject, PdfAnnotation* pWidget )
    : m_pObject( pObject ), m_pWidget( pWidget ), m_eField( EPdfField::Unknown )
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

void PdfField::AssertTerminalField() const
{
    auto& dict = GetDictionary();
    if (dict.HasKey("Kids"))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "This method can be called only on terminal field. Ensure this field has "
            "not been retrieved from AcroFormFields collection or it's not a parent of terminal fields");
}

void PdfField::SetFieldFlag( long lValue, bool bSet )
{
    int64_t lCur = 0;

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
    int64_t flag;
    if ( !GetFieldFlags( *m_pObject, flag) )
        return bDefault;

    return ( flag & lValue ) == lValue;
}


bool PdfField::GetFieldFlags( const PdfObject & rObject, int64_t & lValue )
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
        case EPdfHighlightingMode::None:
            value = PdfName("N");
            break;
        case EPdfHighlightingMode::Invert:
            value = PdfName("I");
            break;
        case EPdfHighlightingMode::InvertOutline:
            value = PdfName("O");
            break;
        case EPdfHighlightingMode::Push:
            value = PdfName("P");
            break;
        case EPdfHighlightingMode::Unknown:
        default:
            PODOFO_RAISE_ERROR( EPdfError::InvalidName );
            break;
    }

    m_pObject->GetDictionary().AddKey( PdfName("H"), value );
}

EPdfHighlightingMode PdfField::GetHighlightingMode() const
{
    EPdfHighlightingMode eMode = EPdfHighlightingMode::Invert;

    if( m_pObject->GetDictionary().HasKey( PdfName("H") ) )
    {
        PdfName value = m_pObject->GetDictionary().GetKey( PdfName("H") )->GetName();
        if( value == PdfName("N") )
            return EPdfHighlightingMode::None;
        else if( value == PdfName("I") )
            return EPdfHighlightingMode::Invert;
        else if( value == PdfName("O") )
            return EPdfHighlightingMode::InvertOutline;
        else if( value == PdfName("P") )
            return EPdfHighlightingMode::Push;
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

void PdfField::SetName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("T"), rsName );
}

PdfString PdfField::GetName() const
{
    PdfObject *name = m_pObject->GetDictionary().FindKeyParent( "T" );
    if( !name )
        return PdfString::StringNull;

    return name->GetString();
}

PdfString PdfField::GetNameRaw() const
{
    PdfObject *name = m_pObject->GetDictionary().GetKey("T");
    if (!name)
        return PdfString::StringNull;

    return name->GetString();
}

PdfString PdfField::GetFullName(bool escapePartialNames) const
{
    string fullName;
    getFullName(m_pObject, escapePartialNames, fullName);
    return PdfString::FromUtf8String(fullName);
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
    pAA->GetDictionary().AddKey( rsName, rAction.GetObject()->GetIndirectReference() );
}

void PdfField::SetReadOnly(bool bReadOnly)
{
    this->SetFieldFlag(static_cast<int>(EPdfFieldFlags::ReadOnly), bReadOnly);
}

bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag(static_cast<int>(EPdfFieldFlags::ReadOnly), false);
}

void PdfField::SetRequired(bool bRequired)
{
    this->SetFieldFlag(static_cast<int>(EPdfFieldFlags::Required), bRequired);
}

bool PdfField::IsRequired() const
{
    return this->GetFieldFlag(static_cast<int>(EPdfFieldFlags::Required), false);
}

void PdfField::SetNoExport(bool bExport)
{
    this->SetFieldFlag(static_cast<int>(EPdfFieldFlags::NoExport), bExport);
}

bool PdfField::IsNoExport() const
{
    return this->GetFieldFlag(static_cast<int>(EPdfFieldFlags::NoExport), false);
}

PdfPage* PdfField::GetPage() const
{
    return m_pWidget->GetPage();
}

void PdfField::SetMouseEnterAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("E"), rAction);
}

void PdfField::SetMouseLeaveAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("X"), rAction);
}

void PdfField::SetMouseDownAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("D"), rAction);
}

void PdfField::SetMouseUpAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("U"), rAction);
}

void PdfField::SetFocusEnterAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("Fo"), rAction);
}

void PdfField::SetFocusLeaveAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("BI"), rAction);
}

void PdfField::SetPageOpenAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PO"), rAction);
}

void PdfField::SetPageCloseAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PC"), rAction);
}

void PdfField::SetPageVisibleAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PV"), rAction);
}

void PdfField::SetPageInvisibleAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PI"), rAction);
}

void PdfField::SetKeystrokeAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("K"), rAction);
}

void PdfField::SetValidateAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("V"), rAction);
}

EPdfField PdfField::GetType() const
{
    return m_eField;
}

PdfAnnotation* PdfField::GetWidgetAnnotation() const
{
    return m_pWidget;
}

PdfObject* PdfField::GetFieldObject() const
{
    return m_pObject;
}

PdfDictionary& PdfField::GetDictionary()
{
    return m_pObject->GetDictionary();
}

const PdfDictionary& PdfField::GetDictionary() const
{
    return m_pObject->GetDictionary();
}

PdfButton::PdfButton(EPdfField eField, PdfAnnotation* pWidget, PdfDocument& doc, bool insertInAcrofrom)
    : PdfField(eField, pWidget, doc, insertInAcrofrom)
{
}

PdfButton::PdfButton( EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget )
    : PdfField( eField, pObject, pWidget )
{
}

PdfButton::PdfButton( EPdfField eField, PdfPage* pPage, const PdfRect & rRect)
    : PdfField( eField, pPage, rRect )
{
}

PdfButton::PdfButton( const PdfField & rhs )
    : PdfField( rhs )
{
}

bool PdfButton::IsPushButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false);
}

bool PdfButton::IsCheckBox() const
{
    return (!this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false) &&
        !this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false));
}

bool PdfButton::IsRadioButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false);
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

PdfPushButton::PdfPushButton( PdfObject* pObject, PdfAnnotation* pWidget )
    : PdfButton( EPdfField::PushButton, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfPushButton::PdfPushButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfButton(EPdfField::PushButton, pWidget, pDoc, insertInAcroform)
{
    Init();
}

PdfPushButton::PdfPushButton( PdfPage* pPage, const PdfRect & rRect)
    : PdfButton( EPdfField::PushButton, pPage, rRect )
{
    Init();
}

PdfPushButton::PdfPushButton( const PdfField & rhs )
    : PdfButton( rhs )
{
    if( this->GetType() != EPdfField::CheckBox )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Field cannot be converted into a PdfPushButton" );
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
        painter.FillRect( 0, xObjOff.GetSize().GetHeight(), xObjOff.GetSize().GetWidth(), xObjOff.GetSize().GetHeight()  );
        painter.FinishPage();

        painter.SetPage( &xObjYes );
        painter.SetColor( 1.0, 0.0, 0.0 );
        painter.FillRect( 0, xObjYes.GetSize().GetHeight(), xObjYes.GetSize().GetWidth(), xObjYes.GetSize().GetHeight()  );
        painter.FinishPage();


        PdfDictionary dict;
        PdfDictionary internal;

        internal.AddKey( "On", xObjYes.GetObject()->GetIndirectReference() );
        internal.AddKey( "Off", xObjOff.GetObject()->GetIndirectReference() );
    
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

PdfCheckBox::PdfCheckBox( PdfObject* pObject, PdfAnnotation* pWidget )
    : PdfButton( EPdfField::CheckBox, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfCheckBox::PdfCheckBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfButton(EPdfField::CheckBox, pWidget, pDoc, insertInAcroform)
{
}

PdfCheckBox::PdfCheckBox( PdfPage* pPage, const PdfRect & rRect)
    : PdfButton( EPdfField::CheckBox, pPage, rRect )
{
}

PdfCheckBox::PdfCheckBox( const PdfField & rhs )
    : PdfButton( rhs )
{
    if( this->GetType() != EPdfField::CheckBox )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Field cannot be converted into a PdfCheckBox" );
    }
}

void PdfCheckBox::AddAppearanceStream( const PdfName & rName, const PdfReference & rReference )
{
    if( !GetFieldObject()->GetDictionary().HasKey( PdfName("AP") ) )
        GetFieldObject()->GetDictionary().AddKey( PdfName("AP"), PdfDictionary() );

    if( !GetFieldObject()->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().HasKey( PdfName("N") ) )
        GetFieldObject()->GetDictionary().GetKey( PdfName("AP") )->GetDictionary().AddKey( PdfName("N"), PdfDictionary() );

    GetFieldObject()->GetDictionary().GetKey( PdfName("AP") )->
        GetDictionary().GetKey( PdfName("N") )->GetDictionary().AddKey( rName, rReference );
}

void PdfCheckBox::SetAppearanceChecked( const PdfXObject & rXObject )
{
    this->AddAppearanceStream( PdfName("Yes"), rXObject.GetObject()->GetIndirectReference() );
}

void PdfCheckBox::SetAppearanceUnchecked( const PdfXObject & rXObject )
{
    this->AddAppearanceStream( PdfName("Off"), rXObject.GetObject()->GetIndirectReference() );
}

void PdfCheckBox::SetChecked( bool bChecked )
{
    GetFieldObject()->GetDictionary().AddKey( PdfName("V"), (bChecked ? PdfName("Yes") : PdfName("Off")) );
    GetFieldObject()->GetDictionary().AddKey( PdfName("AS"), (bChecked ? PdfName("Yes") : PdfName("Off")) );
}

bool PdfCheckBox::IsChecked() const
{
    PdfDictionary dic = GetFieldObject()->GetDictionary();

    if (dic.HasKey(PdfName("V"))) {
        PdfName name = dic.GetKey( PdfName("V") )->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
     } else if (dic.HasKey(PdfName("AS"))) {
        PdfName name = dic.GetKey( PdfName("AS") )->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
     }

    return false;
}

PdfTextField::PdfTextField( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfField( EPdfField::TextField, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfTextField::PdfTextField(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfField(EPdfField::TextField, pWidget, pDoc, insertInAcroform)
{
    Init();
}

PdfTextField::PdfTextField( PdfPage* pPage, const PdfRect & rRect)
    : PdfField( EPdfField::TextField, pPage, rRect )
{
    Init();
}

PdfTextField::PdfTextField( const PdfField & rhs )
    : PdfField( rhs )
{
    if( this->GetType() != EPdfField::TextField )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Field cannot be converted into a PdfTextField" );
    }
}

void PdfTextField::Init()
{
    if( !GetFieldObject()->GetDictionary().HasKey( PdfName("DS") ) )
        GetFieldObject()->GetDictionary().AddKey( PdfName("DS"), PdfString("font: 12pt Helvetica") );
}

void PdfTextField::SetText( const PdfString & rsText )
{
    AssertTerminalField();
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");

    // if rsText is longer than maxlen, truncate it
    int64_t nMax = this->GetMaxLen();
    if( nMax != -1 && rsText.GetLength() > (size_t)nMax )
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Unable to set text larger MaxLen");

    GetFieldObject()->GetDictionary().AddKey( key, rsText );
}

PdfString PdfTextField::GetText() const
{
    AssertTerminalField();
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");
    PdfString str;

    auto found = GetFieldObject()->GetDictionary().FindKeyParent(key);
    if (found == nullptr)
        return str;

    return found->GetString();
}

void PdfTextField::SetMaxLen(int64_t nMaxLen )
{
    GetFieldObject()->GetDictionary().AddKey( PdfName("MaxLen"), nMaxLen);
}

int64_t PdfTextField::GetMaxLen() const
{
    auto found = GetFieldObject()->GetDictionary().FindKeyParent("MaxLen");
    if (found == nullptr)
        return -1;

    return found->GetNumber();
}

void PdfTextField::SetMultiLine(bool bMultiLine)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_MultiLine), bMultiLine);
}

bool PdfTextField::IsMultiLine() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_MultiLine), false);
}

void PdfTextField::SetPasswordField(bool bPassword)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_Password), bPassword);
}

bool PdfTextField::IsPasswordField() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_Password), false);
}

void PdfTextField::SetFileField(bool bFile)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_FileSelect), bFile);
}

bool PdfTextField::IsFileField() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_FileSelect), false);
}

void PdfTextField::SetSpellcheckingEnabled(bool bSpellcheck)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_NoSpellcheck), !bSpellcheck);
}

bool PdfTextField::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_NoSpellcheck), true);
}

void PdfTextField::SetScrollBarsEnabled(bool bScroll)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_NoScroll), !bScroll);
}

bool PdfTextField::IsScrollBarsEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_NoScroll), true);
}

void PdfTextField::SetCombs(bool bCombs)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_Comb), bCombs);
}

bool PdfTextField::IsCombs() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_Comb), false);
}

void PdfTextField::SetRichText(bool bRichText)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_RichText), bRichText);
}

bool PdfTextField::IsRichText() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_RichText), false);
}

PdfListField::PdfListField(EPdfField eField, PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfField(eField, pWidget, pDoc, insertInAcroform)
{
}

PdfListField::PdfListField( EPdfField eField, PdfObject *pObject, PdfAnnotation * pWidget )
    : PdfField( eField, pObject, pWidget )
{
}

PdfListField::PdfListField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect )
    : PdfField( eField, pPage, rRect )
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

    if(GetFieldObject()->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = GetFieldObject()->GetDictionary().GetKey( PdfName("Opt") )->GetArray();

    // TODO: Sorting
    opt.push_back( var );
    GetFieldObject()->GetDictionary().AddKey( PdfName("Opt"), opt );

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

    if(GetFieldObject()->GetDictionary().HasKey( PdfName("Opt") ) )
        opt = GetFieldObject()->GetDictionary().GetKey( PdfName("Opt") )->GetArray();
    
    if( nIndex < 0 || nIndex > static_cast<int>(opt.size()) )
    {
        PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );
    }

    opt.erase( opt.begin() + nIndex );
    GetFieldObject()->GetDictionary().AddKey( PdfName("Opt"), opt );
}

const PdfString PdfListField::GetItem( int nIndex ) const
{
    PdfObject *opt = GetFieldObject()->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return PdfString::StringNull;

    PdfArray &optArray = opt->GetArray();
    if (nIndex < 0 || nIndex >= optArray.GetSize())
    {
        PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );
    }

    PdfObject &item = optArray[nIndex];
    if( item.IsArray() )
    {
        PdfArray &itemArray = item.GetArray();
        if( itemArray.size() < 2 ) 
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
        }
        else
            return itemArray.FindAt(0).GetString();
    }

    return item.GetString();
}

const PdfString PdfListField::GetItemDisplayText( int nIndex ) const
{
    PdfObject *opt = GetFieldObject()->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return PdfString::StringNull;

    PdfArray &optArray = opt->GetArray();
    if ( nIndex < 0 || nIndex >= static_cast<int>( optArray.size() ) )
    {
        PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );
    }

    PdfObject &item = optArray[nIndex];
    if( item.IsArray() )
    {
        PdfArray &itemArray = item.GetArray();
        if( itemArray.size() < 2 ) 
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
        }
        else
            return itemArray.FindAt(1).GetString();
    }

    return item.GetString();
}

size_t PdfListField::GetItemCount() const
{
    PdfObject *opt = GetFieldObject()->GetDictionary().FindKey( "Opt" );
    if ( opt == NULL )
        return 0;
    
    return opt->GetArray().size();
}

void PdfListField::SetSelectedIndex( int nIndex )
{
    AssertTerminalField();
    PdfString selected = this->GetItem( nIndex );
    GetFieldObject()->GetDictionary().AddKey( "V", selected );
}

int PdfListField::GetSelectedIndex() const
{
    AssertTerminalField();
    PdfObject *valueObj = GetFieldObject()->GetDictionary().FindKey( "V" );
    if (valueObj == nullptr || !valueObj->IsString())
        return -1;

    PdfString value = valueObj->GetString();
    PdfObject *opt = GetFieldObject()->GetDictionary().FindKey( "Opt" );
    if ( opt == nullptr)
        return -1;

    PdfArray &optArray = opt->GetArray();
    for (int i = 0; i < optArray.GetSize(); i++)
    {
        auto& found = optArray.FindAt(i);
        if (found.IsString())
        {
            if (found.GetString() == value)
                return i;
        }
        else if (found.IsArray())
        {
            auto& arr = found.GetArray();
            if (arr.FindAt(0).GetString() == value)
                return i;
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Choice field item has invaid data type");
        }
    }

    return -1;
}

bool PdfListField::IsComboBox() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

void PdfListField::SetSpellcheckingEnabled(bool bSpellcheck)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_NoSpellcheck), !bSpellcheck);
}

bool PdfListField::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_NoSpellcheck), true);
}

void PdfListField::SetSorted(bool bSorted)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Sort), bSorted);
}

bool PdfListField::IsSorted() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Sort), false);
}

void PdfListField::SetMultiSelect(bool bMulti)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_MultiSelect), bMulti);
}

bool PdfListField::IsMultiSelect() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_MultiSelect), false);
}

void PdfListField::SetCommitOnSelectionChange(bool bCommit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_CommitOnSelChange), bCommit);
}

bool PdfListField::IsCommitOnSelectionChange() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_CommitOnSelChange), false);
}

PdfComboBox::PdfComboBox( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfListField( EPdfField::ComboBox, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfComboBox::PdfComboBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfListField(EPdfField::ComboBox, pWidget, pDoc, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

PdfComboBox::PdfComboBox( PdfPage* pPage, const PdfRect & rRect)
    : PdfListField( EPdfField::ComboBox, pPage, rRect )
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), true );        
}

PdfComboBox::PdfComboBox( const PdfField & rhs )
    : PdfListField( rhs )
{
    if( this->GetType() != EPdfField::ComboBox )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Field cannot be converted into a PdfTextField" );
    }
}

void PdfComboBox::SetEditable(bool bEdit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Edit), bEdit);
}

bool PdfComboBox::IsEditable() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Edit), false);
}

PdfListBox::PdfListBox( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfListField( EPdfField::ListBox, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfListBox::PdfListBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfListField(EPdfField::ListBox, pWidget, pDoc, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox( PdfPage* pPage, const PdfRect & rRect)
    : PdfListField( EPdfField::ListBox, pPage, rRect)
{
    this->SetFieldFlag( static_cast<int>(ePdfListField_Combo), false );
}

PdfListBox::PdfListBox( const PdfField & rhs )
    : PdfListField( rhs )
{
    if( this->GetType() != EPdfField::ListBox )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Field cannot be converted into a PdfTextField" );
    }
}

PdfRadioButton::PdfRadioButton( PdfObject *pObject, PdfAnnotation* pWidget )
    : PdfButton( EPdfField::RadioButton, pObject, pWidget )
{
    // NOTE: We assume initialization was performed in the given object
}

PdfRadioButton::PdfRadioButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfButton(EPdfField::RadioButton, pWidget, pDoc, insertInAcroform)
{
}

PdfRadioButton::PdfRadioButton( PdfPage * pPage, const PdfRect & rRect)
    : PdfButton( EPdfField::RadioButton, pPage, rRect)
{
}

PdfRadioButton::PdfRadioButton( const PdfField & rhs )
    : PdfButton( rhs )
{
}

void getFullName(const PdfObject* obj, bool escapePartialNames, string& fullname)
{
    const PdfDictionary& dict = obj->GetDictionary();
    auto parent = dict.FindKey("Parent");;
    if (parent != nullptr)
        getFullName(parent, escapePartialNames, fullname);

    const PdfObject* nameObj = dict.GetKey("T");
    if (nameObj != NULL)
    {
        string name = nameObj->GetString().GetStringUtf8();

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

        if (fullname.length() == 0)
            fullname = name;
        else
            fullname.append(".").append(name);
    }
}

