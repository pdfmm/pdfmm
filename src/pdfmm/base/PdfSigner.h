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
    class StreamDevice;

    class PDFMM_API PdfSigner
    {
    public:
        virtual ~PdfSigner();

        /**
         * Called before computing the signature with ComputeSignature(buffer, false)
         */
        virtual void Reset() = 0;

        /**
         * Called incrementally with document raw data to compute
         * the signature with
         * \param data incremental raw data 
         */
        virtual void AppendData(const bufferview& data) = 0;
        /**
         * Called to compute the signature 
         * \param buffer The buffer that will hold the signature
         * \param dryrun If true the buffer is not required to
         *   hold the signature, the call is just performed to
         *   infer the signature size
         */
        virtual void ComputeSignature(charbuff& buffer, bool dryrun) = 0;

        /**
         * Should return the signature /Filter, for example "Adobe.PPKLite"
         */
        virtual std::string GetSignatureFilter() const;

        /**
         * Should return the signature /SubFilter, for example "ETSI.CAdES.detached"
         */
        virtual std::string GetSignatureSubFilter() const = 0;

        /**
         * Should return the signature /Type. It can be "Sig" or "DocTimeStamp"
         */
        virtual std::string GetSignatureType() const = 0;
    };

    void SignDocument(PdfMemDocument& doc, StreamDevice& device, PdfSigner& signer,
        PdfSignature& signature, PdfSaveOptions opts = PdfSaveOptions::None);
}

#endif // PDF_SIGNER_H
