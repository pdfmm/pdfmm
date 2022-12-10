/**
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
 */

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
    xmlNodePtr FindChildElement(xmlNodePtr element, const std::string_view& name);
    xmlNodePtr FindChildElement(xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    xmlNodePtr FindSiblingNode(xmlNodePtr element, const std::string_view& name);
    xmlNodePtr FindSiblingNode(xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    mm::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& name);
    mm::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& prefix, const std::string_view& name);
    mm::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& name, xmlAttrPtr& ptr);
    mm::nullable<std::string> FindAttribute(xmlNodePtr element, const std::string_view& prefix, const std::string_view& name, xmlAttrPtr& ptr);
    mm::nullable<std::string> GetNodeContent(xmlNodePtr element);
    std::string GetAttributeValue(const xmlAttrPtr attr);
}
