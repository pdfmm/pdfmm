/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfAction.h"

#include "base/PdfDictionary.h"

using namespace std;
using namespace PoDoFo;

static const char* s_names[] = {
    nullptr,
    "GoTo",
    "GoToR",
    "GoToE",
    "Launch",
    "Thread",
    "URI",
    "Sound",
    "Movie",
    "Hide",
    "Named",
    "SubmitForm",
    "ResetForm",
    "ImportData",
    "JavaScript",
    "SetOCGState",
    "Rendition",
    "Trans",
    "GoTo3DView",
};

PdfAction::PdfAction(PdfDocument& doc, PdfActionType action)
    : PdfElement(doc, "Action"), m_Type(action)
{
    const PdfName type = PdfName(TypeNameForIndex((unsigned)action, s_names, std::size(s_names)));

    if (type.GetLength() == 0)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    this->GetObject().GetDictionary().AddKey("S", type);
}

PdfAction::PdfAction(PdfObject& obj)
// The typename /Action is optional for PdfActions
    : PdfElement(obj)
{
    m_Type = static_cast<PdfActionType>(TypeNameToIndex(
        this->GetObject().GetDictionary().FindKeyAs<PdfName>("S").GetString().c_str(),
        s_names, std::size(s_names), (int)PdfActionType::Unknown));
}

void PdfAction::SetURI(const PdfString& uri)
{
    this->GetObject().GetDictionary().AddKey("URI", uri);
}

PdfString PdfAction::GetURI() const
{
    return this->GetObject().GetDictionary().MustFindKey("URI").GetString();
}

bool PdfAction::HasURI() const
{
    return (this->GetObject().GetDictionary().FindKey("URI") != nullptr);
}

void PdfAction::SetScript(const PdfString& sScript)
{
    this->GetObject().GetDictionary().AddKey("JS", sScript);
}

PdfString PdfAction::GetScript() const
{
    return this->GetObject().GetDictionary().MustFindKey("JS").GetString();
}

bool PdfAction::HasScript() const
{
    return this->GetObject().GetDictionary().HasKey("JS");
}

void PdfAction::AddToDictionary(PdfDictionary& dictionary) const
{
    // since we can only have EITHER a Dest OR an Action
    // we check for an Action, and if already present, we throw
    if (dictionary.HasKey("Dest"))
        PODOFO_RAISE_ERROR(EPdfError::ActionAlreadyPresent);

    dictionary.AddKey("A", this->GetObject());
}
