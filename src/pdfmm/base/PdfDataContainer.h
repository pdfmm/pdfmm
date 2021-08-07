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
    /** Create a new PdfDataOwnedType.
     *  Can only be called by subclasses
     */
    PdfDataContainer();

    PdfDataContainer(const PdfDataContainer& rhs);

public:
    PdfDataContainer& operator=(const PdfDataContainer& rhs);

public:
    /** \returns a pointer to a PdfObject that is the
     *           owner of this data type.
     *           Might be nullptr if the data type has no owner.
     */
    inline const PdfObject* GetOwner() const { return m_Owner; }
    inline PdfObject* GetOwner() { return m_Owner; }

    /**
     * Retrieve if an object is immutable.
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so
     * that no keys can be added to an object after setting stream data on it.
     *
     * \returns true if the object is immutable
     */
    bool IsImmutable() const { return m_IsImmutable; }

    /**
     * Sets this object to immutable,
     * so that no keys can be edited or changed.
     *
     * \param bImmutable if true set the object to be immutable
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so
     * that no keys can be added to an object after setting stream data on it.
     *
     */
    void SetImmutable(bool immutable) { m_IsImmutable = immutable; }

protected:
    void AssertMutable() const;
    virtual void ResetDirtyInternal() = 0;
    PdfObject& GetIndirectObject(const PdfReference& reference) const;
    PdfDocument* GetObjectDocument();
    virtual void SetOwner(PdfObject* owner);
    void SetDirty();
    bool IsIndirectReferenceAllowed(const PdfObject& obj);

    /** Templatized object type getter helper
     */
    template <typename T>
    struct Object
    {
        static T Get(const PdfObject& obj)
        {
            static_assert(false, "Unsupported type");
        }
    };

    template <>
    struct Object<bool>
    {
        static bool Get(const PdfObject& obj)
        {
            return obj.GetBool();
        }
    };

    template <>
    struct Object<int64_t>
    {
        static int64_t Get(const PdfObject& obj)
        {
            return obj.GetNumber();
        }
    };

    template <>
    struct Object<double>
    {
        static double Get(const PdfObject& obj)
        {
            return obj.GetReal();
        }
    };

    template <>
    struct Object<PdfReference>
    {
        static PdfReference Get(const PdfObject& obj)
        {
            return obj.GetReference();
        }
    };

    template <>
    struct Object<PdfName>
    {
        static PdfName Get(const PdfObject& obj)
        {
            return obj.GetName();
        }
    };

    template <>
    struct Object<PdfString>
    {
        static PdfString Get(const PdfObject& obj)
        {
            return obj.GetString();
        }
    };

private:
    void ResetDirty();

private:
    PdfObject* m_Owner;
    bool m_IsImmutable;
};

}

#endif // PDF_CONTAINER_DATATYPE_H
