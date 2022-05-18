/**
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfStringStream.h"

#include "PdfLocale.h"

using namespace std;
using namespace mm;

PdfStringStream& PdfStringStream::operator<<(float val)
{
    utls::FormatTo(m_temp, val, (unsigned char)precision());
    m_stream << m_temp;
    return *this;
}

PdfStringStream& PdfStringStream::operator<<(double val)
{
    utls::FormatTo(m_temp, val, (unsigned char)precision());
    m_stream << m_temp;
    return *this;
}

PdfStringStream::PdfStringStream()
{
    mm::PdfLocaleImbue(m_stream);
}

PdfStringStream& PdfStringStream::operator<<(
    std::ostream& (*pfn)(std::ostream&))
{
    pfn(m_stream);
    return *this;
}

string PdfStringStream::str() const
{
    return m_stream.str();
}

void PdfStringStream::str(const string_view& st)
{
    m_stream.str(st.data());
}

streamsize PdfStringStream::precision(streamsize n)
{
    return m_stream.precision(n);
}

streamsize PdfStringStream::precision() const
{
    return m_stream.precision();
}
