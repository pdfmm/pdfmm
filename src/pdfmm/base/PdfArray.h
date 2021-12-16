/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ARRAY_H
#define PDF_ARRAY_H

#include "PdfDeclarations.h"
#include "PdfDataContainer.h"

namespace mm {

class PdfArray;

/**
 * Helper class to iterate through array indirect objects
 */
class PdfArrayIndirectIterator
{
    friend class PdfArray;
    typedef std::vector<PdfObject> List;

private:
    PdfArrayIndirectIterator(PdfArray& arr);

public:
    template <typename TObject, typename TListIterator>
    class Iterator
    {
        friend class PdfArrayIndirectIterator;
    public:
        using difference_type = void;
        using value_type = TObject*;
        using pointer = void;
        using reference = void;
        using iterator_category = std::forward_iterator_tag;
    private:
        Iterator(const TListIterator& iterator);
    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;
        Iterator& operator++();
        value_type operator*();
        value_type operator->();
    private:
        value_type resolve();
    private:
        TListIterator m_iterator;
    };

    using iterator = Iterator<PdfObject, List::iterator>;
    using const_iterator = Iterator<const PdfObject, List::const_iterator>;

public:
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

private:
    PdfArray* m_arr;
};

/** This class represents a PdfArray
 *  Use it for all arrays that are written to a PDF file.
 *
 *  A PdfArray can hold any PdfVariant.
 *
 *  \see PdfVariant
 */
class PDFMM_API PdfArray final : public PdfDataContainer
{
    friend class PdfObject;
public:
    typedef size_t size_type;
    typedef PdfObject value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::vector<value_type> List;
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;
    typedef List::reverse_iterator reverse_iterator;
    typedef List::const_reverse_iterator const_reverse_iterator;

    /** Create an empty array
     */
    PdfArray();

    /** Create an array and add one value to it.
     *  The value is copied.
     *
     *  \param var add this object to the array.
     */
    explicit PdfArray(const PdfObject& var);

    /** Deep copy an existing PdfArray
     *
     *  \param rhs the array to copy
     */
    PdfArray(const PdfArray& rhs);
    PdfArray(PdfArray&& rhs) noexcept;

    /** assignment operator
     *
     *  \param rhs the array to assign
     */
    PdfArray& operator=(const PdfArray& rhs);
    PdfArray& operator=(PdfArray&& rhs) noexcept;

    /**
     *  \returns the size of the array
     */
    unsigned GetSize() const;

    /**
     *  \returns true if is empty
     */
    bool IsEmpty() const;

    /** Remove all elements from the array
     */
    void Clear();

    void Write(PdfOutputDevice& device, PdfWriteMode eriteMode,
        const PdfEncrypt* encrypt) const override;

    /** Get the object at the given index out of the array.
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param idx
     *  \returns pointer to the found value. nullptr if the index was out of the boundaries
     */
    const PdfObject& FindAt(unsigned idx) const;
    PdfObject& FindAt(unsigned idx);

    void RemoveAt(unsigned idx);

    PdfObject& Add(const PdfObject& obj);

    void SetAt(const PdfObject& obj, unsigned idx);

    void AddIndirect(const PdfObject* obj);

    PdfObject& AddIndirectSafe(const PdfObject& obj);

    void SetAtIndirect(const PdfObject& obj, unsigned idx);

    PdfArrayIndirectIterator GetIndirectIterator();

    const PdfArrayIndirectIterator GetIndirectIterator() const;

    /**
     * Resize the internal vector.
     * \param count new size
     * \param value refernce value
     */
    void Resize(size_t count, const PdfObject& val = PdfObject());

public:
    /**
     *  \returns the size of the array
     */
    size_t size() const;

    PdfObject& operator[](size_type idx);
    const PdfObject& operator[](size_type idx) const;

    /**
     *  Returns a read/write iterator that points to the first
     *  element in the array.  Iteration is done in ordinary
     *  element order.
     */
    iterator begin();

    /**
     *  Returns a read-only (constant) iterator that points to the
     *  first element in the array.  Iteration is done in ordinary
     *  element order.
     */
    const_iterator begin() const;

    /**
     *  Returns a read/write iterator that points one past the last
     *  element in the array.  Iteration is done in ordinary
     *  element order.
     */
    iterator end();

    /**
     *  Returns a read-only (constant) iterator that points one past
     *  the last element in the array.  Iteration is done in
     *  ordinary element order.
     */
    const_iterator end() const;

    /**
     *  Returns a read/write reverse iterator that points to the
     *  last element in the array.  Iteration is done in reverse
     *  element order.
     */
    reverse_iterator rbegin();

    /**
     *  Returns a read-only (constant) reverse iterator that points
     *  to the last element in the array.  Iteration is done in
     *  reverse element order.
     */
    const_reverse_iterator rbegin() const;

    /**
     *  Returns a read/write reverse iterator that points to one
     *  before the first element in the array.  Iteration is done
     *  in reverse element order.
     */
    reverse_iterator rend();

    /**
     *  Returns a read-only (constant) reverse iterator that points
     *  to one before the first element in the array.  Iteration
     *  is done in reverse element order.
     */
    const_reverse_iterator rend() const;

    template<typename InputIterator>
    inline void insert(const iterator& pos, const InputIterator& first, const InputIterator& last);

    iterator insert(const iterator& pos, const PdfObject& val);

    void erase(const iterator& pos);
    void erase(const iterator& first, const iterator& last);

    void reserve(size_type n);

    /**
     *  \returns a read/write reference to the data at the first
     *           element of the array.
     */
    reference front();

    /**
     *  \returns a read-only (constant) reference to the data at the first
     *           element of the array.
     */
    const_reference front() const;

    /**
     *  \returns a read/write reference to the data at the last
     *           element of the array.
     */
    reference back();

    /**
     *  \returns a read-only (constant) reference to the data at the
     *           last element of the array.
     */
    const_reference back() const;

public:
    bool operator==(const PdfArray& rhs) const;
    bool operator!=(const PdfArray& rhs) const;

protected:
    void ResetDirtyInternal() override;
    void setChildrenParent() override;

private:
    PdfObject& add(const PdfObject& obj);
    iterator insertAt(const iterator& pos, const PdfObject& val);
    PdfObject& getAt(unsigned idx) const;
    PdfObject& findAt(unsigned idx) const;

private:
    List m_Objects;
};

template<typename InputIterator>
void PdfArray::insert(const PdfArray::iterator& pos,
    const InputIterator& first,
    const InputIterator& last)
{
    auto document = GetObjectDocument();
    iterator it1 = first;
    iterator it2 = pos;
    for (; it1 != last; it1++, it2++)
    {
        it2 = m_Objects.insert(it2, *it1);
        it2->SetDocument(document);
    }

    SetDirty();
}

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::Iterator(const TListIterator& iterator)
    : m_iterator(iterator) { }

template<typename TObject, typename TListIterator>
bool PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::operator==(const Iterator& rhs) const
{
    return m_iterator == rhs.m_iterator;
}

template<typename TObject, typename TListIterator>
bool PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::operator!=(const Iterator& rhs) const
{
    return m_iterator != rhs.m_iterator;
}

template<typename TObject, typename TListIterator>
PdfArrayIndirectIterator::Iterator<TObject, TListIterator>& PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::operator++()
{
    m_iterator++;
    return *this;
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::value_type PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::operator*()
{
    return resolve();
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::value_type PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::operator->()
{
    return resolve();
}

template<typename TObject, typename TListIterator>
typename PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::value_type PdfArrayIndirectIterator::Iterator<TObject, TListIterator>::resolve()
{
    TObject& robj = *m_iterator;
    TObject* indirectobj;
    PdfReference ref;
    if (robj.TryGetReference(ref)
        && ref.IsIndirect()
        && (indirectobj = robj.GetDocument()->GetObjects().GetObject(ref)) != nullptr)
    {
       return indirectobj;
    }
    else
    {
        return &robj;
    }
}

};

#endif // PDF_ARRAY_H
