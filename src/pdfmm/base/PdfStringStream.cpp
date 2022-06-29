/**
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfStringStream.h"

#include <pdfmm/private/outstringstream.h>

using namespace std;
using namespace cmn;
using namespace mm;

PdfStringStream::PdfStringStream()
    : m_stream(new outstringstream())
{
    m_stream->imbue(utls::GetInvariantLocale());
}

PdfStringStream& PdfStringStream::operator<<(float val)
{
    utls::FormatTo(m_temp, val, (unsigned short)m_stream->precision());
    (*m_stream) << m_temp;
    return *this;
}

PdfStringStream& PdfStringStream::operator<<(double val)
{
    utls::FormatTo(m_temp, val, (unsigned char)m_stream->precision());
    (*m_stream) << m_temp;
    return *this;
}

PdfStringStream& PdfStringStream::operator<<(
    std::ostream& (*pfn)(std::ostream&))
{
    pfn(*m_stream);
    return *this;
}

string_view PdfStringStream::GetString() const
{
    return static_cast<const outstringstream&>(*m_stream).str();
}

string PdfStringStream::TakeString()
{
    return static_cast<outstringstream&>(*m_stream).take_str();
}

void PdfStringStream::Clear()
{
    static_cast<outstringstream&>(*m_stream).clear();
    m_temp.clear();
}

void PdfStringStream::SetPrecision(unsigned short value)
{
    (void)m_stream->precision(value);
}

unsigned short PdfStringStream::GetPrecision() const
{
    return (unsigned short)m_stream->precision();
}

unsigned PdfStringStream::GetSize() const
{
    return (unsigned)static_cast<const outstringstream&>(*m_stream).size();
}
