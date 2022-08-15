/**
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_STRING_STREAM
#define PDF_STRING_STREAM

#include "PdfDeclarations.h"
#include "PdfOutputStream.h"

namespace mm
{
    /** A specialized Pdf output string stream
     * It suplies an iostream-like operator<< interface,
     * while still inheriting OutputStream
     */
    class PDFMM_API PdfStringStream final : public OutputStream
    {
    public:
        PdfStringStream();

        template <typename T>
        inline PdfStringStream& operator<<(T const& val)
        {
            (*m_stream) << val;
            return *this;
        }

        // This is needed to allow using std::endl
        PdfStringStream& operator<<(
            std::ostream& (*pfn)(std::ostream&));

        PdfStringStream& operator<<(float val);

        PdfStringStream& operator<<(double val);

        std::string_view GetString() const;

        std::string TakeString();

        void Clear();

        void SetPrecision(unsigned short value);

        unsigned short GetPrecision() const;

        unsigned GetSize() const;

        explicit operator std::ostream& () { return *m_stream; }

    protected:
        void writeBuffer(const char* buffer, size_t size);

    private:
        using OutputStream::Flush;
        using OutputStream::Write;

    private:
        charbuff m_temp;
        std::unique_ptr<std::ostream> m_stream;
    };
}

#endif // PDF_STRING_STREAM
