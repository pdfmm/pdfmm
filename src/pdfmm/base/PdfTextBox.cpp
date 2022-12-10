/**
 * SPDX-FileCopyrightText: (C) 2007 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfTextBox.h"
#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfTextBox::PdfTextBox(PdfAcroForm& acroform, const shared_ptr<PdfField>& parent)
    : PdfField(acroform, PdfFieldType::TextBox, parent)
{
    init();
}

PdfTextBox::PdfTextBox(PdfAnnotationWidget& widget, const shared_ptr<PdfField>& parent)
    : PdfField(widget, PdfFieldType::TextBox, parent)
{
    init();
}

PdfTextBox::PdfTextBox(PdfObject& obj, PdfAcroForm* acroform)
    : PdfField(obj, acroform, PdfFieldType::TextBox)
{
    // NOTE: Do not call init() here
}

void PdfTextBox::init()
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
    if (maxLength != -1 && text.GetString().length() > (unsigned)maxLength)
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::ValueOutOfRange, "Unable to set text larger MaxLen");

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
    this->SetFieldFlag(static_cast<int>(PdfTextBox_MultiLine), multiLine);
}

bool PdfTextBox::IsMultiLine() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_MultiLine), false);
}

void PdfTextBox::SetPasswordField(bool password)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_Password), password);
}

bool PdfTextBox::IsPasswordField() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_Password), false);
}

void PdfTextBox::SetFileField(bool file)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_FileSelect), file);
}

bool PdfTextBox::IsFileField() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_FileSelect), false);
}

void PdfTextBox::SetSpellcheckingEnabled(bool spellcheck)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_NoSpellcheck), !spellcheck);
}

bool PdfTextBox::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_NoSpellcheck), true);
}

void PdfTextBox::SetScrollBarsEnabled(bool scroll)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_NoScroll), !scroll);
}

bool PdfTextBox::IsScrollBarsEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_NoScroll), true);
}

void PdfTextBox::SetCombs(bool combs)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_Comb), combs);
}

bool PdfTextBox::IsCombs() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_Comb), false);
}

void PdfTextBox::SetRichText(bool richText)
{
    this->SetFieldFlag(static_cast<int>(PdfTextBox_RichText), richText);
}

bool PdfTextBox::IsRichText() const
{
    return this->GetFieldFlag(static_cast<int>(PdfTextBox_RichText), false);
}

PdfTextBox* PdfTextBox::GetParent()
{
    return GetParentTyped<PdfTextBox>(PdfFieldType::TextBox);
}

const PdfTextBox* PdfTextBox::GetParent() const
{
    return GetParentTyped<PdfTextBox>(PdfFieldType::TextBox);
}
