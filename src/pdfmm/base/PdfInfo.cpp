/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfInfo.h"

#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfString.h"

#define PRODUCER_STRING "pdfmm - https://github.com/pdfmm/pdfmm"

using namespace std;
using namespace mm;

PdfInfo::PdfInfo(PdfDocument& doc, PdfInfoInitial initial)
    : PdfDictionaryElement(doc)
{
    Init(initial);
}

PdfInfo::PdfInfo(PdfObject& obj, PdfInfoInitial initial)
    : PdfDictionaryElement(obj)
{
    Init(initial);
}

void PdfInfo::Init(PdfInfoInitial initial)
{
    PdfDate date;
    PdfString str = date.ToString();

    if ((initial & PdfInfoInitial::WriteCreationTime) == PdfInfoInitial::WriteCreationTime)
        this->GetObject().GetDictionary().AddKey("CreationDate", str);

    if ((initial & PdfInfoInitial::WriteModificationTime) == PdfInfoInitial::WriteModificationTime)
        this->GetObject().GetDictionary().AddKey("ModDate", str);

    if ((initial & PdfInfoInitial::WriteProducer) == PdfInfoInitial::WriteProducer)
        this->GetObject().GetDictionary().AddKey("Producer", PdfString(PRODUCER_STRING));
}

nullable<PdfString> PdfInfo::GetStringFromInfoDict(const PdfName& name) const
{
    auto obj = this->GetObject().GetDictionary().FindKey(name);
    return obj != nullptr && obj->IsString() ? nullable<PdfString>(obj->GetString()) : nullptr;
}

const PdfName& PdfInfo::GetNameFromInfoDict(const PdfName& name) const
{
    auto obj = this->GetObject().GetDictionary().FindKey(name);
    return  obj != nullptr && obj->IsName() ? obj->GetName() : PdfName::KeyNull;
}

void PdfInfo::SetCustomKey(const PdfName& name, const PdfString& value)
{
    this->GetObject().GetDictionary().AddKey(name, value);
}

void PdfInfo::SetAuthor(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Author", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Author");
}

void PdfInfo::SetCreator(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Creator", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Creator");
}

void PdfInfo::SetKeywords(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Keywords", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Keywords");
}

void PdfInfo::SetSubject(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Subject", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Subject");
}

void PdfInfo::SetTitle(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Title", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Title");
}

void PdfInfo::SetProducer(nullable<const PdfString&> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("Producer", *value);
    else
        this->GetObject().GetDictionary().RemoveKey("Producer");
}

void PdfInfo::SetTrapped(const PdfName& trapped)
{
    if ((trapped.GetString() == "True") || (trapped.GetString() == "False"))
        this->GetObject().GetDictionary().AddKey("Trapped", trapped);
    else
        this->GetObject().GetDictionary().AddKey("Trapped", PdfName("Unknown"));
}

nullable<PdfString> PdfInfo::GetAuthor() const
{
    return this->GetStringFromInfoDict("Author");
}

nullable<PdfString> PdfInfo::GetCreator() const
{
    return this->GetStringFromInfoDict("Creator");
}

nullable<PdfString> PdfInfo::GetKeywords() const
{
    return this->GetStringFromInfoDict("Keywords");
}

nullable<PdfString> PdfInfo::GetSubject() const
{
    return this->GetStringFromInfoDict("Subject");
}

nullable<PdfString> PdfInfo::GetTitle() const
{
    return this->GetStringFromInfoDict("Title");
}

nullable<PdfString> PdfInfo::GetProducer() const
{
    return this->GetStringFromInfoDict("Producer");
}

nullable<PdfDate> PdfInfo::GetCreationDate() const
{
    auto datestr = this->GetStringFromInfoDict("CreationDate");
    if (datestr == nullptr)
        return PdfDate();
    else
        return PdfDate(*datestr);
}

nullable<PdfDate> PdfInfo::GetModDate() const
{
    auto datestr = this->GetStringFromInfoDict("ModDate");
    if (datestr == nullptr)
        return PdfDate();
    else
        return PdfDate(*datestr);
}

const PdfName& PdfInfo::GetTrapped() const
{
    return this->GetNameFromInfoDict("Trapped");
}

void PdfInfo::SetCreationDate(nullable<PdfDate> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("CreationDate", value->ToString());
    else
        this->GetObject().GetDictionary().RemoveKey("CreationDate");
}

void PdfInfo::SetModDate(nullable<PdfDate> value)
{
    if (value.has_value())
        this->GetObject().GetDictionary().AddKey("ModDate", value->ToString());
    else
        this->GetObject().GetDictionary().RemoveKey("ModDate");
}
