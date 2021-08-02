/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_TRUE_TYPE_H
#define PDF_FONT_TRUE_TYPE_H

#include "podofo/base/PdfDefines.h"
#include "PdfFontSimple.h"

namespace PoDoFo {

/** A PdfFont implementation that can be used
 *  to embedd truetype fonts into a PDF file
 *  or to draw with truetype fonts. 
 *
 *  TrueType fonts are always embedded as suggested in the PDF reference.
 */
class PdfFontTrueType final : public PdfFontSimple
{
    friend class PdfFontFactory;

private:

    /** Create a new TrueType font. 
     *
     *  It will get embedded automatically.
     *
     *  \param doc parent of the font object
     *  \param metrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is 
     *         deleted along with the font.
     *  \param encoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param embed if true the font will get embedded.
     *  
     */
     PdfFontTrueType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
         const PdfEncoding& encoding);

public:
    PdfFontType GetType() const override;

protected:
     void embedFontFile(PdfObject& descriptor) override;
     void initImported() override;
};

};

#endif // PDF_FONT_TRUE_TYPE_H

