/**
 * Copyright (C) 2009 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XREF_STREAM_PARSER_OBJECT_H
#define PDF_XREF_STREAM_PARSER_OBJECT_H

#include "PdfDefines.h"
#include "PdfXRefEntry.h"
#include "PdfParserObject.h"

namespace mm
{
// CHECK-ME: Consider make this class not inherit PdfParserObject and consider mark that final
/**
 * A utility class for PdfParser that can parse
 * an XRef stream object.
 *
 * It is mainly here to make PdfParser more modular.
 * This is only marked PDFMM_API for the benefit of the tests,
 * the class is for internal use only.
 */
class PDFMM_API PdfXRefStreamParserObject : public PdfParserObject
{
    static constexpr unsigned W_ARRAY_SIZE = 3;
    static constexpr unsigned W_MAX_BYTES = 4;

public:

    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param document document where to resolve object references
     *  \param device an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param buffer buffer to use for parsing to avoid reallocations
     */
    PdfXRefStreamParserObject(PdfDocument& document, const PdfRefCountedInputDevice& device,
        const PdfSharedBuffer& buffer, PdfXRefEntries& entries);

    void Parse();

    void ReadXRefTable();

    /**
     * \returns the offset of the previous XRef table
     */
    bool TryGetPreviousOffset(size_t& previousOffset) const;

private:
    /**
     * Read the /Index key from the current dictionary
     * and write it to a vector.
     *
     * \param indices store the indeces hare
     * \param size default value from /Size key
     */
    void GetIndices(std::vector<int64_t>& indices, int64_t size);

    /**
     * Parse the stream contents
     *
     * \param wArray /W key
     * \param indices indices as filled by GetIndices
     *
     * \see GetIndices
     */
    void ParseStream(const int64_t wArray[W_ARRAY_SIZE], const std::vector<int64_t>& indices);

    void ReadXRefStreamEntry(char* buffer, size_t, const int64_t wArray[W_ARRAY_SIZE], int objNo);

private:
    ssize_t m_NextOffset;
    PdfXRefEntries* m_entries;
};

};

#endif // PDF_XREF_STREAM_PARSER_OBJECT_H
