#pragma once

#include <string>
#include <libxml/tree.h>
#include <pdfmm/common/nullable.h>

namespace utls
{
    void InitXml();
    xmlNodePtr FindChildElement(const xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    xmlNodePtr FindSiblingNode(const xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    mm::nullable<std::string> FindAttribute(const xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    mm::nullable<std::string> GetNodeContent(const xmlNodePtr element);
}
