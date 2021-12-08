#include "PdfVariantStack.h"

using namespace std;
using namespace mm;

unsigned PdfVariantStack::GetSize() const
{
    return (unsigned)m_variants.size();
}

const PdfVariant& PdfVariantStack::operator[](size_t index) const
{
    index = (m_variants.size() - 1) - index;
    if (index >= m_variants.size())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index {} is out of range", index);

    return m_variants[index];
}

PdfVariantStack::iterator PdfVariantStack::begin() const
{
    return m_variants.rbegin();
}

PdfVariantStack::iterator PdfVariantStack::end() const
{
    return m_variants.rend();
}

size_t PdfVariantStack::size() const
{
    return m_variants.size();
}

void PdfVariantStack::Push(const PdfVariant& var)
{
    m_variants.push_back(var);
}

void PdfVariantStack::Clear()
{
    m_variants.clear();
}
