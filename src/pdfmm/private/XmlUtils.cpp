#include "PdfDeclarationsPrivate.h"
#include "XmlUtils.h"

using namespace std;
using namespace mm;

xmlNodePtr utls::FindChildElement(xmlNodePtr element, const std::string_view& name)
{
    return FindChildElement(element, { }, name);
}

xmlNodePtr utls::FindChildElement(xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (auto child = xmlFirstElementChild(element); child != nullptr; child = xmlNextElementSibling(child))
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

xmlNodePtr utls::FindSiblingNode(xmlNodePtr element, const std::string_view& name)
{
    return FindSiblingNode(element, { }, name);
}

xmlNodePtr utls::FindSiblingNode(xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (auto sibling = xmlNextElementSibling(element); sibling; sibling = xmlNextElementSibling(sibling))
    {
        if ((prefix.length() == 0 || (sibling->ns != nullptr
                && prefix == (const char*)sibling->ns->prefix))
            && name == (const char*)sibling->name)
        {
            return sibling;
        }
    }

    return nullptr;
}

mm::nullable<string> utls::FindAttribute(xmlNodePtr element, const std::string_view& name)
{
    return FindAttribute(element, { }, name);
}

nullable<string> utls::FindAttribute(xmlNodePtr element, const string_view& prefix, const string_view& name)
{
    for (xmlAttrPtr attr = element->properties; attr != nullptr; attr = attr->next)
    {
        if ((prefix.length() == 0 || (attr->ns != nullptr
                && prefix == (const char*)attr->ns->prefix))
            && name == (const char*)attr->name)
        {
            return GetNodeContent((xmlNodePtr)attr);
        }
    }

    return { };
}

nullable<string> utls::GetNodeContent(xmlNodePtr node)
{
    PDFMM_ASSERT(node != nullptr);
    xmlChar* content = xmlNodeGetContent(node);
    if (content == nullptr)
        return { };

    unique_ptr<xmlChar, decltype(xmlFree)> contentFree(content, xmlFree);
    return string((const char*)content);
}

string utls::GetAttributeValue(xmlAttrPtr attr)
{
    PDFMM_ASSERT(attr != nullptr);
    xmlChar* content = xmlNodeGetContent((xmlNodePtr)attr);
    unique_ptr<xmlChar, decltype(xmlFree)> contentFree(content, xmlFree);
    return string((const char*)content);
}

void utls::InitXml()
{
    LIBXML_TEST_VERSION;
}
