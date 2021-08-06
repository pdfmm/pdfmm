/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_EXTENSION_H
#define PDF_EXTENSION_H

#include "PdfDefines.h"

namespace PoDoFo {
    
    /** PdfExtension is a simple class that describes a vendor-specific extension to
     *  the official specifications.
     */
    class PODOFO_DOC_API PdfExtension
    {
    public:
        PdfExtension(const std::string_view& ns, PdfVersion baseVersion, int64_t level);
        
        inline const std::string& GetNamespace() const { return m_Ns; }
        PdfVersion GetBaseVersion() const { return m_BaseVersion; }
        int64_t GetLevel() const { return m_Level; }
        
    private:
        std::string m_Ns;
        PdfVersion m_BaseVersion;
        int64_t m_Level;
    };
}

#endif	// PDF_EXTENSION_H
