/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XOBJECT_H
#define PDF_XOBJECT_H

#include "PdfDefines.h"

#include "PdfElement.h"
#include "PdfArray.h"
#include "PdfCanvas.h"
#include "PdfRect.h"

namespace mm {

class PdfDictionary;
class PdfObject;

enum class PdfXObjectType
{
    Unknown = 0,
    Form,
    Image,
    PostScript,
};

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 *
 *  \see PdfPainter
 */
class PDFMM_API PdfXObject : public PdfDictionaryElement, public PdfCanvas
{
public:
    /** Create a new XObject with a specified dimension
     *  in a given document
     *
     *  \param doc the parent document of the XObject
     *  \param rect the size of the XObject
     *  \param prefix optional prefix for XObject-name
     *  \param withoutObjNum do not create an object identifier name
     */
    PdfXObject(PdfDocument& doc, const PdfRect& rect, const std::string_view& prefix = { }, bool withoutObjNum = false);

    /** Create a new XObject from a page of another document
     *  in a given document
     *
     *  \param doc the parent document of the XObject
     *  \param sourceDoc the document to create the XObject from
     *  \param pageIndex the page-number in doc to create the XObject from
     *  \param prefix optional prefix for XObject-name
     *	\param useTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObject(PdfDocument& doc, const PdfDocument& sourceDoc, unsigned pageIndex, const std::string_view& prefix = { }, bool useTrimBox = false);

    /** Create a new XObject from an existing page
     *
     *  \param doc the document to create the XObject at
     *  \param pageIndex the page-number in doc to create the XObject from
     *  \param prefix optional prefix for XObject-name
     *  \param useTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObject(PdfDocument& doc, unsigned pageIndex, const std::string_view& prefix = { }, bool useTrimBox = false);

    /** Create a XObject from an existing PdfObject
     *
     *  \param obj an existing object which has to be
     *                 a XObject
     */
    PdfXObject(PdfObject& obj);

protected:
    PdfXObject(PdfDocument& doc, PdfXObjectType subType, const std::string_view& prefix);
    PdfXObject(PdfObject& obj, PdfXObjectType subType);

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfXObject>& xobj, PdfXObjectType& type);

    static std::string ToString(PdfXObjectType type);
    static PdfXObjectType FromString(const std::string& str);

    PdfRect GetRect() const override;

    bool HasRotation(double& teta) const override;

    /** Set the rectangle of this xobject
    *  \param rect a rectangle
    */
    void SetRect(const PdfRect& rect);

    /** Ensure resources initialized on this XObject
    */
    void EnsureResourcesInitialized();

    PdfObject& GetOrCreateResources() override;

    inline const PdfObject* GetResources() const { return m_Resources; }
    inline PdfObject* GetResources() { return m_Resources; }

    /** Get the identifier used for drawig this object
     *  \returns identifier
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    /** Get the reference to the XObject in the PDF file
     *  without having to access the PdfObject.
     *
     *  This allows to work with XObjects which have been
     *  written to disk already.
     *
     *  \returns the reference of the PdfObject for this XObject
     */
    inline const PdfReference& GetObjectReference() const { return m_Reference; }

    inline PdfXObjectType GetType() const { return m_type; }

private:
    PdfObject& GetOrCreateContents() override;

private:
    static PdfXObjectType getPdfXObjectType(const PdfObject& obj);
    void InitXObject(const PdfRect& rect, const std::string_view& prefix);
    void InitIdentifiers(PdfXObjectType subType, const std::string_view& prefix);
    void InitAfterPageInsertion(const PdfDocument& doc, unsigned pageIndex);
    void InitResources();
    PdfStream& GetStreamForAppending(PdfStreamAppendFlags flags) override;

private:
    PdfRect m_Rect;
    PdfXObjectType m_type;
    PdfArray m_matrix;
    PdfObject* m_Resources;
    PdfName m_Identifier;
    PdfReference m_Reference;
};

};

#endif // PDF_XOBJECT_H


