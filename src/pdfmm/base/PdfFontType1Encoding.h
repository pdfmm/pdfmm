/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FONT_TYPE1_ENCODING_H
#define PDF_FONT_TYPE1_ENCODING_H

#include "PdfEncodingMap.h"

namespace mm {

/** Read a built-in encoding from a /Type1 font program
 */
class PDFMM_API PdfFontType1Encoding final : public PdfEncodingMapBase
{
    friend class PdfEncodingFactory;

private:
    PdfFontType1Encoding(const PdfObject& obj);

protected:
    void getExportObject(PdfIndirectObjectList& objects, PdfName& name, PdfObject*& obj) const override;

private:
    PdfCharCodeMap getUnicodeMap(const PdfObject& obj);
};

};

#endif // PDF_FONT_TYPE1_ENCODING_H 
