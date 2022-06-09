#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfVariantStack.h"

using namespace std;
using namespace mm;

void PdfVariantStack::Push(const PdfVariant& var)
{
    m_variants.push_back(var);
}

void PdfVariantStack::Push(PdfVariant&& var)
{
    m_variants.push_back(std::move(var));
}

void PdfVariantStack::Pop()
{
    m_variants.pop_back();
}

void PdfVariantStack::Clear()
{
    m_variants.clear();
}

unsigned PdfVariantStack::GetSize() const
{
    return (unsigned)m_variants.size();
}

const PdfVariant& PdfVariantStack::operator[](size_t index) const
{
    // Access elements from the end
    index = (m_variants.size() - 1) - index;
    if (index >= m_variants.size())
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Index {} is out of range", index);

    return m_variants[index];
}

PdfVariantStack::iterator PdfVariantStack::begin() const
{
    // Iterate elements from the end in the regular iteration
    return m_variants.rbegin();
}

PdfVariantStack::iterator PdfVariantStack::end() const
{
    // Iterate elements from the end in the regular iteration
    return m_variants.rend();
}

PdfVariantStack::reverse_iterator PdfVariantStack::rbegin() const
{
    // Iterate elements from the begin the reverse iteration
    return m_variants.begin();
}

PdfVariantStack::reverse_iterator PdfVariantStack::rend() const
{
    // Iterate elements from the begin the reverse iteration
    return m_variants.end();
}

size_t PdfVariantStack::size() const
{
    return m_variants.size();
}
