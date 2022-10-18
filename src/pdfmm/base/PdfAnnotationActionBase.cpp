/*
 * SPDX-FileCopyrightText: (C) 2022 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.1
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfAnnotationActionBase.h"

#include "PdfDictionary.h"

using namespace std;
using namespace mm;

PdfAnnotationActionBase::PdfAnnotationActionBase(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect)
    : PdfAnnotation(page, annotType, rect)
{
}

PdfAnnotationActionBase::PdfAnnotationActionBase(PdfObject& obj, PdfAnnotationType annotType)
    : PdfAnnotation(obj, annotType)
{
}

void PdfAnnotationActionBase::SetAction(const shared_ptr<PdfAction>& action)
{
    GetDictionary().AddKey("A", action->GetObject().GetIndirectReference());
    m_Action = action;
}

shared_ptr<PdfAction> PdfAnnotationActionBase::GetAction() const
{
    return const_cast<PdfAnnotationActionBase&>(*this).getAction();
}

shared_ptr<PdfAction> PdfAnnotationActionBase::getAction()
{
    if (m_Action == nullptr)
    {
        auto obj = GetDictionary().FindKey("A");
        if (obj == nullptr)
            return nullptr;

        m_Action.reset(new PdfAction(*obj));
    }

    return m_Action;
}
