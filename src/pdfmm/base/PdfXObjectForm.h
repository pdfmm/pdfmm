/**
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

class PDFMM_API PdfXObjectForm : public PdfXObject, public PdfCanvas
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
    /** Get an element from the pages resources dictionary,
     *  using a type (category) and a key.
     *
     *  \param type the type of resource to fetch (e.g. /Font, or /XObject)
     *  \param key the key of the resource
     *
     *  \returns the object of the resource or nullptr if it was not found
     */
    PdfObject* GetFromResources(const PdfName& type, const PdfName& key);

    /** Ensure resources initialized on this XObject
    */
    void EnsureResourcesCreated();

    bool HasRotation(double& teta) const override;

    PdfRect GetRect() const override;

    /** Set the rectangle of this xobject
     *  \param rect a rectangle
     */
    void SetRect(const PdfRect& rect);

    const PdfResources* GetResources() const override;

public:
    inline PdfResources* GetResources() { return m_Resources.get(); }

private:
    PdfXObjectForm(PdfObject& obj);

private:
    PdfObject* getContentsObject() const override;
    PdfResources& GetOrCreateResources() override;
    PdfObjectStream& GetStreamForAppending(PdfStreamAppendFlags flags) override;
    void InitXObject(const PdfRect& rect);
    void InitAfterPageInsertion(const PdfDocument& doc, unsigned pageIndex);

private:
    PdfRect m_Rect;
    PdfArray m_Matrix;
    std::unique_ptr<PdfResources> m_Resources;
};

}

#endif // PDF_XOBJECT_FORM_H
