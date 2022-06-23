/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_STATEFUL_ENCRYPT_H
#define PDF_STATEFUL_ENCRYPT_H

#include "PdfDeclarations.h"
#include "PdfReference.h"

namespace mm
{
    class PdfEncrypt;

    class PDFMM_API PdfStatefulEncrypt final
    {
    public:
        PdfStatefulEncrypt();
        PdfStatefulEncrypt(const PdfEncrypt& encrypt, const PdfReference& objref);
        PdfStatefulEncrypt(const PdfStatefulEncrypt&) = default;
    public:
        /** Encrypt a character span
         */
        void EncryptTo(charbuff& out, const bufferview& view) const;

        /** Decrypt a character span
         */
        void DecryptTo(charbuff& out, const bufferview& view) const;

        size_t CalculateStreamLength(size_t length) const;

        bool HasEncrypt() const { return m_encrypt != nullptr; }

    public:
        PdfStatefulEncrypt& operator=(const PdfStatefulEncrypt&) = default;

    private:
        const PdfEncrypt* m_encrypt;
        PdfReference m_currReference;       // Reference of the current PdfObject
    };
}

#endif // PDF_STATEFUL_ENCRYPT_H
