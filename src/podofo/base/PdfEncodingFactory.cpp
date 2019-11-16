/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "PdfEncodingFactory.h"

#include "PdfEncoding.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfDefinesPrivate.h"
#include "doc/PdfIdentityEncoding.h"

using namespace std;

namespace PoDoFo {

const PdfDocEncoding *          PdfEncodingFactory::s_pDocEncoding          = nullptr;
const PdfWinAnsiEncoding *      PdfEncodingFactory::s_pWinAnsiEncoding      = nullptr;
const PdfMacRomanEncoding *     PdfEncodingFactory::s_pMacRomanEncoding     = nullptr;
const PdfStandardEncoding *     PdfEncodingFactory::s_pStandardEncoding     = nullptr;
const PdfMacExpertEncoding *    PdfEncodingFactory::s_pMacExpertEncoding    = nullptr;
const PdfSymbolEncoding *       PdfEncodingFactory::s_pSymbolEncoding       = nullptr;
const PdfZapfDingbatsEncoding * PdfEncodingFactory::s_pZapfDingbatsEncoding = nullptr;
const PdfIdentityEncoding *     PdfEncodingFactory::s_pIdentityEncoding     = nullptr;
const PdfWin1250Encoding *      PdfEncodingFactory::s_pWin1250Encoding      = nullptr;
const PdfIso88592Encoding *     PdfEncodingFactory::s_pIso88592Encoding     = nullptr;

mutex PdfEncodingFactory::s_mutex;

PdfEncodingFactory::PdfEncodingFactory() { }
	
const PdfEncoding* PdfEncodingFactory::GlobalPdfDocEncodingInstance()
{
    if(!s_pDocEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pDocEncoding)
            s_pDocEncoding = new PdfDocEncoding();
    }

    return s_pDocEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalWinAnsiEncodingInstance()
{
    if(!s_pWinAnsiEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pWinAnsiEncoding)
            s_pWinAnsiEncoding = new PdfWinAnsiEncoding();
    }

    return s_pWinAnsiEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalMacRomanEncodingInstance()
{
    if(!s_pMacRomanEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pMacRomanEncoding)
            s_pMacRomanEncoding = new PdfMacRomanEncoding();
    }

    return s_pMacRomanEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalStandardEncodingInstance()
{
    if(!s_pStandardEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pStandardEncoding)
            s_pStandardEncoding = new PdfStandardEncoding();
    }
    
    return s_pStandardEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalMacExpertEncodingInstance()
{
    if(!s_pMacExpertEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pMacExpertEncoding)
            s_pMacExpertEncoding = new PdfMacExpertEncoding();
    }
    
    return s_pMacExpertEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalSymbolEncodingInstance()
{
    if(!s_pSymbolEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pSymbolEncoding)
            s_pSymbolEncoding = new PdfSymbolEncoding();
    }

    return s_pSymbolEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalZapfDingbatsEncodingInstance()
{
    if(!s_pZapfDingbatsEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pZapfDingbatsEncoding)
            s_pZapfDingbatsEncoding = new PdfZapfDingbatsEncoding();
    }
    
    return s_pZapfDingbatsEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalIdentityEncodingInstance()
{
    if(!s_pIdentityEncoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pIdentityEncoding)
            s_pIdentityEncoding = new PdfIdentityEncoding( 0, 0xffff, false );
    }

    return s_pIdentityEncoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalWin1250EncodingInstance()
{
    if(!s_pWin1250Encoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pWin1250Encoding)
            s_pWin1250Encoding = new PdfWin1250Encoding();
    }

    return s_pWin1250Encoding;
}

const PdfEncoding* PdfEncodingFactory::GlobalIso88592EncodingInstance()
{
    if(!s_pIso88592Encoding)
    {
        unique_lock<mutex> lock(s_mutex);
        if(!s_pIso88592Encoding)
            s_pIso88592Encoding = new PdfIso88592Encoding();
    }

    return s_pIso88592Encoding;
}

int podofo_number_of_clients = 0;

void PdfEncodingFactory::FreeGlobalEncodingInstances()
{
    unique_lock<mutex> lock(s_mutex);	
    
    podofo_number_of_clients--;
    if (podofo_number_of_clients <= 0)
    {
        if (s_pMacRomanEncoding != nullptr)
        {
            delete s_pMacRomanEncoding;
        }
        if (s_pWinAnsiEncoding != nullptr)
        {
            delete s_pWinAnsiEncoding;
        }
        if (s_pDocEncoding != nullptr)
        {
            delete s_pDocEncoding;
        }
        if (s_pStandardEncoding != nullptr)
        {
            delete s_pStandardEncoding;
        }
        if (s_pMacExpertEncoding != nullptr)
        {
            delete s_pMacExpertEncoding;
        }
        if (s_pSymbolEncoding != nullptr)
        {
            delete s_pSymbolEncoding;
        }
        if (s_pZapfDingbatsEncoding != nullptr)
        {
            delete s_pZapfDingbatsEncoding;
        }
        if (s_pIdentityEncoding != nullptr)
        {
            delete s_pIdentityEncoding;
        }
        if (s_pWin1250Encoding != nullptr)
        {
            delete s_pWin1250Encoding;
        }
        if (s_pIso88592Encoding != nullptr)
        {
            delete s_pIso88592Encoding;
        }

        s_pMacRomanEncoding     = nullptr;
        s_pWinAnsiEncoding      = nullptr;
        s_pDocEncoding          = nullptr;
        s_pStandardEncoding     = nullptr;
        s_pMacExpertEncoding    = nullptr;
        s_pSymbolEncoding       = nullptr;
        s_pZapfDingbatsEncoding = nullptr;
        s_pIdentityEncoding     = nullptr;
        s_pWin1250Encoding      = nullptr;
        s_pIso88592Encoding     = nullptr;
    }
}

void PdfEncodingFactory::PoDoFoClientAttached()
{
    podofo_number_of_clients++;
}

};
