#ifndef PDFMM_PDFA_FUNCTIONS_H
#define PDFMM_PDFA_FUNCTIONS_H

#include <pdfmm/base/PdfDeclarations.h>
#include <pdfmm/base/PdfDate.h>

namespace mm
{
    struct PdfDocumentMetadata
    {
        nullable<PdfString> Title;
        nullable<PdfString> Author;
        nullable<PdfString> Subject;
        nullable<PdfString> Keywords;
        nullable< PdfString> Creator;
        nullable<PdfString> Producer;
        nullable<PdfDate> CreationDate;
        nullable<PdfDate> ModDate;
    };

    std::string UpdateXmpModDate(const std::string_view& xmpview, const PdfDate& modDate);
    PdfALevel GetPdfALevel(const std::string_view& xmpview);
    std::string UpdateOrCreateXMPMetadata(const std::string_view& xmpview, const PdfDocumentMetadata& metatata, PdfALevel pdfaLevel);
}

#endif // PDFMM_PDFA_FUNCTIONS_H
