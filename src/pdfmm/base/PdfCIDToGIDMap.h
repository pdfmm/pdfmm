/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CID_TO_GID_MAP_H
#define PDF_CID_TO_GID_MAP_H

#include "PdfDeclarations.h"
#include "PdfObject.h"

namespace mm
{
    /** Helper class to handle the /CIDToGIDMap entry in a Type2 CID font
     * or /TrueType fonts implicit CID to GID mapping
     */
    class PdfCIDToGIDMap final
    {
    public:
        using iterator = CIDToGIDMap::const_iterator;

    public:
        PdfCIDToGIDMap(CIDToGIDMap&& map, PdfGlyphAccess access);
        PdfCIDToGIDMap(const PdfCIDToGIDMap&) = default;
        PdfCIDToGIDMap(PdfCIDToGIDMap&&) noexcept = default;

        static PdfCIDToGIDMap Create(const PdfObject& cidToGidMapObj, PdfGlyphAccess access);

    public:
        bool TryMapCIDToGID(unsigned cid, unsigned& gid) const;
        void ExportTo(PdfObject& descendantFont);

        /** Determines if the current map provides the queried glyph access
         */
        bool HasGlyphAccess(PdfGlyphAccess access) const;

    public:
        unsigned GetSize() const;
        iterator begin() const;
        iterator end() const;

    private:
        CIDToGIDMap m_cidToGidMap;
        PdfGlyphAccess m_access;
    };

    using PdfCIDToGIDMapConstPtr = std::shared_ptr<const PdfCIDToGIDMap>;
}

#endif // PDF_CID_TO_GID_MAP_H
