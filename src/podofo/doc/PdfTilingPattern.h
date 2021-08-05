/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_TILING_PATTERN_H
#define PDF_TILING_PATTERN_H

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfName.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfImage;
class PdfObject;
class PdfPage;
class PdfWriter;

/**
 * This class defined a tiling pattern which can be used
 * to fill abitrary shapes with a pattern using PdfPainter.
 */
class PODOFO_DOC_API PdfTilingPattern final : public PdfElement
{
public:

    /** Returns the identifier of this TilingPattern how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /PtrnXXXXX)
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    /** Create a new PdfTilingPattern object, which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param tilingType the type of this tiling pattern
     *  \param strokeR strok color red component
     *  \param strokeG strok color green component
     *  \param strokeB strok color blue component
     *  \param doFill whether tile fills content first, with fill color
     *  \param fillR fill color red component
     *  \param fillG fill color green component
     *  \param fillB fill color blue component
     *  \param offsetX tile offset on X axis
     *  \param offsetY tile offset on Y axis
     *  \param image image to use - can be set only if eTilingType is EPdfTilingPatternType::Image
     *  \param parent parent document
     *
     *  \note stroke and fill colors are ignored if eTilingType is EPdfTilingPatternType::Image
     *
     *  \note fill color is ignored if doFill is false
     *
     *  \note image is ignored for all but EPdfTilingPatternType::Image tilingType types, where it cannot be nullptr
     *
     */
    PdfTilingPattern(PdfDocument& doc, PdfTilingPatternType tilingType,
        double strokeR, double strokeG, double strokeB,
        bool doFill, double fillR, double fillG, double fillB,
        double offsetX, double offsetY,
        PdfImage* image);

private:
    void Init(PdfTilingPatternType tilingType,
        double strokeR, double strokeG, double strokeB,
        bool doFill, double fillR, double fillG, double fillB,
        double offsetX, double offsetY,
        PdfImage* image);

    void AddToResources(const PdfName& identifier, const PdfReference& ref, const PdfName& name);
private:
    PdfName m_Identifier;
};

} // end namespace

#endif // PDF_TILING_PATTERN_H
