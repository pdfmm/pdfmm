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

namespace PoDoFo
{
// CHECK-ME: Consider make this class not inherit PdfParserObject and consider mark that final
/**
 * A utility class for PdfParser that can parse
 * an XRef stream object.
 *
 * It is mainly here to make PdfParser more modular.
 * This is only marked PODOFO_API for the benefit of the tests,
 * the class is for internal use only.
 */
class PODOFO_API PdfXRefStreamParserObject : public PdfParserObject
{
    static constexpr unsigned W_ARRAY_SIZE = 3;
    static constexpr unsigned W_MAX_BYTES = 4;

public:

    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param document document where to resolve object references
     *  \param rDevice an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param rBuffer buffer to use for parsing to avoid reallocations
     *  \param pOffsets XRef entries are stored into this array
     */
    PdfXRefStreamParserObject(PdfDocument& document, const PdfRefCountedInputDevice & rDevice,
                              const PdfRefCountedBuffer & rBuffer, TVecEntries& entries);

    void Parse();

    void ReadXRefTable();

    /**
     * \returns the offset of the previous XRef table
     */
    bool TryGetPreviousOffset(size_t &previousOffset) const;

private:
    /**
     * Read the /Index key from the current dictionary
     * and write it to a vector.
     *
     * \param rvecIndeces store the indeces hare
     * \param size default value from /Size key
     */
    void GetIndeces( std::vector<int64_t> & rvecIndeces, int64_t size );

    /**
     * Parse the stream contents
     *
     * \param nW /W key
     * \param rvecIndeces indeces as filled by GetIndeces
     *
     * \see GetIndeces
     */
    void ParseStream( const int64_t nW[W_ARRAY_SIZE], const std::vector<int64_t> & rvecIndeces );

    void ReadXRefStreamEntry( char* pBuffer, size_t, const int64_t lW[W_ARRAY_SIZE], int nObjNo );

private:
    ssize_t m_lNextOffset;
    TVecEntries* m_entries;
};

};

#endif // PDF_XREF_STREAM_PARSER_OBJECT_H
