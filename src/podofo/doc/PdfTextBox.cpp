/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfTextBox.h"

using namespace PoDoFo;

PdfTextBox::PdfTextBox(PdfObject& obj, PdfAnnotation* widget)
    : PdfField(PdfFieldType::TextField, obj, widget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfTextBox::PdfTextBox(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform)
    : PdfField(PdfFieldType::TextField, doc, widget, insertInAcroform)
{
    Init();
}

PdfTextBox::PdfTextBox(PdfPage& page, const PdfRect& rect)
    : PdfField(PdfFieldType::TextField, page, rect)
{
    Init();
}

void PdfTextBox::Init()
{
    if (!GetObject().GetDictionary().HasKey("DS"))
        GetObject().GetDictionary().AddKey("DS", PdfString("font: 12pt Helvetica"));
}

void PdfTextBox::SetText(const PdfString& text)
{
    AssertTerminalField();
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");

    // if text is longer than maxlen, truncate it
    int64_t maxLength = this->GetMaxLen();
    if (maxLength != -1 && text.GetLength() > (size_t)maxLength)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Unable to set text larger MaxLen");

    GetObject().GetDictionary().AddKey(key, text);
}

PdfString PdfTextBox::GetText() const
{
    AssertTerminalField();
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");
    PdfString str;

    auto found = GetObject().GetDictionary().FindKeyParent(key);
    if (found == nullptr)
        return str;

    return found->GetString();
}

void PdfTextBox::SetMaxLen(int64_t maxLen)
{
    GetObject().GetDictionary().AddKey("MaxLen", maxLen);
}

int64_t PdfTextBox::GetMaxLen() const
{
    auto found = GetObject().GetDictionary().FindKeyParent("MaxLen");
    if (found == nullptr)
        return -1;

    return found->GetNumber();
}

void PdfTextBox::SetMultiLine(bool multiLine)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_MultiLine), multiLine);
}

bool PdfTextBox::IsMultiLine() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_MultiLine), false);
}

void PdfTextBox::SetPasswordField(bool password)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_Password), password);
}

bool PdfTextBox::IsPasswordField() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_Password), false);
}

void PdfTextBox::SetFileField(bool file)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_FileSelect), file);
}

bool PdfTextBox::IsFileField() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_FileSelect), false);
}

void PdfTextBox::SetSpellcheckingEnabled(bool spellcheck)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_NoSpellcheck), !spellcheck);
}

bool PdfTextBox::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_NoSpellcheck), true);
}

void PdfTextBox::SetScrollBarsEnabled(bool scroll)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_NoScroll), !scroll);
}

bool PdfTextBox::IsScrollBarsEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_NoScroll), true);
}

void PdfTextBox::SetCombs(bool combs)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_Comb), combs);
}

bool PdfTextBox::IsCombs() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_Comb), false);
}

void PdfTextBox::SetRichText(bool richText)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_RichText), richText);
}

bool PdfTextBox::IsRichText() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_RichText), false);
}
