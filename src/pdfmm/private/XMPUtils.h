/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

#ifndef PDFMM_PDFA_FUNCTIONS_H
#define PDFMM_PDFA_FUNCTIONS_H

#include <pdfmm/base/PdfXMPMetadata.h>
#include <pdfmm/base/PdfXMPPacket.h>

namespace mm
{
    PdfXMPMetadata GetXMPMetadata(const std::string_view& xmpview, std::unique_ptr<PdfXMPPacket>& packet);
    void UpdateOrCreateXMPMetadata(std::unique_ptr<PdfXMPPacket>& packet, const PdfXMPMetadata& metatata);
}

// Low level XMP functions
namespace utls
{
    enum class XMPListType
    {
        LangAlt, ///< ISO 16684-1:2019 "8.2.2.4 Language alternative"
        Seq,
        Bag,
    };

    void SetListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
        const mm::cspan<std::string>& value, xmlNodePtr& newNode);
}

#endif // PDFMM_PDFA_FUNCTIONS_H
