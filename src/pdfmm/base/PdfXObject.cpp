/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfXObject.h"

#include <sstream>

#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfRect.h"
#include "PdfVariant.h"
#include "PdfImage.h"
#include "PdfPage.h"
#include "PdfDocument.h"
#include "PdfXObjectForm.h"
#include "PdfXObjectPostScript.h"

using namespace std;
using namespace mm;

PdfXObject::PdfXObject(PdfDocument& doc, PdfXObjectType subType, const string_view& prefix)
    : PdfDictionaryElement(doc, "XObject"), m_Type(subType)
{
    initIdentifiers(prefix);
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName(ToString(subType)));
}

PdfXObject::PdfXObject(PdfObject& obj, PdfXObjectType subType)
    : PdfDictionaryElement(obj), m_Type(subType)
{
    initIdentifiers({ });
}

bool PdfXObject::TryCreateFromObject(PdfObject& obj, unique_ptr<PdfXObject>& xobj)
{
    auto typeObj = obj.GetDictionary().GetKey(PdfName::KeyType);
    if (typeObj == nullptr
        || !typeObj->IsName()
        || typeObj->GetName().GetString() != "XObject")
    {
        xobj.reset();
        return false;
    }

    auto type = getPdfXObjectType(obj);
    switch (type)
    {
        case PdfXObjectType::Form:
        {
            xobj.reset(new PdfXObjectForm(obj));
            return true;
        }
        case PdfXObjectType::PostScript:
        {
            xobj.reset(new PdfXObjectPostScript(obj));
            return true;
        }
        case PdfXObjectType::Image:
        {
            xobj.reset(new PdfImage(obj));
            return true;
        }
        default:
        {
            xobj = nullptr;
            return false;
        }
    }
}

PdfXObjectType PdfXObject::getPdfXObjectType(const PdfObject& obj)
{
    auto subTypeObj = obj.GetDictionary().FindKey(PdfName::KeySubtype);
    if (subTypeObj == nullptr || !subTypeObj->IsName())
        return PdfXObjectType::Unknown;

    auto subtype = obj.GetDictionary().FindKey(PdfName::KeySubtype)->GetName().GetString();
    return FromString(subtype);
}

string PdfXObject::ToString(PdfXObjectType type)
{
    switch (type)
    {
        case PdfXObjectType::Form:
            return "Form";
        case PdfXObjectType::Image:
            return "Image";
        case PdfXObjectType::PostScript:
            return "PS";
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidDataType);
    }
}

PdfXObjectType PdfXObject::FromString(const string& str)
{
    if (str == "Form")
        return PdfXObjectType::Form;
    else if (str == "Image")
        return PdfXObjectType::Image;
    else if (str == "PS")
        return PdfXObjectType::PostScript;
    else
        return PdfXObjectType::Unknown;
}

bool PdfXObject::tryGetXObjectType(const type_info& type, PdfXObjectType& xobjType)
{
    if (type == typeid(PdfXObjectForm))
    {
        xobjType = PdfXObjectType::Form;
        return true;
    }
    else if (type == typeid(PdfImage))
    {
        xobjType = PdfXObjectType::Image;
        return true;
    }
    else if (type == typeid(PdfXObjectPostScript))
    {
        xobjType = PdfXObjectType::PostScript;
        return true;
    }
    else
    {
        xobjType = PdfXObjectType::Unknown;
        return false;
    }
}

void PdfXObject::initIdentifiers(const string_view& prefix)
{
    ostringstream out;
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /XOb for XObject.
    if (prefix.length() == 0)
        out << "XOb" << this->GetObject().GetIndirectReference().ObjectNumber();
    else
        out << prefix << this->GetObject().GetIndirectReference().ObjectNumber();

    m_Identifier = PdfName(out.str().c_str());
}
