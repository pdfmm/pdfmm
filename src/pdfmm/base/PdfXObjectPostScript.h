/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.1
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XOBJECT_POST_SCRIPT_H
#define PDF_XOBJECT_POST_SCRIPT_H

#include "PdfXObject.h"

namespace mm {

class PDFMM_API PdfXObjectPostScript : public PdfXObject
{
    friend class PdfXObject;

public:
    PdfRect GetRect() const override;

private:
    PdfXObjectPostScript(PdfObject& obj);
};

}

#endif // PDF_XOBJECT_POST_SCRIPT_H
