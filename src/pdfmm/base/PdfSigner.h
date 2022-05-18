/**
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_SIGNER_H
#define PDF_SIGNER_H

#include "PdfDeclarations.h"

#include "PdfMemDocument.h"
#include "PdfSignature.h"

namespace mm
{
    class PDFMM_API PdfSigner
    {
    public:
        virtual ~PdfSigner();
        virtual void Reset() = 0;
        virtual void AppendData(const bufferview& data) = 0;
        /**
         * \param buffer The buffer that will hold the signature
         * \param dryrun If true the buffer is not required to hold
         *      the signature, the call is performed to infer the
         *      signature size
         */
        virtual void ComputeSignature(std::string& buffer, bool dryrun) = 0;
        virtual std::string GetSignatureFilter() const;
        virtual std::string GetSignatureSubFilter() const = 0;
        virtual std::string GetSignatureType() const = 0;
    };

    void SignDocument(PdfMemDocument& doc, PdfOutputDevice& device, PdfSigner& signer,
        PdfSignature& signature, PdfSaveOptions opts = PdfSaveOptions::None);
}

#endif // PDF_SIGNER_H
