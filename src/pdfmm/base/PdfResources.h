/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_RESOURCES_H
#define PDF_RESOURCES_H

#include "PdfElement.h"
#include "PdfDictionary.h"

namespace mm {

/** A interface that provides a wrapper around /Resources
 */
class PDFMM_API PdfResources : public PdfDictionaryElement
{
public:
    PdfResources(PdfObject& obj);
    PdfResources(PdfDictionary& dict);

public:
    void AddResource(const PdfName& type, const PdfName& key, const PdfObject* obj);
    PdfDictionaryIndirectIterable GetResourceIterator(const std::string_view& type);
    PdfDictionaryConstIndirectIterable GetResourceIterator(const std::string_view& type) const;
    void RemoveResource(const std::string_view& type, const std::string_view& key);
    PdfObject* GetResource(const std::string_view& type, const std::string_view& key);
    const PdfObject* GetResource(const std::string_view& type, const std::string_view& key) const;
private:
    PdfObject* getResource(const std::string_view& type, const std::string_view& key) const;
    bool tryGetDictionary(const std::string_view& type, PdfDictionary*& dict) const;
};

};

#endif // PDF_RESOURCES_H
