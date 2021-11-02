/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_XOBJECT_H
#define PDF_XOBJECT_H

#include "PdfElement.h"
#include "PdfArray.h"
#include "PdfRect.h"

namespace mm {

class PdfObject;
class PdfImage;
class PdfXObjectForm;
class PdfXObjectPostScript;

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 *
 *  \see PdfPainter
 */
class PDFMM_API PdfXObject : public PdfDictionaryElement
{
protected:
    PdfXObject(PdfDocument& doc, PdfXObjectType subType, const std::string_view& prefix);
    PdfXObject(PdfObject& obj, PdfXObjectType subType);

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfXObject>& xobj);

    template <typename XObjectT>
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<XObjectT>& xobj);

    static std::string ToString(PdfXObjectType type);
    static PdfXObjectType FromString(const std::string& str);

    virtual PdfRect GetRect() const = 0;

    /** Get the identifier used for drawig this object
     *  \returns identifier
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    inline PdfXObjectType GetType() const { return m_Type; }

private:
    void initIdentifiers(const std::string_view& prefix);
    static PdfXObjectType getPdfXObjectType(const PdfObject& obj);

private:
    PdfXObjectType m_Type;
    PdfName m_Identifier;
};

template<typename XObjectT>
inline bool PdfXObject::TryCreateFromObject(PdfObject& obj, std::unique_ptr<XObjectT>& xobj)
{
    PdfXObjectType xobjtype;
    auto type = &typeid(XObjectT);
    if (type == &typeid(PdfXObjectForm))
    {
        xobjtype = PdfXObjectType::Form;
    }
    else if (type == &typeid(PdfImage))
    {
        xobjtype = PdfXObjectType::Image;
    }
    else if (type == &typeid(PdfXObjectPostScript))
    {
        xobjtype = PdfXObjectType::PostScript;
    }
    else
    {
        xobj.reset();
        return false;
    }


    std::unique_ptr<PdfXObject> xobj_;
    if (!TryCreateFromObject(obj, xobj_)
        || xobj_->GetType() != xobjtype)
    {
        return false;
    }

    xobj.reset((XObjectT*)xobj_.release());
    return true;
}

};

#endif // PDF_XOBJECT_H


