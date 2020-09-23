/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfFontMetricsObject.h"

#include "base/PdfDefinesPrivate.h"

#include <doc/PdfDocument.h>
#include "base/PdfDictionary.h"
#include "base/PdfName.h"
#include "base/PdfObject.h"
#include "base/PdfVariant.h"

namespace PoDoFo {

PdfFontMetricsObject::PdfFontMetricsObject( PdfObject* pFont, PdfObject* pDescriptor, const PdfEncoding* const pEncoding )
    : PdfFontMetrics( EPdfFontType::Unknown, "", NULL ),
      m_pEncoding( pEncoding ), m_dDefWidth(0.0)
{
    m_missingWidth = NULL;

    const PdfName & rSubType = pFont->GetDictionary().GetKey( PdfName::KeySubtype )->GetName();

    PdfObject* fontmatrix = nullptr;
    // OC 15.08.2010 BugFix: /FirstChar /LastChar /Widths are in the Font dictionary and not in the FontDescriptor
	if ( rSubType == PdfName("Type1") || rSubType == PdfName("Type3") || rSubType == PdfName("TrueType") ) {
        if ( pDescriptor ) {
            if (pDescriptor->GetDictionary().HasKey( "FontName" ))
                m_sName        = pDescriptor->GetIndirectKey( "FontName" )->GetName();
            if (pDescriptor->GetDictionary().HasKey( "FontBBox" ))
                m_bbox         = pDescriptor->GetIndirectKey( "FontBBox" )->GetArray();
        } else {
            if (pFont->GetDictionary().HasKey( "Name" ))
                m_sName        = pFont->GetIndirectKey( "Name" )->GetName();
            if (pFont->GetDictionary().HasKey( "FontBBox" ))
                m_bbox         = pFont->GetIndirectKey( "FontBBox" )->GetArray();
        }

        // Type3 fonts have a custom FontMatrix
        fontmatrix = pFont->GetDictionary().FindKey( "FontMatrix" );

		m_nFirst       = static_cast<int>(pFont->GetDictionary().GetKeyAsNumber( "FirstChar", 0L ));
        m_nLast        = static_cast<int>(pFont->GetDictionary().GetKeyAsNumber( "LastChar", 0L ));
        // OC 15.08.2010 BugFix: GetIndirectKey() instead of GetDictionary().GetKey() and "Widths" instead of "Width"
        PdfObject* widths = pFont->GetIndirectKey( "Widths" );
        
        if( widths != NULL )
        {
            m_width        = widths->GetArray();
            m_missingWidth = NULL;
        }
        else
        {
            if ( pDescriptor ) {
                widths = pDescriptor->GetDictionary().GetKey( "MissingWidth" );
            } else {
                widths = pFont->GetDictionary().GetKey( "MissingWidth" );
            }
            if( widths == NULL ) 
            {
                PODOFO_RAISE_ERROR_INFO( EPdfError::NoObject, "Font object defines neither Widths, nor MissingWidth values!" );
            }
            m_missingWidth = widths;
        }
	} else if ( rSubType == PdfName("CIDFontType0") || rSubType == PdfName("CIDFontType2") ) {
		PdfObject *pObj = pDescriptor->GetIndirectKey( "FontName" );
		if (pObj) {
			m_sName = pObj->GetName();
		}
		pObj = pDescriptor->GetIndirectKey( "FontBBox" );
		if (pObj) {
			m_bbox = pObj->GetArray();
		}
		m_nFirst = 0;
		m_nLast = 0;

		m_dDefWidth = static_cast<double>(pFont->GetDictionary().GetKeyAsNumber( "DW", 1000L ));
		PdfVariant default_width(m_dDefWidth);
		PdfObject * pw = pFont->GetIndirectKey( "W" );

		for (int i = m_nFirst; i <= m_nLast; ++i) {
			m_width.push_back(default_width);
		}
		if (pw) {
			PdfArray w = pw->GetArray();
			int pos = 0;
			while (pos < static_cast<int>(w.GetSize())) {
				int start = static_cast<int>(w[pos++].GetNumberLenient());
				PODOFO_ASSERT (start >= 0);
				PdfObject * second = &w[pos];
				if (second->IsReference()) {
					// second do not have an associated owner; use the one in pw
					second = pw->GetDocument()->GetObjects().GetObject(second->GetReference());
					PODOFO_ASSERT (!second->IsNull());
				}
				if (second->IsArray()) {
					PdfArray widths = second->GetArray();
					++pos;
					int length = start + widths.GetSize();
					PODOFO_ASSERT (length >= start);
					if (length > m_width.GetSize()) {
						m_width.resize(length, default_width);
					}
					for (int i = 0; i < widths.GetSize(); ++i) {
						m_width[start + i] = widths[i];
					}
				} else {
					int end = static_cast<int>(w[pos++].GetNumberLenient());
					int length = end + 1;
					PODOFO_ASSERT (length >= start);
					if (length > m_width.GetSize()) {
						m_width.resize(length, default_width);
					}
					int64_t width = w[pos++].GetNumberLenient();
					for (int i = start; i <= end; ++i)
						m_width[i] = PdfObject(width);
				}
			}
		}
		m_nLast = m_width.GetSize() - 1;
    } else {
        PODOFO_RAISE_ERROR_INFO( EPdfError::UnsupportedFontFormat, rSubType.GetEscapedName().c_str() );
	}
    
    if ( pDescriptor ) {
        m_nWeight      = static_cast<unsigned int>(pDescriptor->GetDictionary().GetKeyAsNumber( "FontWeight", 400L ));
        m_nItalicAngle = static_cast<int>(pDescriptor->GetDictionary().GetKeyAsNumber( "ItalicAngle", 0L ));
        m_dPdfAscent   = pDescriptor->GetDictionary().GetKeyAsReal( "Ascent", 0.0 );
        m_dPdfDescent  = pDescriptor->GetDictionary().GetKeyAsReal( "Descent", 0.0 );
    } else {
        m_nWeight      = 400L;
        m_nItalicAngle = 0L;
        m_dPdfAscent   = 0.0;
        m_dPdfDescent  = 0.0;
    }


    if (fontmatrix == nullptr)
    {
        m_matrix = { 0.001, 0.0, 0.0, 0.001, 0, 0};
    }
    else
    {
        auto& fontmatrixArr = fontmatrix->GetArray();
        for (int i = 0; i < 6; i++)
            m_matrix[i] = fontmatrixArr[i].GetReal();
    }
    
    m_dAscent      = m_dPdfAscent * m_matrix[3];
    m_dDescent     = m_dPdfDescent * m_matrix[3];
    m_dLineSpacing = m_dAscent + m_dDescent;
    
    // Try to fine some sensible values
    m_dUnderlineThickness = 1.0;
    m_dUnderlinePosition  = 0.0;
    m_dStrikeOutThickness = m_dUnderlinePosition;
    m_dStrikeOutPosition  = m_dAscent / 2.0;

    m_bSymbol = false; // TODO
}

const char* PdfFontMetricsObject::GetFontname() const
{
    return m_sName.GetString().c_str();
}

void PdfFontMetricsObject::GetBoundingBox( PdfArray & array ) const
{
    array = m_bbox;
}

double PdfFontMetricsObject::CharWidth( unsigned char c ) const
{
    if( c >= m_nFirst && c <= m_nLast
        && c - m_nFirst < m_width.GetSize())
    {
        double dWidth = m_width[c - m_nFirst].GetReal();
        
        return (dWidth * m_matrix[0] * m_fFontSize + m_fFontCharSpace) * m_fFontScale / 100.0;

    }

    if( m_missingWidth != NULL )
        return m_missingWidth->GetReal ();
    else
        return m_dDefWidth;
}

double PdfFontMetricsObject::UnicodeCharWidth( unsigned short c ) const
{
    if( c >= m_nFirst && c <= m_nLast
        && c - m_nFirst < m_width.GetSize())
    {
        double dWidth = m_width[c - m_nFirst].GetReal();
        
        return (dWidth * m_matrix[0] * m_fFontSize + m_fFontCharSpace) * m_fFontScale / 100.0;
    }

    if( m_missingWidth != NULL )
        return m_missingWidth->GetReal ();
    else
        return m_dDefWidth;
}

void PdfFontMetricsObject::GetWidthArray( PdfVariant & var, unsigned int, unsigned int, const PdfEncoding* ) const
{
    var = m_width;
}

double PdfFontMetricsObject::GetGlyphWidth( int ) const
{
    // TODO
    return 0.0;
}

double PdfFontMetricsObject::GetGlyphWidth( const char* ) const
{
    // TODO
    return 0.0;
}

long PdfFontMetricsObject::GetGlyphId( long ) const
{
    // TODO
    return 0;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetLineSpacing() const
{
    return m_dLineSpacing * m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetUnderlinePosition() const
{
    return m_dUnderlinePosition * m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetStrikeOutPosition() const
{
	return m_dStrikeOutPosition * m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetUnderlineThickness() const
{
    return m_dUnderlineThickness * m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetStrikeoutThickness() const
{
    return m_dStrikeOutThickness * m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
const char* PdfFontMetricsObject::GetFontData() const
{
    return nullptr;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
size_t PdfFontMetricsObject::GetFontDataLen() const
{
    return 0;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
unsigned int PdfFontMetricsObject::GetWeight() const
{
    return m_nWeight;
}  

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetAscent() const
{
    return m_dAscent * m_fFontSize;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetPdfAscent() const
{
    return m_dPdfAscent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetDescent() const
{
    return m_dDescent * this->GetFontSize();
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
double PdfFontMetricsObject::GetPdfDescent() const
{
    return m_dPdfDescent;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
int PdfFontMetricsObject::GetItalicAngle() const
{
    return m_nItalicAngle;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
bool PdfFontMetricsObject::IsSymbol() const
{
    return m_bSymbol;
}

};
