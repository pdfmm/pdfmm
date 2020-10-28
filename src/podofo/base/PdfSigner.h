/***************************************************************************
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
#ifndef PDF_READER_H
#define PDF_READER_H

#include <string>
#include "..\doc\PdfMemDocument.h"
#include "..\doc\PdfSignatureField.h"

namespace PoDoFo
{
    class PdfSigner
    {
    public:
        virtual ~PdfSigner();
        /** Get the expected signature size
         *
         * Will be used to parepare the pdf file for signing.
         * By default it run a dry run computing of the signature
         */
        virtual unsigned GetSignatureSize();
        virtual void Reset() = 0;
        virtual void AppendData(const std::string_view& data) = 0;
        /**
         * \param dryrun The call is just used to infer signature size
         */
        virtual std::string ComputeSignature(bool dryrun) = 0;
        virtual std::string GetSignatureFilter() const;
        virtual std::string GetSignatureSubFilter() const = 0;
    };

    enum class PdfSignFlags
    {
        None = 0,
        // TODO:
        // NoIncrementalUpdate = 1,
        // NoAcroFormUpdate
    };

    void SignDocument(PdfMemDocument& doc, PdfOutputDevice& device, PdfSigner& signer,
        PdfSignatureField& signature, PdfSignFlags flags = PdfSignFlags::None);
}

#endif // PDF_READER_H
