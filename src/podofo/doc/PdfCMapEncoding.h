/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   Pdf CMAP encoding by kalyan                                           *
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

#ifndef _PDF_CMAP_ENCODING_H
#define _PDF_CMAP_ENCODING_H

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfEncoding.h"
#include "PdfElement.h"


namespace PoDoFo {

class PODOFO_DOC_API PdfCMapEncoding: public PdfEncoding
{
public:
    enum class EBaseEncoding
    {
        Font,      ///< Use The fonts encoding as base
        WinAnsi,   ///< Use WinAnsiEncoding as base encoding
        MacRoman,  ///< Use MacRomanEncoding as base encoding
        MacExpert  ///< Use MacExpertEncoding as base encoding
    };

public:
    PdfCMapEncoding(PdfObject* pObject, PdfObject* pToUnicode = NULL);
    PdfString ConvertToUnicode(const PdfString& rEncodedString, const PdfFont* pFont) const override;
    void AddToDictionary(PdfDictionary & rDictionary ) const override;
    PdfRefCountedBuffer ConvertToEncoding(const PdfString& rString, const PdfFont* pFont) const override;
    bool IsAutoDelete() const override;
    bool IsSingleByteEncoding() const override;
    pdf_utf16be GetCharCode(int nIndex) const override;
    const PdfName & GetID() const override;
    const PdfEncoding* GetBaseEncoding() const;

private:
    EBaseEncoding m_baseEncoding;
    char32_t m_nFirstCode;               // The first defined character code
    char32_t m_nLastCode;                // The last defined character code
    unsigned m_maxCodeRangeSize;         // Size of in bytes of the bigger code range
    UnicodeMap m_toUnicode;
};

}; /*PoDoFo namespace end*/

#endif // _PDF_CMAP_ENCODING_H 




