/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_RESOURCES_H
#define PDF_RESOURCES_H

#include "PdfElement.h"

namespace mm {

/** A interface that provides a wrapper around /Resources
 */
class PDFMM_API PdfResources : public PdfDictionaryElement
{
public:
    PdfResources(PdfObject& obj);
    PdfResources(PdfDictionary& dict);

public:
    PdfObject* GetFromResources(const PdfName& type, const PdfName& key);
};

};

#endif // PDF_RESOURCES_H
