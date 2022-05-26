#ifndef PDFMM_PDFA_FUNCTIONS_H
#define PDFMM_PDFA_FUNCTIONS_H

#include <pdfmm/base/PdfDeclarations.h>
#include <pdfmm/base/PdfDate.h>

extern "C"
{
    typedef struct _xmlDoc xmlDoc;
    typedef xmlDoc* xmlDocPtr;
    typedef struct _xmlNode xmlNode;
    typedef xmlNode* xmlNodePtr;
}

namespace mm
{
    struct PdfXMPMetadata
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

    // REMOVE-ME: Switch to use GetXMPMetadata
    std::string UpdateXMPModDate(const std::string_view& xmpview, const PdfDate& modDate);

    // REMOVE-ME: Switch to use GetXMPMetadata
    PdfALevel GetPdfALevel(const std::string_view& xmpview);

    PdfXMPMetadata PDFMM_API GetXMPMetadata(const std::string_view& xmpview, std::shared_ptr<PdfXMPPacket>& packet);
    void PDFMM_API UpdateOrCreateXMPMetadata(std::shared_ptr<PdfXMPPacket>& packet, const PdfXMPMetadata& metatata);
}

#endif // PDFMM_PDFA_FUNCTIONS_H
