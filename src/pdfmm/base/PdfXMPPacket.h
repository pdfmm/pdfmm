/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XMP_PACKET
#define PDF_XMP_PACKET

#include "PdfDeclarations.h"

extern "C"
{
    typedef struct _xmlDoc xmlDoc;
    typedef xmlDoc* xmlDocPtr;
    typedef struct _xmlNode xmlNode;
    typedef xmlNode* xmlNodePtr;
}

namespace mm
{
    class PDFMM_API PdfXMPPacket final
    {
    public:
        PdfXMPPacket();
        PdfXMPPacket(const PdfXMPPacket&) = delete;
        ~PdfXMPPacket();

        static std::unique_ptr<PdfXMPPacket> Create(const std::string_view& xmpview);

    public:
        void ToString(std::string& str) const;
        std::string ToString() const;

    public:
        xmlDocPtr GetDoc() { return m_Doc; }
        const xmlDocPtr GetDoc() const { return m_Doc; }
        xmlNodePtr GetOrCreateDescription();
        const xmlNodePtr GetDescription() const { return m_Description; }

    public:
        PdfXMPPacket& operator=(const PdfXMPPacket&) = delete;

    private:
        PdfXMPPacket(xmlDocPtr doc, xmlNodePtr xmpmeta);

    private:
        xmlDocPtr m_Doc;
        xmlNodePtr m_XMPMeta;
        xmlNodePtr m_Description;
    };
}

#endif // PDF_XMP_PACKET
