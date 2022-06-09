/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XMP_METADATA
#define PDF_XMP_METADATA

#include "PdfString.h"
#include "PdfDate.h"

namespace mm
{
    struct PDFMM_API PdfXMPMetadata
    {
        PdfXMPMetadata();
        nullable<PdfString> Title;
        nullable<PdfString> Author;
        nullable<PdfString> Subject;
        nullable<PdfString> Keywords;
        nullable<PdfString> Creator;
        nullable<PdfString> Producer;
        nullable<PdfDate> CreationDate;
        nullable<PdfDate> ModDate;
        PdfALevel PdfaLevel;
    };
}

#endif // PDF_XMP_METADATA
