/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_OBJECT_STREAM_PARSER_OBJECT_H
#define PDF_OBJECT_STREAM_PARSER_OBJECT_H

#include "PdfDefines.h"

#include "PdfRefCountedBuffer.h"
#include "PdfParserObject.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfVecObjects;

/**
 * A utility class for PdfParser that can parse
 * an object stream object (PDF Reference 1.7 3.4.6 Object Streams)
 *
 * It is mainly here to make PdfParser more modular.
 */
class PdfObjectStreamParser
{
public:
    typedef std::vector<int64_t> ObjectIdList;
    /**
     * Create a new PdfObjectStreamParserObject from an existing
     * PdfParserObject. The PdfParserObject will be removed and deleted.
     * All objects from the object stream will be read into memory.
     *
     * \param pParser PdfParserObject for an object stream
     * \param pVecObjects add loaded objects to this vector of objects
     * \param rBuffer use this allocated buffer for caching
     * \param pEncrypt encryption object used to decrypt streams
     */
    PdfObjectStreamParser(PdfParserObject& parser, PdfVecObjects& objects, const PdfRefCountedBuffer& buffer);

    void Parse(ObjectIdList const&);

private:
    void ReadObjectsFromStream(char* pBuffer, size_t lBufferLen, int64_t lNum, int64_t lFirst, ObjectIdList const&);

private:
    PdfParserObject* m_pParser;
    PdfVecObjects* m_vecObjects;
    PdfRefCountedBuffer m_buffer;
};

};

#endif // PDF_OBJECT_STREAM_PARSER_OBJECT_H
