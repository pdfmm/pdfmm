/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CONTENTS_H
#define PDF_CONTENTS_H

#include "PdfCanvas.h"

namespace mm {

class PdfPage;

/** A interface that provides a wrapper around "PDF content" -
	the instructions that are used to draw on the PDF "canvas".
 */
class PDFMM_API PdfContents
{
public:
    PdfContents(PdfPage &parent, PdfObject &obj);

    PdfContents(PdfPage &parent);

    /** Get access to the raw contents object.
     *  It will either be a PdfStream or a PdfArray
     *  \returns a contents object
     */
    PdfObject & GetContents() const;

    /** Get access to an object into which you can add contents
     *   at the end of the "stream".
     */
    PdfStream & GetStreamForAppending(PdfStreamAppendFlags flags);

private:
    PdfPage *m_parent;
    PdfObject *m_object;
};

};

#endif // PDF_CONTENTS_H
