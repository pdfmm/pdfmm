/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfInfo.h"

#include "PdfDate.h"
#include "PdfDictionary.h"
#include "PdfString.h"

#define PRODUCER_STRING "pdfmm - https://github.com/pdfmm/pdfmm"

using namespace std;
using namespace mm;

PdfInfo::PdfInfo(PdfDocument& doc, PdfInfoInitial initial)
    : PdfElement(doc)
{
    Init(initial);
}

PdfInfo::PdfInfo(PdfObject& obj, PdfInfoInitial initial)
    : PdfElement(obj)
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

optional<PdfString> PdfInfo::GetStringFromInfoDict(const PdfName& name) const
{
    auto obj = this->GetObject().GetDictionary().FindKey(name);
    return obj != nullptr && obj->IsString() ? std::make_optional(obj->GetString()) : std::nullopt;
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

void PdfInfo::SetAuthor(const PdfString& author)
{
    this->GetObject().GetDictionary().AddKey("Author", author);
}

void PdfInfo::SetCreator(const PdfString& creator)
{
    this->GetObject().GetDictionary().AddKey("Creator", creator);
}

void PdfInfo::SetKeywords(const PdfString& keywords)
{
    this->GetObject().GetDictionary().AddKey("Keywords", keywords);
}

void PdfInfo::SetSubject(const PdfString& subject)
{
    this->GetObject().GetDictionary().AddKey("Subject", subject);
}

void PdfInfo::SetTitle(const PdfString& title)
{
    this->GetObject().GetDictionary().AddKey("Title", title);
}

// Peter Petrov 27 April 2008
// We have added a SetProducer() method in PdfInfo
void PdfInfo::SetProducer(const PdfString& producer)
{
    this->GetObject().GetDictionary().AddKey("Producer", producer);
}

void PdfInfo::SetTrapped(const PdfName& trapped)
{
    if ((trapped.GetString() == "True") || (trapped.GetString() == "False"))
        this->GetObject().GetDictionary().AddKey("Trapped", trapped);
    else
        this->GetObject().GetDictionary().AddKey("Trapped", PdfName("Unknown"));
}

optional<PdfString> PdfInfo::GetAuthor() const
{
    return this->GetStringFromInfoDict("Author");
}

optional<PdfString> PdfInfo::GetCreator() const
{
    return this->GetStringFromInfoDict("Creator");
}

optional<PdfString> PdfInfo::GetKeywords() const
{
    return this->GetStringFromInfoDict("Keywords");
}

optional<PdfString> PdfInfo::GetSubject() const
{
    return this->GetStringFromInfoDict("Subject");
}

optional<PdfString> PdfInfo::GetTitle() const
{
    return this->GetStringFromInfoDict("Title");
}

optional<PdfString> PdfInfo::GetProducer() const
{
    return this->GetStringFromInfoDict("Producer");
}

const PdfName& PdfInfo::GetTrapped() const
{
    return this->GetNameFromInfoDict("Trapped");
}

PdfDate PdfInfo::GetCreationDate() const
{
    auto datestr = this->GetStringFromInfoDict("CreationDate");
    if (datestr == nullptr)
        return PdfDate();
    else
        return PdfDate(*datestr);
}

PdfDate PdfInfo::GetModDate() const
{
    auto datestr = this->GetStringFromInfoDict("ModDate");
    if (datestr == nullptr)
        return PdfDate();
    else
        return PdfDate(*datestr);
}
