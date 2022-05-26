/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.1
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfXObjectPostScript.h"

using namespace std;
using namespace mm;

PdfXObjectPostScript::PdfXObjectPostScript(PdfObject& obj)
    : PdfXObject(obj, PdfXObjectType::PostScript)
{

}

PdfRect PdfXObjectPostScript::GetRect() const
{
    return PdfRect();
}
