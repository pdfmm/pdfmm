/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OBJECT_STREAM_PARSER_OBJECT_H
#define PDF_OBJECT_STREAM_PARSER_OBJECT_H

#include "PdfDeclarations.h"

#include "PdfParserObject.h"

namespace mm {

class PdfEncrypt;
class PdfIndirectObjectList;

/**
 * A utility class for PdfParser that can parse
 * an object stream object (PDF Reference 1.7 3.4.6 Object Streams)
 *
 * It is mainly here to make PdfParser more modular.
 */
class PdfObjectStreamParser
{
public:
    using ObjectIdList = std::vector<int64_t>;
    /**
     * Create a new PdfObjectStreamParserObject from an existing
     * PdfParserObject. The PdfParserObject will be removed and deleted.
     * All objects from the object stream will be read into memory.
     *
     * \param parser PdfParserObject for an object stream
     * \param objects add loaded objects to this vector of objects
     * \param buffer use this allocated buffer for caching
     */
    PdfObjectStreamParser(PdfParserObject& parser, PdfIndirectObjectList& objects, const std::shared_ptr<charbuff>& buffer);

    void Parse(ObjectIdList const&);

private:
    void ReadObjectsFromStream(char* buffer, size_t lBufferLen, int64_t lNum, int64_t lFirst, ObjectIdList const&);

private:
    PdfParserObject* m_Parser;
    PdfIndirectObjectList* m_Objects;
    std::shared_ptr<charbuff> m_buffer;
};

};

#endif // PDF_OBJECT_STREAM_PARSER_OBJECT_H
