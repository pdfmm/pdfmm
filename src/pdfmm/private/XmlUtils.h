#pragma once

#include <string>
#include <libxml/tree.h>
#include <pdfmm/common/nullable.h>

// Cast macro that keep or enforce const to use
// instead of BAD_CAST
#define XMLCHAR (const xmlChar*)

namespace utls
{
    void InitXml();
    xmlNodePtr FindChildElement(const xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    xmlNodePtr FindSiblingNode(const xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    mm::nullable<std::string> FindAttribute(const xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    mm::nullable<std::string> GetNodeContent(const xmlNodePtr element);
}
