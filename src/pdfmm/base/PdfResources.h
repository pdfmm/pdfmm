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
    PdfDictionaryIndirectIterator GetResourceIterator(const PdfName& type);
    const PdfDictionaryIndirectIterator GetResourceIterator(const PdfName& type) const;
    void RemoveResource(const PdfName& type, const PdfName& key);

    PdfObject* GetResource(const PdfName& type, const PdfName& key);
    const PdfObject* GetResource(const PdfName& type, const PdfName& key) const;
private:
    PdfObject* getResource(const PdfName& type, const PdfName& key) const;
    bool tryGetDictionary(const PdfName& type, PdfDictionary*& dict) const;
};

};

#endif // PDF_RESOURCES_H
