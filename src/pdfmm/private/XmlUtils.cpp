#include "PdfDefinesPrivate.h"
#include "XmlUtils.h"

using namespace std;
using namespace mm;

xmlNodePtr utls::FindChildElement(const xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (auto child = xmlFirstElementChild(element); child; child = xmlNextElementSibling(child))
    {
        if (child->ns != nullptr
            && prefix == (const char*)child->ns->prefix
            && name == (const char*)child->name)
        {
            return child;
        }
    }

    return nullptr;
}

xmlNodePtr utls::FindSiblingNode(const xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (auto sibling = xmlNextElementSibling(element); sibling; sibling = xmlNextElementSibling(sibling))
    {
        if (sibling->ns != nullptr
            && prefix == (const char*)sibling->ns->prefix
            && name == (const char*)sibling->name)
        {
            return sibling;
        }
    }

    return nullptr;
}

nullable<string> utls::FindAttribute(const xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (xmlAttrPtr attribute = element->properties; attribute; attribute = attribute->next)
    {
        if (attribute->ns != nullptr
            && prefix == (const char*)attribute->ns->prefix
            && name == (const char*)attribute->name)
        {
            return GetNodeContent((xmlNodePtr)attribute);
        }
    }

    return { };
}

nullable<string> utls::GetNodeContent(const xmlNodePtr node)
{
    PDFMM_ASSERT(node != nullptr);
    xmlChar* content = xmlNodeGetContent(node);
    if (content == nullptr)
        return { };

    unique_ptr<xmlChar, decltype(xmlFree)> contentFree(content, xmlFree);
    return string((const char*)content);
}

void utls::InitXml()
{
    LIBXML_TEST_VERSION;
}
