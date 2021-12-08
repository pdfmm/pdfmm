/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OPERATOR_STACK_H
#define PDF_OPERATOR_STACK_H

#include "PdfVariant.h"

namespace mm {

class PDFMM_API PdfVariantStack final
{
    friend class PdfContentsReader;

public:
    using Stack = std::vector<PdfVariant>;
    using iterator = std::vector<PdfVariant>::const_reverse_iterator;

public:
    unsigned GetSize() const;
    const PdfVariant& operator[](size_t index) const;
    iterator begin() const;
    iterator end() const;
    size_t size() const;

private:
    void Push(const PdfVariant& var);
    void Clear();

private:
    std::vector<PdfVariant> m_variants;
};

}

#endif // PDF_OPERATOR_STACK_H
