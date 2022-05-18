/**
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_STRING_STREAM
#define PDF_STRING_STREAM

#include "PdfDeclarations.h"

#include <sstream>

namespace mm
{
    class PDFMM_API PdfStringStream
    {
    public:
        PdfStringStream();

        template <typename T>
        inline PdfStringStream& operator<<(T const& val)
        {
            m_stream << val;
            return *this;
        }

        // This is needed to allow using std::endl
        PdfStringStream& operator<<(
            std::ostream& (*pfn)(std::ostream&));

        PdfStringStream& operator<<(float val);

        PdfStringStream& operator<<(double val);

        std::string str() const;

        void str(const std::string_view& str);

        std::streamsize precision(std::streamsize n);

        std::streamsize precision() const;

        explicit operator std::ostream& () { return m_stream; }

    private:
        std::string m_temp;
        std::ostringstream m_stream;
    };
}

#endif // PDF_STRING_STREAM
