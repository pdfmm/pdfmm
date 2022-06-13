#include "PdfDeclarationsPrivate.h"
#include "XMPUtils.h"

#include <libxml/xmlsave.h>

#include "XmlUtils.h"

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

enum class XMPMetadataKind
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
    PdfARevision,
};

enum class PdfANamespaceKind
{
    Dc,
    Pdf,
    Xmp,
    PdfAId,
};

enum class XMPListType
{
    LangAlt, ///< ISO 16684-1:2019 "8.2.2.4 Language alternative"
    Seq,
    Bag,
};

static unordered_map<string, XMPListType> s_knownListNodes = {
    { "dc:date", XMPListType::Seq },
    { "dc:language", XMPListType::Bag },
};

static void normalizeXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, xmlNodePtr& description);
static void normalizeQualifiersAndValues(xmlDocPtr doc, xmlNsPtr rdfNs, xmlNodePtr node);
static void normalizeElement(xmlNodePtr elem);
static void tryFixArrayElement(xmlDocPtr doc, xmlNodePtr& node, const string& nodeContent);
static bool shouldSkipAttribute(xmlAttrPtr attr, vector<xmlAttrPtr>& attibsToRemove);
static void setXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, const PdfXMPMetadata& metatata);
static xmlDocPtr createXMPDoc(xmlNodePtr& root);
static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const string& value);
static void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const cspan<string>& values);
static void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property);
static xmlNodePtr findRootXMPMeta(xmlDocPtr doc);
static xmlNsPtr findOrCreateNamespace(xmlDocPtr doc, xmlNodePtr description, PdfANamespaceKind nsKind);
static xmlNodePtr createRDFElement(xmlNodePtr xmpmeta);
static void createRDFNamespace(xmlNodePtr rdf);
static xmlNodePtr createDescriptionElement(xmlNodePtr xmpmeta);
static void serializeXMPMetadataTo(string& str, xmlDocPtr xmpMeta);
static PdfALevel getPDFALevelFromString(const string_view& level);
static void getPdfALevelComponents(PdfALevel level, string& levelStr, string& conformanceStr, string& revision);
static int xmlOutputStringWriter(void* context, const char* buffer, int len);
static int xmlOutputStringWriterClose(void* context);
static void setListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
    const cspan<string>& value, xmlNodePtr& newNode);
static string getNodeName(xmlNodePtr node);
static string getAttributeName(xmlAttrPtr attr);
static nullable<PdfString> getListElementText(xmlNodePtr elem);
static nullable<PdfString> getElementText(xmlNodePtr elem);

PdfXMPMetadata mm::GetXMPMetadata(const string_view& xmpview, unique_ptr<PdfXMPPacket>& packet)
{
    utls::InitXml();

    PdfXMPMetadata metadata;
    xmlNodePtr description;
    packet = PdfXMPPacket::Create(xmpview);
    if (packet == nullptr || (description = packet->GetDescription()) == nullptr)
    {
        // The the XMP metadata is missing or has insufficient data
        // to determine a PDF/A level
        return metadata;
    }

    xmlNodePtr childElement = nullptr;
    nullable<PdfString> text;

    {
        nullable<string> pdfaid_part;
        nullable<string> pdfaid_conformance;

        childElement = utls::FindChildElement(description, "pdfaid", "part");
        if (childElement != nullptr)
            pdfaid_part = utls::GetNodeContent(childElement);
        childElement = utls::FindChildElement(description, "pdfaid", "conformance");
        if (childElement != nullptr)
            pdfaid_conformance = utls::GetNodeContent(childElement);

        if (pdfaid_part.has_value() && pdfaid_conformance.has_value())
            metadata.PdfaLevel = getPDFALevelFromString(*pdfaid_part + *pdfaid_conformance);
    }

    childElement = utls::FindChildElement(description, "dc", "title");
    if (childElement != nullptr)
        metadata.Title = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "dc", "creator");
    if (childElement != nullptr)
        metadata.Author = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "dc", "description");
    if (childElement != nullptr)
        metadata.Subject = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "pdf", "Keywords");
    if (childElement != nullptr)
        metadata.Keywords = getElementText(childElement);

    childElement = utls::FindChildElement(description, "xmp", "CreatorTool");
    if (childElement != nullptr)
        metadata.Creator = getListElementText(childElement);

    childElement = utls::FindChildElement(description, "pdf", "Producer");
    if (childElement != nullptr)
        metadata.Producer = getElementText(childElement);

    childElement = utls::FindChildElement(description, "xmp", "CreateDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr)
        metadata.CreationDate = PdfDate::ParseW3C(*text);

    childElement = utls::FindChildElement(description, "xmp", "ModifyDate");
    if (childElement != nullptr && (text = getElementText(childElement)) != nullptr)
        metadata.ModDate = PdfDate::ParseW3C(*text);

    return metadata;
}

void mm::UpdateOrCreateXMPMetadata(unique_ptr<PdfXMPPacket>& packet, const PdfXMPMetadata& metatata)
{
    utls::InitXml();
    if (packet == nullptr)
        packet.reset(new PdfXMPPacket());

    setXMPMetadata(packet->GetDoc(), packet->GetOrCreateDescription(), metatata);
}

// Normalize XMP accordingly to ISO 16684-2:2014
void normalizeXMPMetadata(xmlDocPtr doc, xmlNodePtr xmpmeta, xmlNodePtr& description)
{
    auto rdf = utls::FindChildElement(xmpmeta, "rdf", "RDF");
    if (rdf == nullptr)
    {
        description = nullptr;
        return;
    }

    normalizeQualifiersAndValues(doc, rdf->ns, rdf);

    description = utls::FindChildElement(rdf, "rdf", "Description");
    if (description == nullptr)
        return;

    // Merge top level rdf:Description elements
    vector<xmlNodePtr> descriptionsToRemove;
    auto element = description;
    while (true)
    {
        element = utls::FindSiblingNode(element, "rdf", "Description");
        if (element == nullptr)
            break;
        else
            descriptionsToRemove.push_back(element);

        vector<xmlNodePtr> childrenToMove;
        for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
            childrenToMove.push_back(child);

        for (auto child : childrenToMove)
        {
            xmlUnlinkNode(child);
            xmlAddChild(description, child);
        }
    }

    if (xmlReconciliateNs(doc, description) == -1)
        THROW_LIBXML_EXCEPTION("Error fixing namespaces");

    // Finally remove spurious rdf:Description elements
    for (auto descToRemove : descriptionsToRemove)
    {
        xmlUnlinkNode(descToRemove);
        xmlFreeNode(descToRemove);
    }
}

void normalizeQualifiersAndValues(xmlDocPtr doc, xmlNsPtr rdfNs, xmlNodePtr elem)
{
    // TODO: Convert RDF Typed nodes from rdf:type notation with rdf:resource
    // As specified in 16684-2:2014:
    // "The RDF TypedNode notation defined in ISO 16684-1:2012,
    // 7.9.2.5 shall not be used for an rdf:type qualifier".
    // Eg. from:
    //      <rdf:Description>
    //          <rdf:type rdf:resource="http://ns.adobe.com/xmp-example/myType"/>
    //          <xe:Field>value</xe:Field>
    //      </rdf:Description>
    //
    // To:
    //     <xe:MyType>
    //         <xe:Field>value</xe:Field>
    //     </xe:MyType>

    auto child = xmlFirstElementChild(elem);
    nullable<string> content;
    if (child == nullptr
        && (elem->children == nullptr || elem->children->type != XML_COMMENT_NODE)
        && (content = utls::GetNodeContent(elem)) != nullptr
        && !utls::IsStringEmptyOrWhiteSpace(*content))
    {
        // Some elements are arrays but they don't use
        // proper array notation
        tryFixArrayElement(doc, elem, *content);
        normalizeElement(elem);
        return;
    }

    normalizeElement(elem);
    for (; child != nullptr; child = xmlNextElementSibling(child))
        normalizeQualifiersAndValues(doc, rdfNs, child);
}

void normalizeElement(xmlNodePtr elem)
{
    // ISO 16684-2:2014 "5.3 Property serialization"
    // ISO 16684-2:2014 "5.6 Qualifier serialization".
    // Try to convert XMP simple properties and qualifiers to elements
    vector<xmlAttrPtr> attribsToRemove;
    for (xmlAttrPtr attr = elem->properties; attr != nullptr; attr = attr->next)
    {
        if (shouldSkipAttribute(attr, attribsToRemove))
            continue;

        auto value = utls::GetAttributeValue(attr);
        if (xmlNewChild(elem, attr->ns, attr->name, XMLCHAR value.data()) == nullptr)
            THROW_LIBXML_EXCEPTION("Can't create value replacement node");

        attribsToRemove.push_back(attr);
    }

    for (auto attr : attribsToRemove)
        xmlRemoveProp(attr);
}

// ISO 16684-2:2014 "6.3.3 Array value data types"
void tryFixArrayElement(xmlDocPtr doc, xmlNodePtr& node, const string& nodeContent)
{
    if (node->ns == nullptr)
        return;

    auto nodename = getNodeName(node);
    auto found = s_knownListNodes.find(nodename);
    if (found == s_knownListNodes.end())
        return;

    // Delete existing content
    xmlNodeSetContent(node, nullptr);

    xmlNodePtr newNode;
    setListNodeContent(doc, node, found->second, cspan<string>(&nodeContent, 1), newNode);
    node = newNode;
}

bool shouldSkipAttribute(xmlAttrPtr attr, vector<xmlAttrPtr>& attibsToRemove)
{
    auto attrname = getAttributeName(attr);
    if (attrname == "xml:lang")
    {
        return true;
    }
    else if (attrname == "rdf:about")
    {
        return true;
    }
    else if (attrname == "rdf:resource")
    {
        // ISO 16684-1:2019 "7.5 Simple valued XMP properties"
        // The element content for an XMP property with a
        // URI simple value shall be empty. The value shall be
        // provided as the value of an rdf : resource attribute
        // attached to the XML element.
        return true;
    }
    else if (attrname == "rdf:parseType")
    {
        // ISO 16684-2:2014 "5.6 Qualifier serialization"
        attibsToRemove.push_back(attr);
        return true;
    }
    else
    {
        return false;
    }
}

void setXMPMetadata(xmlDocPtr doc, xmlNodePtr description, const PdfXMPMetadata& metatata)
{
    removeXMPProperty(description, XMPMetadataKind::Title);
    removeXMPProperty(description, XMPMetadataKind::Author);
    removeXMPProperty(description, XMPMetadataKind::Subject);
    removeXMPProperty(description, XMPMetadataKind::Keywords);
    removeXMPProperty(description, XMPMetadataKind::Creator);
    removeXMPProperty(description, XMPMetadataKind::Producer);
    removeXMPProperty(description, XMPMetadataKind::CreationDate);
    removeXMPProperty(description, XMPMetadataKind::ModDate);
    removeXMPProperty(description, XMPMetadataKind::PdfALevel);
    removeXMPProperty(description, XMPMetadataKind::PdfAConformance);
    removeXMPProperty(description, XMPMetadataKind::PdfARevision);
    if (metatata.Title.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Title, metatata.Title->GetString());
    if (metatata.Author.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Author, metatata.Author->GetString());
    if (metatata.Subject.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Subject, metatata.Subject->GetString());
    if (metatata.Keywords.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Keywords, metatata.Keywords->GetString());
    if (metatata.Creator.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Creator, metatata.Creator->GetString());
    if (metatata.Producer.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::Producer, metatata.Producer->GetString());
    if (metatata.CreationDate.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::CreationDate, metatata.CreationDate->ToStringW3C().GetString());
    if (metatata.ModDate.has_value())
        addXMPProperty(doc, description, XMPMetadataKind::ModDate, metatata.ModDate->ToStringW3C().GetString());

    if (metatata.PdfaLevel != PdfALevel::Unknown)
    {
        // Set actual PdfA level
        string levelStr;
        string conformanceStr;
        string revision;
        getPdfALevelComponents(metatata.PdfaLevel, levelStr, conformanceStr, revision);
        addXMPProperty(doc, description, XMPMetadataKind::PdfALevel, levelStr);
        addXMPProperty(doc, description, XMPMetadataKind::PdfAConformance, conformanceStr);
        if (revision.length() != 0)
            addXMPProperty(doc, description, XMPMetadataKind::PdfARevision, revision);
    }
}

xmlDocPtr createXMPDoc(xmlNodePtr& root)
{
    auto doc = xmlNewDoc(nullptr);

    // https://wwwimages2.adobe.com/content/dam/acom/en/devnet/xmp/pdfs/XMP%20SDK%20Release%20cc-2016-08/XMPSpecificationPart1.pdf
    // See 7.3.2 XMP Packet Wrapper
    auto xpacketBegin = xmlNewPI(XMLCHAR "xpacket", XMLCHAR "begin=\"\357\273\277\" id=\"W5M0MpCehiHzreSzNTczkc9d\"");
    if (xpacketBegin == nullptr || xmlAddChild((xmlNodePtr)doc, xpacketBegin) == nullptr)
    {
        xmlFreeNode(xpacketBegin);
        THROW_LIBXML_EXCEPTION("Can't create xpacket begin node");
    }

    // NOTE: x:xmpmeta element does't define any attribute
    // but other attributes can be defined (eg. x:xmptk)
    // and should be ignored by processors
    auto xmpmeta = xmlNewChild((xmlNodePtr)doc, nullptr, XMLCHAR "xmpmeta", nullptr);
    if (xmpmeta == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create x:xmpmeta node");

    auto nsAdobeMeta = xmlNewNs(xmpmeta, XMLCHAR "adobe:ns:meta/", XMLCHAR "x");
    if (nsAdobeMeta == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create x namespace");
    xmlSetNs(xmpmeta, nsAdobeMeta);

    auto xpacketEnd = xmlNewPI(XMLCHAR "xpacket", XMLCHAR "end=\"w\"");
    if (xpacketEnd == nullptr || xmlAddChild((xmlNodePtr)doc, xpacketEnd) == nullptr)
    {
        xmlFreeNode(xpacketEnd);
        THROW_LIBXML_EXCEPTION("Can't create xpacket end node");
    }

    root = xmpmeta;
    return doc;
}

xmlNodePtr findRootXMPMeta(xmlDocPtr doc)
{
    auto root = xmlDocGetRootElement(doc);
    if (root == nullptr || (const char*)root->name != "xmpmeta"sv)
        return nullptr;

    return root;
}


xmlNodePtr createRDFElement(xmlNodePtr xmpmeta)
{
    auto rdf = xmlNewChild(xmpmeta, nullptr, XMLCHAR "RDF", nullptr);
    if (rdf == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create rdf:RDF node");

    createRDFNamespace(rdf);
    return rdf;
}

void createRDFNamespace(xmlNodePtr rdf)
{
    auto rdfNs = xmlNewNs(rdf, XMLCHAR "http://www.w3.org/1999/02/22-rdf-syntax-ns#", XMLCHAR "rdf");
    if (rdfNs == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create rdf namespace");
    xmlSetNs(rdf, rdfNs);
}

xmlNodePtr createDescriptionElement(xmlNodePtr rdf)
{
    auto description = xmlNewChild(rdf, nullptr, XMLCHAR "Description", nullptr);
    if (description == nullptr)
        THROW_LIBXML_EXCEPTION("Can't create rdf:Description node");

    auto nsRDF = xmlNewNs(description, XMLCHAR "http://www.w3.org/1999/02/22-rdf-syntax-ns#", XMLCHAR "rdf");
    if (nsRDF == nullptr)
        THROW_LIBXML_EXCEPTION("Can't find or create rdf namespace");

    xmlSetNs(description, nsRDF);
    if (xmlSetNsProp(description, nsRDF, XMLCHAR "about", XMLCHAR "") == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't set rdf:about attribute on rdf:Description node"));

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
    auto xmlNs = xmlSearchNs(doc, description, XMLCHAR prefix);
    if (xmlNs == nullptr)
        xmlNs = xmlNewNs(description, XMLCHAR href, XMLCHAR prefix);

    if (xmlNs == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't find or create {} namespace", prefix));

    return xmlNs;
}

void addXMPProperty(xmlDocPtr doc, xmlNodePtr description, XMPMetadataKind prop, const string& value)
{
    addXMPProperty(doc, description, prop, cspan<string>(&value, 1));
}

void addXMPProperty(xmlDocPtr doc, xmlNodePtr description,
    XMPMetadataKind property, const cspan<string>& values)
{
    xmlNsPtr xmlNs;
    const char* propName;
    switch (property)
    {
        case XMPMetadataKind::Title:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "title";
            break;
        case XMPMetadataKind::Author:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "creator";
            break;
        case XMPMetadataKind::Subject:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Dc);
            propName = "description";
            break;
        case XMPMetadataKind::Keywords:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Keywords";
            break;
        case XMPMetadataKind::Creator:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "CreatorTool";
            break;
        case XMPMetadataKind::Producer:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Pdf);
            propName = "Producer";
            break;
        case XMPMetadataKind::CreationDate:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "CreateDate";
            break;
        case XMPMetadataKind::ModDate:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::Xmp);
            propName = "ModifyDate";
            break;
        case XMPMetadataKind::PdfALevel:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "part";
            break;
        case XMPMetadataKind::PdfAConformance:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "conformance";
            break;
        case XMPMetadataKind::PdfARevision:
            xmlNs = findOrCreateNamespace(doc, description, PdfANamespaceKind::PdfAId);
            propName = "rev";
            break;
        default:
            throw runtime_error("Unsupported");
    }

    auto element = xmlNewChild(description, xmlNs, XMLCHAR propName, nullptr);
    if (element == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't create xmp:{} node", propName));

    switch (property)
    {
        case XMPMetadataKind::Title:
        case XMPMetadataKind::Subject:
        {
            xmlNodePtr newNode;
            setListNodeContent(doc, element, XMPListType::LangAlt, values, newNode);
            break;
        }
        case XMPMetadataKind::Author:
        {
            xmlNodePtr newNode;
            setListNodeContent(doc, element, XMPListType::Seq, values, newNode);
            break;
        }
        default:
        {
            xmlNodeSetContent(element, XMLCHAR values[0].data());
            break;
        }
    }
}

void setListNodeContent(xmlDocPtr doc, xmlNodePtr node, XMPListType seqType,
    const cspan<string>& values, xmlNodePtr& newNode)
{
    const char* elemName;
    switch (seqType)
    {
        case XMPListType::LangAlt:
            elemName = "Alt";
            break;
        case XMPListType::Seq:
            elemName = "Seq";
            break;
        case XMPListType::Bag:
            elemName = "Bag";
            break;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }

    auto rdfNs = xmlSearchNs(doc, node, XMLCHAR "rdf");
    PDFMM_ASSERT(rdfNs != nullptr);
    auto innerElem = xmlNewChild(node, rdfNs, XMLCHAR elemName, nullptr);
    if (innerElem == nullptr)
        THROW_LIBXML_EXCEPTION(utls::Format("Can't create rdf:{} node", elemName));

    for (auto& view : values)
    {
        auto liElem = xmlNewChild(innerElem, rdfNs, XMLCHAR "li", nullptr);
        if (liElem == nullptr)
            THROW_LIBXML_EXCEPTION(utls::Format("Can't create rdf:li node"));

        if (seqType == XMPListType::LangAlt)
        {
            // Set a xml:lang "x-default" attribute, accordingly
            // ISO 16684-1:2019 "8.2.2.4 Language alternative"
            auto xmlNs = xmlSearchNs(doc, node, XMLCHAR "xml");
            PDFMM_ASSERT(xmlNs != nullptr);
            if (xmlSetNsProp(liElem, xmlNs, XMLCHAR "lang", XMLCHAR "x-default") == nullptr)
                THROW_LIBXML_EXCEPTION(utls::Format("Can't set xml:lang attribute on rdf:li node"));
        }

        xmlNodeSetContent(liElem, XMLCHAR view.data());
    }

    newNode = innerElem->children;
}

void removeXMPProperty(xmlNodePtr description, XMPMetadataKind property)
{
    const char* propname;
    const char* ns;
    switch (property)
    {
        case XMPMetadataKind::Title:
            ns = "dc";
            propname = "title";
            break;
        case XMPMetadataKind::Author:
            ns = "dc";
            propname = "creator";
            break;
        case XMPMetadataKind::Subject:
            ns = "dc";
            propname = "description";
            break;
        case XMPMetadataKind::Keywords:
            ns = "pdf";
            propname = "Keywords";
            break;
        case XMPMetadataKind::Creator:
            ns = "xmp";
            propname = "CreatorTool";
            break;
        case XMPMetadataKind::Producer:
            ns = "pdf";
            propname = "Producer";
            break;
        case XMPMetadataKind::CreationDate:
            ns = "xmp";
            propname = "CreateDate";
            break;
        case XMPMetadataKind::ModDate:
            ns = "xmp";
            propname = "ModifyDate";
            break;
        case XMPMetadataKind::PdfALevel:
            ns = "pdfaid";
            propname = "part";
            break;
        case XMPMetadataKind::PdfAConformance:
            ns = "pdfaid";
            propname = "conformance";
            break;
        case XMPMetadataKind::PdfARevision:
            ns = "pdfaid";
            propname = "rev";
            break;
        default:
            throw runtime_error("Unsupported");
    }

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

void serializeXMPMetadataTo(string& str, xmlDocPtr xmpMeta)
{
    auto ctx = xmlSaveToIO(xmlOutputStringWriter, xmlOutputStringWriterClose, &str, nullptr, XML_SAVE_NO_DECL | XML_SAVE_FORMAT);
    if (ctx == nullptr || xmlSaveDoc(ctx, xmpMeta) == -1 || xmlSaveClose(ctx) == -1)
        THROW_LIBXML_EXCEPTION("Can't save XPM fragment");
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
    else if (pdfaid == "4E")
        return PdfALevel::L4E;
    else if (pdfaid == "4F")
        return PdfALevel::L4F;
    else
        return PdfALevel::Unknown;
}

void getPdfALevelComponents(PdfALevel level, string& levelStr, string& conformanceStr, string& revision)
{
    switch (level)
    {
        case PdfALevel::L1B:
            levelStr = "1";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L1A:
            levelStr = "1";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L2B:
            levelStr = "2";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L2A:
            levelStr = "2";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L2U:
            levelStr = "2";
            conformanceStr = "U";
            revision.clear();
            return;
        case PdfALevel::L3B:
            levelStr = "3";
            conformanceStr = "B";
            revision.clear();
            return;
        case PdfALevel::L3A:
            levelStr = "3";
            conformanceStr = "A";
            revision.clear();
            return;
        case PdfALevel::L3U:
            levelStr = "3";
            conformanceStr = "U";
            revision.clear();
            return;
        case PdfALevel::L4E:
            levelStr = "4";
            conformanceStr = "E";
            revision = "2020";
            return;
        case PdfALevel::L4F:
            levelStr = "4";
            conformanceStr = "F";
            revision = "2020";
            return;
        default:
            throw runtime_error("Unsupported");
    }
}

string getNodeName(xmlNodePtr node)
{
    if (node->ns == nullptr)
    {
        return (const char*)node->name;
    }
    else
    {
        string nodename((const char*)node->ns->prefix);
        nodename.push_back(':');
        nodename.append((const char*)node->name);
        return nodename;
    }
}

string getAttributeName(xmlAttrPtr attr)
{
    return getNodeName((xmlNodePtr)attr);
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

nullable<PdfString> getListElementText(xmlNodePtr elem)
{
    auto listNode = xmlFirstElementChild(elem);
    if (listNode == nullptr)
        return nullptr;

    auto liNode = xmlFirstElementChild(listNode);
    if (liNode == nullptr)
        return nullptr;

    return getElementText(liNode);
}

nullable<PdfString> getElementText(xmlNodePtr elem)
{
    auto text = utls::GetNodeContent(elem);
    if (text == nullptr)
        return nullptr;
    else
        return PdfString(*text);
}

PdfXMPPacket::PdfXMPPacket()
    : m_Description(nullptr)
{
    m_Doc = createXMPDoc(m_XMPMeta);
}

PdfXMPPacket::PdfXMPPacket(xmlDocPtr doc, xmlNodePtr xmpmeta)
    : m_Doc(doc), m_XMPMeta(xmpmeta), m_Description(nullptr) { }

PdfXMPPacket::~PdfXMPPacket()
{
    xmlFreeDoc(m_Doc);
}

unique_ptr<PdfXMPPacket> PdfXMPPacket::Create(const string_view& xmpview)
{
    auto doc = xmlReadMemory(xmpview.data(), (int)xmpview.size(), nullptr, nullptr, XML_PARSE_NOBLANKS);
    xmlNodePtr xmpmeta;
    if (doc == nullptr
        || (xmpmeta = findRootXMPMeta(doc)) == nullptr)
    {
        xmlFreeDoc(doc);
        return nullptr;
    }

    unique_ptr<PdfXMPPacket> ret(new PdfXMPPacket(doc, xmpmeta));
    normalizeXMPMetadata(doc, xmpmeta, ret->m_Description);
    return ret;
}


xmlNodePtr PdfXMPPacket::GetOrCreateDescription()
{
    if (m_Description != nullptr)
        return m_Description;

    auto rdf = utls::FindChildElement(m_XMPMeta, "rdf", "RDF");
    if (rdf == nullptr)
        rdf = createRDFElement(m_XMPMeta);

    auto description = utls::FindChildElement(rdf, "rdf", "Description");
    if (description == nullptr)
        description = createDescriptionElement(rdf);

    m_Description = description;
    return description;
}

void PdfXMPPacket::ToString(string& str) const
{
    serializeXMPMetadataTo(str, m_Doc);
}

string PdfXMPPacket::ToString() const
{
    string ret;
    ToString(ret);
    return ret;
}
