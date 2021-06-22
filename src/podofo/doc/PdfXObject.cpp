/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfXObject.h" 

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfRect.h"
#include "base/PdfVariant.h"

#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfDocument.h"

#include <sstream>

#define PI 3.141592654

using namespace std;
using namespace PoDoFo;

PdfXObject::PdfXObject( const PdfRect & rRect, PdfDocument* pParent, const char* pszPrefix, bool bWithoutIdentifier )
    : PdfElement(*pParent, "XObject"), m_rRect( rRect ), m_pResources( nullptr )
{
    InitXObject( rRect, pszPrefix );
    if( bWithoutIdentifier )
    {
       m_Identifier = PdfName(pszPrefix);
    }
}

PdfXObject::PdfXObject( const PdfRect & rRect, PdfVecObjects* pParent, const char* pszPrefix )
    : PdfElement(*pParent, "XObject"), m_rRect( rRect ), m_pResources( nullptr )
{
    InitXObject( rRect, pszPrefix );
}

PdfXObject::PdfXObject( const PdfDocument & rDoc, int nPage, PdfDocument* pParent, const char* pszPrefix, bool bUseTrimBox )
    : PdfElement(*pParent, "XObject"), m_pResources( nullptr )
{
    InitXObject( m_rRect, pszPrefix );

    // Implementation note: source document must be different from distination
    if ( pParent == reinterpret_cast<const PdfDocument*>(&rDoc) )
    {
        PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
    }

    // After filling set correct BBox, independent of rotation
    m_rRect = pParent->FillXObjectFromDocumentPage( this, rDoc, nPage, bUseTrimBox );

    InitAfterPageInsertion(rDoc, nPage);
}

PdfXObject::PdfXObject( PdfDocument *pDoc, int nPage, const char* pszPrefix, bool bUseTrimBox )
    : PdfElement(*pDoc, "XObject"), PdfCanvas(), m_pResources( nullptr )
{
    m_rRect = PdfRect();

    InitXObject( m_rRect, pszPrefix );

    // After filling set correct BBox, independent of rotation
    m_rRect = pDoc->FillXObjectFromExistingPage( this, nPage, bUseTrimBox );

    InitAfterPageInsertion(*pDoc, nPage);
}

PdfXObject::PdfXObject(PdfObject* pObject)
    : PdfElement(*pObject), PdfCanvas(), m_pResources(nullptr)
{
    InitIdentifiers(getPdfXObjectType(*pObject));
    m_pResources = pObject->GetIndirectKey("Resources");

    if (this->GetObject()->GetIndirectKey("BBox"))
        m_rRect = PdfRect(this->GetObject()->GetIndirectKey("BBox")->GetArray());
}

PdfXObject::PdfXObject(EPdfXObject subType, PdfDocument* pParent, const char* pszPrefix)
    : PdfElement(*pParent, "XObject"), m_pResources(nullptr)
{
    InitIdentifiers(subType, pszPrefix);

    this->GetObject()->GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(subType)));
}

PdfXObject::PdfXObject(EPdfXObject subType, PdfVecObjects* pParent, const char* pszPrefix)
    : PdfElement(*pParent, "XObject"), m_pResources(nullptr)
{
    InitIdentifiers(subType, pszPrefix);

    this->GetObject()->GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(subType)));
}

PdfXObject::PdfXObject(EPdfXObject subType, PdfObject* pObject)
    : PdfElement(*pObject), m_pResources(nullptr)
{
    if (getPdfXObjectType(*pObject) != subType)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }

    InitIdentifiers(subType);
}

bool PdfXObject::TryCreateFromObject(PdfObject &obj, std::unique_ptr<PdfXObject>& xobj, EPdfXObject &type)
{
    auto typeObj = obj.GetDictionary().GetKey(PdfName::KeyType);
    if (typeObj == nullptr
        || !typeObj->IsName()
        || typeObj->GetName().GetString() != "XObject")
    {
        xobj = nullptr;
        type = EPdfXObject::Unknown;
        return false;
    }

    type = getPdfXObjectType(obj);
    switch (type)
    {
        case EPdfXObject::Form:
        case EPdfXObject::PostScript:
        {
            xobj.reset(new PdfXObject(type, &obj));
            return true;
        }
        case EPdfXObject::Image:
        {
            xobj.reset(new PdfImage(&obj));
            return true;
        }
        default:
        {
            xobj = nullptr;
            return false;
        }
    }
}

EPdfXObject PdfXObject::getPdfXObjectType(const PdfObject &obj)
{
    auto subTypeObj = obj.GetDictionary().GetKey(PdfName::KeySubtype);
    if (subTypeObj == nullptr || !subTypeObj->IsName())
        return EPdfXObject::Unknown;

    auto subtype = obj.GetDictionary().GetKey(PdfName::KeySubtype)->GetName().GetString();
    return FromString(subtype);
}

string PdfXObject::ToString(EPdfXObject type)
{
    switch (type)
    {
        case EPdfXObject::Form:
            return "Form";
        case EPdfXObject::Image:
            return "Image";
        case EPdfXObject::PostScript:
            return "PS";
        default:
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
    }
}

EPdfXObject PdfXObject::FromString(const string &str)
{
    if (str == "Form")
        return EPdfXObject::Form;
    else if (str == "Image")
        return EPdfXObject::Image;
    else if (str == "PS")
        return EPdfXObject::PostScript;
    else
        return EPdfXObject::Unknown;
}

void PdfXObject::InitAfterPageInsertion(const PdfDocument & rDoc, int nPage)
{
    PdfVariant    var;
    m_rRect.ToVariant(var);
    this->GetObject()->GetDictionary().AddKey("BBox", var);

    int rotation = rDoc.GetPage(nPage)->GetRotationRaw();
    // correct negative rotation
    if (rotation < 0)
        rotation = 360 + rotation;

    // Swap offsets/width/height for vertical rotation
    switch (rotation)
    {
        case 90:
        case 270:
        {
            double temp;

            temp = m_rRect.GetWidth();
            m_rRect.SetWidth(m_rRect.GetHeight());
            m_rRect.SetHeight(temp);

            temp = m_rRect.GetLeft();
            m_rRect.SetLeft(m_rRect.GetBottom());
            m_rRect.SetBottom(temp);
        }
        break;

        default:
            break;
    }

    // Build matrix for rotation and cropping
    double alpha = -rotation / 360.0 * 2.0 * PI;

    double a, b, c, d, e, f;

    a = cos(alpha);
    b = sin(alpha);
    c = -sin(alpha);
    d = cos(alpha);

    switch (rotation)
    {
        case 90:
            e = -m_rRect.GetLeft();
            f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;

        case 180:
            e = m_rRect.GetLeft() + m_rRect.GetWidth();
            f = m_rRect.GetBottom() + m_rRect.GetHeight();
            break;

        case 270:
            e = m_rRect.GetLeft() + m_rRect.GetWidth();
            f = -m_rRect.GetBottom();
            break;

        case 0:
        default:
            e = -m_rRect.GetLeft();
            f = -m_rRect.GetBottom();
            break;
    }

    PdfArray      matrix;
    matrix.push_back(PdfVariant(a));
    matrix.push_back(PdfVariant(b));
    matrix.push_back(PdfVariant(c));
    matrix.push_back(PdfVariant(d));
    matrix.push_back(PdfVariant(e));
    matrix.push_back(PdfVariant(f));

    this->GetObject()->GetDictionary().AddKey("Matrix", matrix);
}

PdfRect PdfXObject::GetRect() const
{
    return m_rRect;
}

bool PdfXObject::HasRotation(double& teta) const
{
    teta = 0;
    return false;
}

void PdfXObject::SetRect( const PdfRect & rect )
{
    PdfVariant array;
    rect.ToVariant( array );
    GetObject()->GetDictionary().AddKey( "BBox", array );
    m_rRect = rect;
}

void PdfXObject::EnsureResourcesInitialized()
{
    if ( m_pResources == nullptr )
        InitResources();

    // A Form XObject must have a stream
    GetObject()->ForceCreateStream();
}

PdfObject * PdfXObject::GetContents() const
{
    return const_cast<PdfXObject &>(*this).GetObject();
}

inline PdfStream & PdfXObject::GetStreamForAppending(EPdfStreamAppendFlags flags)
{
    (void)flags; // Flags have no use here
    return GetObject()->GetOrCreateStream();
}

void PdfXObject::InitXObject( const PdfRect & rRect, const char* pszPrefix )
{
    InitIdentifiers(EPdfXObject::Form, pszPrefix);

    // Initialize static data
    if( m_matrix.empty() )
    {
        // This matrix is the same for all PdfXObjects so cache it
        m_matrix.push_back( PdfVariant( static_cast<int64_t>(1) ) );
        m_matrix.push_back( PdfVariant( static_cast<int64_t>(0) ) );
        m_matrix.push_back( PdfVariant( static_cast<int64_t>(0) ) );
        m_matrix.push_back( PdfVariant( static_cast<int64_t>(1) ) );
        m_matrix.push_back( PdfVariant( static_cast<int64_t>(0) ) );
        m_matrix.push_back( PdfVariant( static_cast<int64_t>(0) ) );
    }

    PdfVariant    var;
    rRect.ToVariant( var );
    this->GetObject()->GetDictionary().AddKey( "BBox", var );
    this->GetObject()->GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(EPdfXObject::Form)));
    this->GetObject()->GetDictionary().AddKey( "FormType", PdfVariant( static_cast<int64_t>(1) ) ); // only 1 is only defined in the specification.
    this->GetObject()->GetDictionary().AddKey( "Matrix", m_matrix );

    InitResources();
}

void PdfXObject::InitIdentifiers(EPdfXObject subType, const char * pszPrefix)
{
    ostringstream out;
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
	if ( pszPrefix == nullptr )
	    out << "XOb" << this->GetObject()->GetIndirectReference().ObjectNumber();
	else
	    out << pszPrefix << this->GetObject()->GetIndirectReference().ObjectNumber();

    m_Identifier = PdfName( out.str().c_str() );
    m_Reference  = this->GetObject()->GetIndirectReference();
    m_type = subType;
}

void PdfXObject::InitResources()
{
    // The PDF specification suggests that we send all available PDF Procedure sets
    this->GetObject()->GetDictionary().AddKey("Resources", PdfObject(PdfDictionary()));
    m_pResources = this->GetObject()->GetDictionary().GetKey("Resources");
    m_pResources->GetDictionary().AddKey("ProcSet", PdfCanvas::GetProcSet());
}

PdfObject* PdfXObject::GetResources() const
{
    return m_pResources;
}
