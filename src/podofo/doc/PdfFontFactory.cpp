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

#include "PdfFontFactory.h"

#include "base/PdfDefinesPrivate.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfEncoding.h"
#include "base/PdfEncodingFactory.h"

#include "PdfEncodingObjectFactory.h"
#include "PdfFont.h"
#include "PdfFontCID.h"
#include "PdfFontMetrics.h"
#include "PdfFontMetricsBase14.h"
#include "PdfFontMetricsObject.h"
#include "PdfFontType1.h"
#include "PdfFontType3.h"
#include "PdfFontType1Base14.h"
#include "PdfFontTrueType.h"
#include "PdfFontFactoryBase14Data.h"

namespace PoDoFo {

PdfFontFactory::PdfFontFactory()
{
}

PdfFont* PdfFontFactory::CreateFontObject( PdfFontMetrics* pMetrics, EPdfFontFlags nFlags,
                                           const PdfEncoding* pEncoding,
                                           PdfVecObjects* pParent )
{
    PdfFont * pFont  = nullptr;
    EPdfFontType eType = pMetrics->GetFontType();
    bool bEmbed = (nFlags & EPdfFontFlags::Embedded) == EPdfFontFlags::Embedded;
    bool bSubsetting = (nFlags & EPdfFontFlags::Subsetting) != EPdfFontFlags::Normal;

    try
    { 
        pFont = PdfFontFactory::CreateFontForType( eType, pMetrics, pEncoding, bEmbed, bSubsetting, pParent );
        
        if( pFont ) 
        {
            pFont->SetBold((nFlags & EPdfFontFlags::Bold) == EPdfFontFlags::Bold ? true : false );
            pFont->SetItalic((nFlags & EPdfFontFlags::Italic) == EPdfFontFlags::Italic ? true : false );
        }
        else
        {
            // something went wrong, so we have to delete
            // the font metrics
            delete pMetrics;
            pMetrics = nullptr;
            // make sure this will be done before the catch block
            // as the encoding might be deleted already
            // afterwars, but we cannot set the pointer to nullptr
            if( pEncoding && pEncoding->IsAutoDelete() )
            {
                delete pEncoding;
                pEncoding = nullptr;
            }
        }
    }
    catch( PdfError & e ) 
    {
        // we have to delete the pMetrics object in case of error 
        if( pFont ) 
        {
            // The font will delete encoding and metrics
            delete pFont;
            pFont = nullptr;
        }
        else
        {
            // something went wrong, so we have to delete
            // the font metrics (and if auto-delete, also the encoding)
            delete pMetrics;
            pMetrics = nullptr;

            if( pEncoding && pEncoding->IsAutoDelete() )
                delete pEncoding;
        }

        e.AddToCallstack( __FILE__, __LINE__, "Font creation failed." );
        throw e;
        
    }
    
    return pFont;
}

PdfFont* PdfFontFactory::CreateFontForType( EPdfFontType eType, PdfFontMetrics* pMetrics, 
                                            const PdfEncoding* const pEncoding, 
                                            bool bEmbed, bool bSubsetting, PdfVecObjects* pParent )
{
    PdfFont* pFont = nullptr;

    if( pEncoding->IsSingleByteEncoding() ) 
    {
        switch( eType ) 
        {
            case EPdfFontType::TrueType:
                // Peter Petrov 30 April 2008 - added bEmbed parameter
		        if (bSubsetting) {
		            pFont = new PdfFontCID( pMetrics, pEncoding, pParent, bEmbed, true );
		        }
		        else {
                    pFont = new PdfFontTrueType( pMetrics, pEncoding, pParent, bEmbed );
		        }
                break;
                
            case EPdfFontType::Type1Pfa:
            case EPdfFontType::Type1Pfb:
				if ( bSubsetting )
				{
					// don't embed yet for subsetting
	                pFont = new PdfFontType1( pMetrics, pEncoding, pParent, false, true );
				}
				else
					pFont = new PdfFontType1( pMetrics, pEncoding, pParent, bEmbed );
                break;
            case EPdfFontType::Type3:
                pFont = new PdfFontType3( pMetrics, pEncoding, pParent, bEmbed );
                break;
            case EPdfFontType::Unknown:
            case EPdfFontType::Type1Base14:
            default:
                PdfError::LogMessage( ELogSeverity::Error, "The font format is unknown. Fontname: %s Filename: %s", 
                                      (pMetrics->GetFontname() ? pMetrics->GetFontname() : "<unknown>"),
                                      (pMetrics->GetFilename() ? pMetrics->GetFilename() : "<unknown>") );
        }
    }
    else
    {
        switch( eType ) 
        {
            case EPdfFontType::TrueType:
                // Peter Petrov 30 April 2008 - added bEmbed parameter
					 pFont = new PdfFontCID( pMetrics, pEncoding, pParent, bEmbed, bSubsetting );
                break;
            case EPdfFontType::Type1Pfa:
            case EPdfFontType::Type1Pfb:
            case EPdfFontType::Type1Base14:
            case EPdfFontType::Type3:
            case EPdfFontType::Unknown:
            default:
                PdfError::LogMessage( ELogSeverity::Error, 
                                      "The font format is unknown or no multibyte encoding defined. Fontname: %s Filename: %s", 
                                      (pMetrics->GetFontname() ? pMetrics->GetFontname() : "<unknown>"),
                                      (pMetrics->GetFilename() ? pMetrics->GetFilename() : "<unknown>") );
        }
    }

    return pFont;
}

PdfFont* PdfFontFactory::CreateFont( FT_Library*, PdfObject* pObject )
{
    PdfFontMetrics* pMetrics    = nullptr;
    PdfFont*        pFont       = nullptr;
    PdfObject*      pDescriptor = nullptr;
    PdfObject*      pEncoding   = nullptr;
    PdfObject*      pToUnicode = nullptr;

    PdfObject* pTypeKey = pObject->GetDictionary().GetKey( PdfName::KeyType );
    if ( nullptr == pTypeKey )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Font: No Type" );
    }

    if( pTypeKey->GetName() != PdfName("Font") )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
    }

    PdfObject* pSubTypeKey = pObject->GetDictionary()
                            .GetKey( PdfName::KeySubtype );
    if ( nullptr == pSubTypeKey )
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Font: No SubType" );
    }
    const PdfName & rSubType = pSubTypeKey->GetName();
    if( rSubType == PdfName("Type0") ) 
    {
        // TABLE 5.18 Entries in a Type 0 font dictionary

        // The PDF reference states that DescendantFonts must be an array,
        // some applications (e.g. MS Word) put the array into an indirect object though.
        PdfObject* pDescendantObj = pObject->GetIndirectKey( "DescendantFonts" );

        if ( nullptr == pDescendantObj )
            PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidDataType, "Type0 Font: No DescendantFonts" );
        
        PdfArray & descendants  = pDescendantObj->GetArray();
        PdfObject* pFontObject = nullptr;
        
        if ( descendants.size() )
        {
            // DescendantFonts is an one-element array
            PdfObject &descendant = descendants[0];
            if ( descendant.IsReference() )
            {
                pFontObject = pObject->GetDocument()->GetObjects().GetObject( descendant.GetReference() );
                pDescriptor = pFontObject->GetIndirectKey( "FontDescriptor" );
            }
            else
            {
                pFontObject = &descendant;
                pDescriptor = pFontObject->GetIndirectKey( "FontDescriptor" );
            }
        }
        pEncoding   = pObject->GetIndirectKey( "Encoding" );

        if ( pEncoding && pDescriptor ) // OC 18.08.2010: Avoid sigsegv
        {
            const PdfEncoding* const pPdfEncoding =
               PdfEncodingObjectFactory::CreateEncoding( pEncoding, pObject->GetIndirectKey("ToUnicode") );
            
            // OC 15.08.2010 BugFix: Parameter pFontObject added: TODO: untested
            pMetrics    = new PdfFontMetricsObject( pFontObject, pDescriptor, pPdfEncoding );
            pFont       = new PdfFontCID( pMetrics, pPdfEncoding, pObject, false );
        }
    }
    else if( rSubType == PdfName("Type1") ) 
    {
        // TODO: Old documents do not have a FontDescriptor for 
        //       the 14 standard fonts. This suggestions is 
        //       deprecated now, but give us problems with old documents.
        pDescriptor = pObject->GetIndirectKey( "FontDescriptor" );
        pEncoding   = pObject->GetIndirectKey( "Encoding" );

        // OC 13.08.2010: Handle missing FontDescriptor for the 14 standard fonts:
        if( !pDescriptor )
        {
           // Check if its a PdfFontType1Base14
           PdfObject* pBaseFont = nullptr;
           pBaseFont = pObject->GetIndirectKey( "BaseFont" );
           if ( nullptr == pBaseFont )
               PODOFO_RAISE_ERROR_INFO( EPdfError::NoObject, "No BaseFont object found"
                                       " by reference in given object" );
           const char* pszBaseFontName = pBaseFont->GetName().GetString().c_str();
           const PdfFontMetricsBase14* pMetrics = PODOFO_Base14FontDef_FindBuiltinData(pszBaseFontName);
           if ( pMetrics != nullptr )
           {
               // pEncoding may be undefined, found a valid pdf with
               //   20 0 obj
               //   <<
               //   /Type /Font
               //   /BaseFont /ZapfDingbats
               //   /Subtype /Type1
               //   >>
               //   endobj 
               // If pEncoding is null then
               // use StandardEncoding for Courier, Times, Helvetica font families
               // and special encodings for Symbol and ZapfDingbats
               const PdfEncoding* pPdfEncoding = nullptr;
               if ( pEncoding!= nullptr )
                   pPdfEncoding = PdfEncodingObjectFactory::CreateEncoding( pEncoding );
               else if ( !pMetrics->IsSymbol() )
                   pPdfEncoding = PdfEncodingFactory::GlobalStandardEncodingInstance();
               else if ( strcmp(pszBaseFontName, "Symbol") == 0 )
                   pPdfEncoding = PdfEncodingFactory::GlobalSymbolEncodingInstance();
               else if ( strcmp(pszBaseFontName, "ZapfDingbats") == 0 )
                   pPdfEncoding = PdfEncodingFactory::GlobalZapfDingbatsEncodingInstance();
               return new PdfFontType1Base14(new PdfFontMetricsBase14(*pMetrics), pPdfEncoding, pObject);
           }
        }
        const PdfEncoding* pPdfEncoding = nullptr;
        if ( pEncoding != nullptr )
            pPdfEncoding = PdfEncodingObjectFactory::CreateEncoding( pEncoding );
        else if ( pDescriptor )
        {
           // OC 18.08.2010 TODO: Encoding has to be taken from the font's built-in encoding
           // Its extremely complicated to interpret the type1 font programs
           // so i try to determine if its a symbolic font by reading the FontDescriptor Flags
           // Flags & 4 --> Symbolic, Flags & 32 --> Nonsymbolic
            int32_t lFlags = static_cast<int32_t>(pDescriptor->GetDictionary().GetKeyAsNumber( "Flags", 0L ));
            if ( lFlags & 32 ) // Nonsymbolic, otherwise pEncoding remains nullptr
                pPdfEncoding = PdfEncodingFactory::GlobalStandardEncodingInstance();
        }
        if ( pPdfEncoding && pDescriptor ) // OC 18.08.2010: Avoid sigsegv
        {
            // OC 15.08.2010 BugFix: Parameter pObject added:
            pMetrics    = new PdfFontMetricsObject( pObject, pDescriptor, pPdfEncoding );
            pFont       = new PdfFontType1( pMetrics, pPdfEncoding, pObject );
        }
    }
    else if( rSubType == PdfName("Type3") )
    {
        pDescriptor = pObject->GetIndirectKey( "FontDescriptor" );
        pEncoding   = pObject->GetIndirectKey( "Encoding" );
        
        if ( pEncoding ) // FontDescriptor may only be present in PDF 1.5+
        {
            const PdfEncoding* const pPdfEncoding =
            PdfEncodingObjectFactory::CreateEncoding( pEncoding, nullptr, true );
            
            pMetrics    = new PdfFontMetricsObject( pObject, pDescriptor, pPdfEncoding );
            pFont       = new PdfFontType3( pMetrics, pPdfEncoding, pObject );
        }
    }
    else if( rSubType == PdfName("TrueType") )
    {
        pDescriptor = pObject->GetIndirectKey( "FontDescriptor" );
        pEncoding   = pObject->GetIndirectKey( "Encoding" );
        pToUnicode   = pObject->GetIndirectKey("ToUnicode");

        if (!pEncoding)
            pEncoding = pToUnicode;

        if ( pEncoding && pDescriptor ) // OC 18.08.2010: Avoid sigsegv
        {
           const PdfEncoding* const pPdfEncoding = 
               PdfEncodingObjectFactory::CreateEncoding( pEncoding, pToUnicode );

           // OC 15.08.2010 BugFix: Parameter pObject added:
           pMetrics    = new PdfFontMetricsObject( pObject, pDescriptor, pPdfEncoding );
           pFont       = new PdfFontTrueType( pMetrics, pPdfEncoding, pObject );
        }
    }

    return pFont;
}

EPdfFontType PdfFontFactory::GetFontType( const char* pszFilename )
{
    EPdfFontType eFontType = EPdfFontType::Unknown;

    // We check by file extension right now
    // which is not quite correct, but still better than before

    if( pszFilename && strlen( pszFilename ) > 3 )
    {
        const char* pszExtension = pszFilename + strlen( pszFilename ) - 3;
        if( PoDoFo::compat::strncasecmp( pszExtension, "ttf", 3 ) == 0 )
            eFontType = EPdfFontType::TrueType;
        else if( PoDoFo::compat::strncasecmp( pszExtension, "otf", 3 ) == 0 )
            eFontType = EPdfFontType::TrueType;
        else if( PoDoFo::compat::strncasecmp( pszExtension, "ttc", 3 ) == 0 )
            eFontType = EPdfFontType::TrueType;
        else if( PoDoFo::compat::strncasecmp( pszExtension, "pfa", 3 ) == 0 )
            eFontType = EPdfFontType::Type1Pfa;
        else if( PoDoFo::compat::strncasecmp( pszExtension, "pfb", 3 ) == 0 )
            eFontType = EPdfFontType::Type1Pfb;
    }

    return eFontType;
}


const PdfFontMetricsBase14*
PODOFO_Base14FontDef_FindBuiltinData(const char  *font_name)
{
    unsigned int i = 0;
	bool found = false;
    while (PODOFO_BUILTIN_FONTS[i].font_name) {
        if (strcmp(PODOFO_BUILTIN_FONTS[i].font_name, font_name) == 0) // kaushik  : HPDFStrcmp changed to strcmp
		{
			found = true;
			break;
		}

        i++;
    }

	return found ? &PODOFO_BUILTIN_FONTS[i] : nullptr;
}

PdfFont *PdfFontFactory::CreateBase14Font(const char* pszFontName,
                    EPdfFontFlags eFlags, const PdfEncoding * const pEncoding,
                    PdfVecObjects *pParent)
{
    PdfFont *pFont = new PdfFontType1Base14(
        new PdfFontMetricsBase14(*PODOFO_Base14FontDef_FindBuiltinData(pszFontName)), pEncoding, pParent);
    if (pFont) {
        pFont->SetBold((eFlags & EPdfFontFlags::Bold) == EPdfFontFlags::Bold ? true : false );
        pFont->SetItalic((eFlags & EPdfFontFlags::Italic) == EPdfFontFlags::Italic ? true : false );
    }
    return pFont;
}

};
