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
     */
    class PdfCIDToGIDMap
    {
    public:
        PdfCIDToGIDMap(const PdfCIDToGIDMap&) = default;
        PdfCIDToGIDMap(PdfCIDToGIDMap&&) noexcept = default;

        static PdfCIDToGIDMap Create(const PdfObject& cidToGidMapObj);

    public:
        bool TryMapCIDToGID(unsigned cid, unsigned& gid) const;
        void ExportTo(PdfObject& descendantFont);

    public:
        const CIDToGIDMap& GetCIDToGIDMap() const { return m_cidToGidMap; }

    private:
        PdfCIDToGIDMap(CIDToGIDMap&& map);

    private:
        CIDToGIDMap m_cidToGidMap;
    };
}

#endif // PDF_CID_TO_GID_MAP_H
