/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_EXTENSION_H
#define PDF_EXTENSION_H

#include "podofo/base/PdfDefines.h"

namespace PoDoFo {
    
    /** PdfExtension is a simple class that describes a vendor-specific extension to
     *  the official specifications.
     */
    class PODOFO_DOC_API PdfExtension {
        
    public:
        
        PdfExtension(const char* ns, PdfVersion baseVersion, int64_t level):
        _ns(ns), _baseVersion(baseVersion), _level(level) {}
        
        const std::string& getNamespace() const { return _ns; }
        PdfVersion getBaseVersion() const { return _baseVersion; }
        int64_t getLevel() const { return _level; }
        
    private:
        
        std::string _ns;
        PdfVersion _baseVersion;
        int64_t _level;
    };
}

#endif	// PDF_EXTENSION_H
