/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XOBJECT_FORM_H
#define PDF_XOBJECT_FORM_H

#include "PdfXObject.h"
#include "PdfCanvas.h"
#include "PdfResources.h"

namespace mm {

class PDFMM_API PdfXObjectForm final : public PdfXObject, public PdfCanvas
{
    friend class PdfXObject;

public:
    /** Create a new XObject with a specified dimension
     *  in a given document
     *
     *  \param doc the parent document of the XObject
     *  \param rect the size of the XObject
     *  \param prefix optional prefix for XObject-name
     */
    PdfXObjectForm(PdfDocument& doc, const PdfRect& rect,
        const std::string_view& prefix = { });

    /** Create a new XObject from a page of another document
     *  in a given document
     *
     *  \param doc the parent document of the XObject
     *  \param sourceDoc the document to create the XObject from
     *  \param pageIndex the page-number in doc to create the XObject from
     *  \param prefix optional prefix for XObject-name
     *	\param useTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObjectForm(PdfDocument& doc, const PdfDocument& sourceDoc,
        unsigned pageIndex, const std::string_view& prefix = { }, bool useTrimBox = false);

    /** Create a new XObject from an existing page
     *
     *  \param doc the document to create the XObject at
     *  \param pageIndex the page-number in doc to create the XObject from
     *  \param prefix optional prefix for XObject-name
     *  \param useTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObjectForm(PdfDocument& doc, unsigned pageIndex,
        const std::string_view& prefix = { }, bool useTrimBox = false);

public:
    /** Ensure resources initialized on this XObject
    */
    void EnsureResourcesCreated();

    PdfResources& GetOrCreateResources() override;

    bool HasRotation(double& teta) const override;

    PdfRect GetRect() const override;

    /** Set the rectangle of this xobject
     *  \param rect a rectangle
     */
    void SetRect(const PdfRect& rect);

public:
    inline PdfResources* GetResources() { return m_Resources.get(); }
    inline const PdfResources* GetResources() const { return m_Resources.get(); }

private:
    PdfXObjectForm(PdfObject& obj);

private:
    PdfObject* getContentsObject() const override;
    PdfResources* getResources() const override;
    PdfElement& getElement() const override;
    PdfObjectStream& GetStreamForAppending(PdfStreamAppendFlags flags) override;
    void initXObject(const PdfRect& rect);
    void initAfterPageInsertion(const PdfDocument& doc, unsigned pageIndex);

private:
    PdfElement& GetElement() = delete;
    const PdfElement& GetElement() const = delete;
    PdfObject* GetContentsObject() = delete;
    const PdfObject* GetContentsObject() const = delete;

private:
    PdfRect m_Rect;
    PdfArray m_Matrix;
    std::unique_ptr<PdfResources> m_Resources;
};

}

#endif // PDF_XOBJECT_FORM_H
