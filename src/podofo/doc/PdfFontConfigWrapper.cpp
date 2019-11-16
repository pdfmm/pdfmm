/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
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

#include "PdfFontConfigWrapper.h"

#include <fontconfig/fontconfig.h>

using namespace std;

namespace PoDoFo {

PdfFontConfigWrapper::PdfFontConfigWrapper( FcConfig* pFcConfig )
    : m_pFcConfig( pFcConfig )
{
}

PdfFontConfigWrapper::~PdfFontConfigWrapper()
{
    unique_lock<mutex> lock(m_mutex);
    if ( m_pFcConfig )
        FcConfigDestroy( m_pFcConfig );
}

void PdfFontConfigWrapper::InitializeFontConfig()
{
    unique_lock<mutex> lock(m_mutex);
    if ( m_pFcConfig )
        return;

    // Default initialize FontConfig
    FcInit();
    m_pFcConfig = FcConfigGetCurrent();
}

std::string PdfFontConfigWrapper::GetFontConfigFontPath( const char* pszFontName, bool bBold, bool bItalic )
{
    InitializeFontConfig();

    FcPattern*  pattern;
    FcPattern*  matched;
    FcResult    result = FcResultMatch;
    FcValue     v;
    std::string sPath;
    // Build a pattern to search using fontname, bold and italic
    pattern = FcPatternBuild (0, FC_FAMILY, FcTypeString, pszFontName,
                              FC_WEIGHT, FcTypeInteger, (bBold ? FC_WEIGHT_BOLD : FC_WEIGHT_MEDIUM),
                              FC_SLANT, FcTypeInteger, (bItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN),
                              static_cast<char*>(0));

    FcDefaultSubstitute( pattern );

    if( !FcConfigSubstitute( m_pFcConfig, pattern, FcMatchFont ) )
        {
        FcPatternDestroy( pattern );
        return sPath;
        }

    matched = FcFontMatch( m_pFcConfig, pattern, &result );
    if( result != FcResultNoMatch )
    {
        result = FcPatternGet( matched, FC_FILE, 0, &v );
        sPath = reinterpret_cast<const char*>(v.u.s);
#ifdef PODOFO_VERBOSE_DEBUG
        PdfError::LogMessage( eLogSeverity_Debug,
                              "Got Font %s for for %s\n", sPath.c_str(), pszFontName );
#endif // PODOFO_DEBUG
    }

    FcPatternDestroy( pattern );
    FcPatternDestroy( matched );
    return sPath;
}

FcConfig * PdfFontConfigWrapper::GetFcConfig()
    {
    InitializeFontConfig();
    return m_pFcConfig;
    }

PdfFontConfigWrapper * PdfFontConfigWrapper::GetInstance()
{
    static PdfFontConfigWrapper wrapper;
    return &wrapper;
}

};
