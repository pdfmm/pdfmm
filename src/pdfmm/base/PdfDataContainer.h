/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CONTAINER_DATATYPE_H
#define PDF_CONTAINER_DATATYPE_H

#include "PdfDataProvider.h"
#include "PdfObject.h"

namespace mm {

class PdfObject;
class PdfDocument;
class PdfReference;

/**
 * A PdfDataProvider object with a PdfObject owner, specialized
 * in holding objects
 */
class PDFMM_API PdfDataContainer : public PdfDataProvider
{
    friend class PdfObject;

protected:
    /** Create a new PdfDataOwnedType
     * Can only be called by subclasses
     * \remarks We don't define copy/move constructor as the
     * the owner is not copied/movied
     */
    PdfDataContainer();

public:
    /** \returns a pointer to a PdfObject that is the
     *           owner of this data type.
     *           Might be nullptr if the data type has no owner.
     */
    inline const PdfObject* GetOwner() const { return m_Owner; }
    inline PdfObject* GetOwner() { return m_Owner; }

protected:
    virtual void ResetDirtyInternal() = 0;
    PdfObject& GetIndirectObject(const PdfReference& reference) const;
    PdfDocument* GetObjectDocument();
    void SetDirty();
    bool IsIndirectReferenceAllowed(const PdfObject& obj);
    virtual void setChildrenParent() = 0;

private:
    void SetOwner(PdfObject& owner);
    void ResetDirty();

private:
    PdfObject* m_Owner;
};

}

#endif // PDF_CONTAINER_DATATYPE_H
