/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfInfo.h"

#include "base/PdfDate.h"
#include "base/PdfDictionary.h"
#include "base/PdfString.h"

#define PRODUCER_STRING "PoDoFo - http:/" "/podofo.sf.net"

using namespace std;
using namespace PoDoFo;

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
    return obj != nullptr && obj->IsString() ? optional(obj->GetString()) : std::nullopt;
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

void PdfInfo::SetAuthor(const PdfString& sAuthor)
{
    this->GetObject().GetDictionary().AddKey("Author", sAuthor);
}

void PdfInfo::SetCreator(const PdfString& sCreator)
{
    this->GetObject().GetDictionary().AddKey("Creator", sCreator);
}

void PdfInfo::SetKeywords(const PdfString& sKeywords)
{
    this->GetObject().GetDictionary().AddKey("Keywords", sKeywords);
}

void PdfInfo::SetSubject(const PdfString& sSubject)
{
    this->GetObject().GetDictionary().AddKey("Subject", sSubject);
}

void PdfInfo::SetTitle(const PdfString& title)
{
    this->GetObject().GetDictionary().AddKey("Title", title);
}

// Peter Petrov 27 April 2008
// We have added a SetProducer() method in PdfInfo
void PdfInfo::SetProducer(const PdfString& sProducer)
{
    this->GetObject().GetDictionary().AddKey("Producer", sProducer);
}

void PdfInfo::SetTrapped(const PdfName& sTrapped)
{
    if ((sTrapped.GetString() == "True") || (sTrapped.GetString() == "False"))
        this->GetObject().GetDictionary().AddKey("Trapped", sTrapped);
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
