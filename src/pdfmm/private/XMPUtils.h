#ifndef PDFMM_PDFA_FUNCTIONS_H
#define PDFMM_PDFA_FUNCTIONS_H

#include <pdfmm/base/PdfXMPMetadata.h>
#include <pdfmm/base/PdfXMPPacket.h>

namespace mm
{
    PdfXMPMetadata PDFMM_API GetXMPMetadata(const std::string_view& xmpview, std::unique_ptr<PdfXMPPacket>& packet);
    void PDFMM_API UpdateOrCreateXMPMetadata(std::unique_ptr<PdfXMPPacket>& packet, const PdfXMPMetadata& metatata);
}

#endif // PDFMM_PDFA_FUNCTIONS_H
