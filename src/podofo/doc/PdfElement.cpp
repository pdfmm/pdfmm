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

#include "PdfElement.h"

#include "base/PdfDefinesPrivate.h"

#include <doc/PdfDocument.h>
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"

#include "PdfStreamedDocument.h"

#include <string.h>

using namespace PoDoFo;

PdfElement::PdfElement(PdfVecObjects& pParent, const std::string_view& type)
{
    m_pObject = pParent.CreateDictionaryObject(type);
}

PdfElement::PdfElement(PdfDocument& pParent, const std::string_view& type)
{
    m_pObject = pParent.m_vecObjects.CreateDictionaryObject(type);
}

PdfElement::PdfElement(PdfObject& obj)
{
    if(!obj.IsDictionary())
        PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);

    m_pObject = &obj;
}

PdfElement::PdfElement( EPdfDataType eExpectedDataType, PdfObject* pObject ) 
{
    if( !pObject )         
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_pObject = pObject;

    if( m_pObject->GetDataType() != eExpectedDataType ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
    }
}

PdfElement::PdfElement(const PdfElement & element)
{
    m_pObject = element.GetNonConstObject();
}

PdfElement::~PdfElement() { }

const char* PdfElement::TypeNameForIndex( int i, const char** ppTypes, long lLen ) const
{
    return ( i >= lLen ? nullptr : ppTypes[i] );
}

int PdfElement::TypeNameToIndex( const char* pszType, const char** ppTypes, long lLen, int nUnknownValue ) const
{
    int i;

    if( !pszType )
        return nUnknownValue;

    for( i=0; i<lLen; i++ )
    {
        if( ppTypes[i] && strcmp( pszType, ppTypes[i] ) == 0 ) 
        {
            return i;
        }
    }
    
    return nUnknownValue;
}
PdfObject* PdfElement::CreateObject( const char* pszType )
{
    return m_pObject->GetDocument()->GetObjects().CreateDictionaryObject( pszType );
}

PdfObject * PdfElement::GetNonConstObject() const
{
    return const_cast<PdfElement*>(this)->m_pObject;
}

PdfDocument& PdfElement::GetDocument()
{
    return *m_pObject->GetDocument();
}

const PdfDocument& PdfElement::GetDocument() const
{
    return *m_pObject->GetDocument();
}
