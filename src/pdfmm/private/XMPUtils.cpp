#include "PdfDefinesPrivate.h"
#include "XMPUtils.h"
#include "XmlUtils.h"
#include <libxml/xmlsave.h>

using namespace std;
using namespace mm;

#define THROW_LIBXML_EXCEPTION(msg)\
{\
    xmlErrorPtr error_ = xmlGetLastError();\
    if (error_ == nullptr)\
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadata, msg);\
    else\
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::XmpMetadata, "{}, internal error: {}", msg, error_->message);\
}

enum class MetadataKind
{
    Title,
    Author,
    Subject,
    Keywords,
    Creator,
    Producer,
    CreationDate,
    ModDate,
    PdfALevel,
    PdfAConformance,
};

enum class PdfANamespaceKind
{
    Dc,
    Pdf,
    Xmp,
    PdfAId,
};

static void setXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, const PdfDocumentMetadata& metatata, PdfALevel pdfaLevel);
static xmlDocPtr createXmpDoc(xmlNodePtr& root);
static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    MetadataKind property, const string_view& value);
static void removeXMPProperty(xmlNodePtr rdf, MetadataKind property);
static PdfALevel getPdfALevel(const xmlNodePtr root);
static xmlNodePtr findRootXMPMeta(xmlDocPtr doc);
static xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind);
static xmlNodePtr findOrCreateRDFElement(xmlDocPtr doc, xmlNodePtr xmpmeta);
static void ensureRDFNamespace(xmlDocPtr doc, xmlNodePtr element, bool create = false);
static xmlNodePtr createRDFElement(xmlDocPtr doc, xmlNodePtr xmpmeta);
static void createRDFNamespace(xmlNodePtr rdf);
static xmlNodePtr findOrCreateDescriptionElement(xmlDocPtr doc, xmlNodePtr rdf);
static xmlNodePtr createDescriptionElement(xmlDocPtr doc, xmlNodePtr xmpmeta);
static string serializeXMPMetadata(xmlDocPtr xmpMeta);
static PdfALevel getPDFALevelFromString(const string_view& level);
static void getPdfALevelComponents(PdfALevel level, string& levelStr, string& conformanceStr);
static int xmlOutputStringWriter(void* context, const char* buffer, int len);
static int xmlOutputStringWriterClose(void* context);

string mm::UpdateXmpModDate(const string_view& xmpview, const PdfDate& modDate)
{
    utls::InitXml();
    unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> xmlDoc(
        xmlReadMemory(xmpview.data(), (int)xmpview.size(), nullptr, nullptr, 0),
        &xmlFreeDoc);
    xmlNodePtr xmpmeta;
    PdfALevel version;
    if (xmlDoc == nullptr
        || (xmpmeta = findRootXMPMeta(xmlDoc.get())) == nullptr
        || (version = getPdfALevel(xmpmeta)) == PdfALevel::Unknown)
    {
        // If the XMP metadata is missing or has insufficient data
        //  to determine a PDF/A level, ignore updating it
        return { };
    }

    auto rdf = findOrCreateRDFElement(xmlDoc.get(), xmpmeta);
    removeXMPProperty(rdf, MetadataKind::ModDate);
    auto description = findOrCreateDescriptionElement(xmlDoc.get(), rdf);
    addXMPProperty(xmlDoc.get(), description, MetadataKind::ModDate, modDate.ToStringW3C().GetString());
    return serializeXMPMetadata(xmlDoc.get());
}

PdfALevel mm::GetPdfALevel(const string_view& xmpview)
{
    utls::InitXml();
    unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> xmpmeta(
        xmlReadMemory(xmpview.data(), (int)xmpview.size(), nullptr, nullptr, 0),
        &xmlFreeDoc);
    xmlNodePtr root;
    if (xmpmeta == nullptr || (root = findRootXMPMeta(xmpmeta.get())) == nullptr)
    {
        // The the XMP metadata is missing or has insufficient data
        // to determine a PDF/A level
        return PdfALevel::Unknown;
    }

    return getPdfALevel(root);
}

string mm::UpdateOrCreateXMPMetadata(const string_view& xmpview, const PdfDocumentMetadata& metatata, PdfALevel pdfaLevel)
{
    utls::InitXml();
    xmlDocPtr docXml;
    xmlNodePtr xmpmeta;
    if (xmpview.length() == 0
        || (docXml = xmlReadMemory(xmpview.data(), (int)xmpview.size(), nullptr, nullptr, 0)) == nullptr
        || (xmpmeta = findRootXMPMeta(docXml)) == nullptr)
    {
        docXml = createXmpDoc(xmpmeta);
    }

    unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> free(docXml, xmlFreeDoc);
    setXMPMetadata(docXml, xmpmeta, metatata, pdfaLevel);
    return serializeXMPMetadata(docXml);
}

void setXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, const PdfDocumentMetadata& metatata, PdfALevel pdfaLevel)
{
    auto rdf = findOrCreateRDFElement(doc, xmpmeta);
    removeXMPProperty(rdf, MetadataKind::Title);
    removeXMPProperty(rdf, MetadataKind::Author);
    removeXMPProperty(rdf, MetadataKind::Subject);
    removeXMPProperty(rdf, MetadataKind::Keywords);
    removeXMPProperty(rdf, MetadataKind::Creator);
    removeXMPProperty(rdf, MetadataKind::Producer);
    removeXMPProperty(rdf, MetadataKind::CreationDate);
    removeXMPProperty(rdf, MetadataKind::ModDate);
    removeXMPProperty(rdf, MetadataKind::PdfALevel);
    removeXMPProperty(rdf, MetadataKind::PdfAConformance);
    auto description = findOrCreateDescriptionElement(doc, rdf);
    if (metatata.Title.has_value())
        addXMPProperty(doc, description, MetadataKind::Title, metatata.Title->GetString());
    if (metatata.Creator.has_value())
        addXMPProperty(doc, description, MetadataKind::Creator, metatata.Creator->GetString());
    if (metatata.Subject.has_value())
        addXMPProperty(doc, description, MetadataKind::Subject, metatata.Subject->GetString());
    if (metatata.Keywords.has_value())
        addXMPProperty(doc, description, MetadataKind::Keywords, metatata.Keywords->GetString());
    if (metatata.Creator.has_value())
        addXMPProperty(doc, description, MetadataKind::Creator, metatata.Creator->GetString());
    if (metatata.Producer.has_value())
        addXMPProperty(doc, description, MetadataKind::Producer, metatata.Producer->GetString());
    if (metatata.CreationDate.has_value())
        addXMPProperty(doc, description, MetadataKind::CreationDate, metatata.CreationDate->ToStringW3C().GetString());
    if (metatata.ModDate.has_value())
        addXMPProperty(doc, description, MetadataKind::ModDate, metatata.ModDate->ToStringW3C().GetString());

    // Set actual PdfA level
    string levelStr;
    string conformanceStr;
    getPdfALevelComponents(pdfaLevel, levelStr, conformanceStr);
    addXMPProperty(doc, description, MetadataKind::PdfALevel, levelStr);
    addXMPProperty(doc, description, MetadataKind::PdfAConformance, conformanceStr);
}

xmlDocPtr createXmpDoc(xmlNodePtr& root)
{
    auto doc = xmlNewDoc(nullptr);

    // https://wwwimages2.adobe.com/content/dam/acom/en/devnet/xmp/pdfs/XMP%20SDK%20Release%20cc-2016-08/XMPSpecificationPart1.pdf
    // See 7.3.2 XMP Packet Wrapper
    auto xpacketBegin = xmlNewPI(BAD_CAST "xpacket", BAD_CAST "begin=\"ï»¿\" id=\"W5M0MpCehiHzreSzNTczkc9d\"");
    xmlAddChild((xmlNodePtr)doc, xpacketBegin);

    // NOTE: x:xmpmeta element does't define any attribute
    // but other attributes can be defined (eg. x:xmptk)
    // and should be ignored by processors
    auto xmpmeta = xmlNewDocNode(doc, nullptr, BAD_CAST "xmpmeta", nullptr);
    if (xmpmeta == nullptr || xmlAddChild((xmlNodePtr)doc, xmpmeta) == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create x:xmpmeta node");

    auto nsAdobeMeta = xmlNewNs(xmpmeta, BAD_CAST "adobe:ns:meta/", BAD_CAST "x");
    if (nsAdobeMeta == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create x namespace");
    xmlSetNs(xmpmeta, nsAdobeMeta);

    auto xpacketEnd = xmlNewPI(BAD_CAST "xpacket", BAD_CAST "end=\"w\"");
    xmlAddChild((xmlNodePtr)doc, xpacketEnd);
    root = xmpmeta;
    return doc;
}

xmlNodePtr findRootXMPMeta(xmlDocPtr doc)
{
    auto root = xmlDocGetRootElement(doc);
    if (root == nullptr || string_view((const char*)root->name) != "xmpmeta")
        return nullptr;

    return root;
}

xmlNodePtr findOrCreateRDFElement(xmlDocPtr doc, xmlNodePtr xmpmeta)
{
    auto rdf = utls::FindChildElement(xmpmeta, "rdf", "RDF");
    if (rdf != nullptr)
    {
        ensureRDFNamespace(doc, rdf, true);
        return rdf;
    }

    return createRDFElement(doc, xmpmeta);
}

void ensureRDFNamespace(xmlDocPtr doc, xmlNodePtr element, bool create)
{
    auto rdfNs = xmlSearchNs(doc, element, BAD_CAST "rdf");
    if (rdfNs == nullptr)
    {
        if (create)
        {
            PDFMM_ASSERT(string_view((const char*)element->name) == "RDF");
            createRDFNamespace(element);
        }
    }
    else
    {
        if (element->ns != rdfNs)
            xmlSetNs(element, rdfNs);
    }
}


xmlNodePtr createRDFElement(xmlDocPtr doc, xmlNodePtr xmpmeta)
{
    auto rdf = xmlNewDocNode(doc, nullptr, BAD_CAST "RDF", nullptr);
    if (rdf == nullptr || xmlAddChild(xmpmeta, rdf) == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create rdf:RDF node");

    createRDFNamespace(rdf);
    return rdf;
}

void createRDFNamespace(xmlNodePtr rdf)
{
    auto rdfNs = xmlNewNs(rdf, BAD_CAST "http://www.w3.org/1999/02/22-rdf-syntax-ns#", BAD_CAST "rdf");
    if (rdfNs == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create rdf namespace");
    xmlSetNs(rdf, rdfNs);
}

xmlNodePtr findOrCreateDescriptionElement(xmlDocPtr doc, xmlNodePtr rdf)
{
    auto description = utls::FindChildElement(rdf, "rdf", "Description");
    if (description != nullptr)
    {
        ensureRDFNamespace(doc, description);
        return description;
    }

    return createDescriptionElement(doc, rdf);
}

xmlNodePtr createDescriptionElement(xmlDocPtr doc, xmlNodePtr rdf)
{
    auto description = xmlNewDocNode(doc, nullptr, BAD_CAST "Description", nullptr);
    if (description == nullptr || xmlAddChild(rdf, description) == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create rdf:Description node");

    auto nsRDF = xmlNewNs(description, BAD_CAST "http://www.w3.org/1999/02/22-rdf-syntax-ns#", BAD_CAST "rdf");
    if (nsRDF == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create rdf namespace");

    xmlSetNs(description, nsRDF);
    xmlSetNsProp(description, nsRDF, BAD_CAST "about", BAD_CAST "");
    return description;
}

xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind)
{
    const char* prefix;
    const char* href;
    switch (nsKind)
    {
        case PdfANamespaceKind::Dc:
            prefix = "dc";
            href = "http://purl.org/dc/elements/1.1/";
            break;
        case PdfANamespaceKind::Pdf:
            prefix = "pdf";
            href = "http://ns.adobe.com/pdf/1.3/";
            break;
        case PdfANamespaceKind::Xmp:
            prefix = "xmp";
            href = "http://ns.adobe.com/xap/1.0/";
            break;
        case PdfANamespaceKind::PdfAId:
            prefix = "pdfaid";
            href = "http://www.aiim.org/pdfa/ns/id/";
            break;
        default:
            throw runtime_error("Unsupported");
    }
    auto xmlNs = xmlSearchNs(doc, description, BAD_CAST prefix);
    if (xmlNs == nullptr)
        xmlNs = xmlNewNs(description, BAD_CAST href, BAD_CAST prefix);

    if (xmlNs == nullptr)
        THROW_LIBXML_EXCEPTION(mm::Format("Can't find or create {} namespace", prefix));

    return xmlNs;
}

PdfALevel getPdfALevel(const xmlNodePtr root)
{
    auto element = utls::FindChildElement(root, "rdf", "RDF");
    if (element == nullptr)
        return PdfALevel::Unknown;

    element = utls::FindChildElement(element, "rdf", "Description");
    while (element != nullptr)
    {
        nullable<string> pdfaid_part;
        nullable<string> pdfaid_conformance;
        xmlNodePtr childElement = nullptr;
        PdfALevel level = PdfALevel::Unknown;

        childElement = utls::FindChildElement(element, "pdfaid", "part");
        if (childElement == nullptr)
            pdfaid_part = utls::FindAttribute(element, "pdfaid", "part");
        else
            pdfaid_part = utls::GetNodeContent(childElement);

        if (!pdfaid_part.has_value())
            goto Next;

        childElement = utls::FindChildElement(element, "pdfaid", "conformance");
        if (childElement == nullptr)
            pdfaid_conformance = utls::FindAttribute(element, "pdfaid", "conformance");
        else
            pdfaid_conformance = utls::GetNodeContent(childElement);

        if (!pdfaid_conformance.has_value())
            goto Next;

        level = getPDFALevelFromString(*pdfaid_part + *pdfaid_conformance);
        if (level != PdfALevel::Unknown)
            return level;

    Next:
        element = utls::FindSiblingNode(element, "rdf", "Description");
    }

    return PdfALevel::Unknown;
}

void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    MetadataKind property, const string_view& value)
{
    xmlNsPtr xmlNs;
    const char* propName;
    switch (property)
    {
        case MetadataKind::Title:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "title";
            break;
        case MetadataKind::Author:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "creator";
            break;
        case MetadataKind::Subject:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "subject";
            break;
        case MetadataKind::Keywords:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Keywords";
            break;
        case MetadataKind::Creator:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "CreatorTool";
            break;
        case MetadataKind::Producer:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Producer";
            break;
        case MetadataKind::CreationDate:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "CreateDate";
            break;
        case MetadataKind::ModDate:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "ModifyDate";
            break;
        case MetadataKind::PdfALevel:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "part";
            break;
        case MetadataKind::PdfAConformance:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "conformance";
            break;
        default:
            throw runtime_error("Unsupported");
    }

    auto element = xmlNewDocNode(doc, xmlNs, BAD_CAST propName, BAD_CAST value.data());
    if (element == nullptr || xmlAddChild(description, element) == nullptr)
        THROW_LIBXML_EXCEPTION(mm::Format("Can't create xmp:{} node", propName));
}

void removeXMPProperty(xmlNodePtr rdf, MetadataKind property)
{
    const char* propname;
    const char* ns;
    switch (property)
    {
        case MetadataKind::Title:
            ns = "dc";
            propname = "title";
            break;
        case MetadataKind::Author:
            ns = "dc";
            propname = "creator";
            break;
        case MetadataKind::Subject:
            ns = "dc";
            propname = "subject";
            break;
        case MetadataKind::Keywords:
            ns = "pdf";
            propname = "Keywords";
            break;
        case MetadataKind::Creator:
            ns = "xmp";
            propname = "CreatorTool";
            break;
        case MetadataKind::Producer:
            ns = "pdf";
            propname = "Producer";
            break;
        case MetadataKind::CreationDate:
            ns = "xmp";
            propname = "CreateDate";
            break;
        case MetadataKind::ModDate:
            ns = "xmp";
            propname = "ModifyDate";
            break;
        case MetadataKind::PdfALevel:
            ns = "pdfaid";
            propname = "part";
            break;
        case MetadataKind::PdfAConformance:
            ns = "pdfaid";
            propname = "conformance";
            break;
        default:
            throw runtime_error("Unsupported");
    }

    xmlNodePtr description = utls::FindChildElement(rdf, "rdf", "Description");
    xmlNodePtr elemModDate = nullptr;
    do
    {
        elemModDate = utls::FindChildElement(description, ns, propname);
        if (elemModDate != nullptr)
            break;

        description = utls::FindSiblingNode(description, "rdf", "Description");
    } while (description != nullptr);

    if (elemModDate != nullptr)
    {
        // Remove the existing ModifyDate. We recreate the element
        xmlUnlinkNode(elemModDate);
        xmlFreeNode(elemModDate);
    }
}

string serializeXMPMetadata(xmlDocPtr xmpMeta)
{
    string ret;
    auto ctx = xmlSaveToIO(xmlOutputStringWriter, xmlOutputStringWriterClose, &ret, nullptr, XML_SAVE_NO_DECL | XML_SAVE_FORMAT);
    if (ctx == nullptr || xmlSaveDoc(ctx, xmpMeta) == -1 || xmlSaveClose(ctx) == -1)
        THROW_LIBXML_EXCEPTION("Can't save XPM fragment");

    return ret;
}

PdfALevel getPDFALevelFromString(const string_view& pdfaid)
{
    if (pdfaid == "1B")
        return PdfALevel::L1B;
    else if (pdfaid == "1A")
        return PdfALevel::L1A;
    else if (pdfaid == "2B")
        return PdfALevel::L2B;
    else if (pdfaid == "2A")
        return PdfALevel::L2A;
    else if (pdfaid == "2U")
        return PdfALevel::L2U;
    else if (pdfaid == "3B")
        return PdfALevel::L3B;
    else if (pdfaid == "3A")
        return PdfALevel::L3A;
    else if (pdfaid == "3U")
        return PdfALevel::L3U;
    else
        return PdfALevel::Unknown;
}

void getPdfALevelComponents(PdfALevel level, string& levelStr, string& conformanceStr)
{
    switch (level)
    {
        case PdfALevel::L1B:
            levelStr = "1";
            conformanceStr = "B";
            return;
        case PdfALevel::L1A:
            levelStr = "1";
            conformanceStr = "A";
            return;
        case PdfALevel::L2B:
            levelStr = "2";
            conformanceStr = "B";
            return;
        case PdfALevel::L2A:
            levelStr = "2";
            conformanceStr = "A";
            return;
        case PdfALevel::L2U:
            levelStr = "2";
            conformanceStr = "U";
            return;
        case PdfALevel::L3B:
            levelStr = "3";
            conformanceStr = "B";
            return;
        case PdfALevel::L3A:
            levelStr = "3";
            conformanceStr = "A";
            return;
        case PdfALevel::L3U:
            levelStr = "3";
            conformanceStr = "U";
            return;
        default:
            throw runtime_error("Unsupported");
    }
}

int xmlOutputStringWriter(void* context, const char* buffer, int len)
{
    auto str = (string*)context;
    str->append(buffer, (size_t)len);
    return len;
}

int xmlOutputStringWriterClose(void* context)
{
    (void)context;
    // Do nothing
    return 0;
}
