/**
 * SPDX-FileCopyrightText: (C) 2021 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 * SPDX-License-Identifier: MPL-2.0
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
