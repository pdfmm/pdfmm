/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ARRAY_H
#define PDF_ARRAY_H

#include "PdfDefines.h"
#include "PdfDataContainer.h"

namespace mm {

class PdfArray;

class PdfArrayIterator
{
    friend class PdfArray;
    typedef std::vector<PdfObject> Array;

private:
    PdfArrayIterator(PdfArray& arr);

public:
    template <typename TObject, typename TIterator>
    class Iterator
    {
        friend class PdfArrayIterator;
    public:
        using difference_type = void;
        using value_type = TObject;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::forward_iterator_tag;
    private:
        Iterator(const TIterator& iterator);
    public:
        Iterator(const Iterator&) = default;
        Iterator& operator=(const Iterator&) = default;
        bool operator==(const Iterator& rhs) const;
        bool operator!=(const Iterator& rhs) const;
        Iterator& operator++();
        reference operator*();
        pointer operator->();
    private:
        void resolve();
    private:
        TIterator m_iterator;
        bool m_resolved;
        pointer m_ptr;
    };

    using iterator = Iterator<PdfObject, Array::iterator>;
    using const_iterator = Iterator<const PdfObject, Array::const_iterator>;

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
public:
    typedef size_t                                          size_type;
    typedef PdfObject                                       value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::vector<value_type>::iterator               iterator;
    typedef std::vector<value_type>::const_iterator         const_iterator;
    typedef std::vector<value_type>::reverse_iterator       reverse_iterator;
    typedef std::vector<value_type>::const_reverse_iterator const_reverse_iterator;

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

    /** assignment operator
     *
     *  \param rhs the array to assign
     */
    PdfArray& operator=(const PdfArray& rhs);

    /**
     *  \returns the size of the array
     */
    unsigned GetSize() const;

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

    void Add(const PdfObject& obj);

    void SetAt(const PdfObject& obj, unsigned idx);

    void AddIndirect(const PdfObject& obj);

    void SetAtIndirect(const PdfObject& obj, unsigned idx);

    PdfArrayIterator GetIndirectIterator();

    const PdfArrayIterator GetIndirectIterator() const;

public:
    /** Adds a PdfObject to the array
     *
     *  \param var add a PdfObject to the array
     *
     *  This will set the dirty flag of this object.
     *  \see IsDirty
     */
    void push_back(const PdfObject& obj);

    /** Remove all elements from the array
     */
    void clear();

    /**
     *  \returns the size of the array
     */
    size_t size() const;

    /**
     *  \returns true if the array is empty.
     */
    bool empty() const;

    PdfObject& operator[](size_type idx);
    const PdfObject& operator[](size_type idx) const;

    /**
     * Resize the internal vector.
     * \param count new size
     * \param value refernce value
     */
    void resize(size_t count, const PdfObject& val = PdfObject());

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
    void SetOwner(PdfObject* owner) override;

private:
    void add(const PdfObject& obj);
    iterator insertAt(const iterator& pos, const PdfObject& val);
    PdfObject& getAt(unsigned idx) const;
    PdfObject& findAt(unsigned idx) const;

private:
    std::vector<PdfObject> m_Objects;
};

template<typename InputIterator>
void PdfArray::insert(const PdfArray::iterator& pos,
    const InputIterator& first,
    const InputIterator& last)
{
    AssertMutable();

    auto document = GetObjectDocument();
    iterator it1 = first;
    iterator it2 = pos;
    for (; it1 != last; it1++, it2++)
    {
        it2 = m_Objects.insert(it2, *it1);
        if (document != nullptr)
            it2->SetDocument(*document);
    }

    SetDirty();
}

template<typename TObject, typename TIterator>
PdfArrayIterator::Iterator<TObject, TIterator>::Iterator(const TIterator& iterator)
    : m_iterator(iterator), m_resolved(false), m_ptr(nullptr) { }

template<typename TObject, typename TIterator>
inline bool PdfArrayIterator::Iterator<TObject, TIterator>::operator==(const Iterator& rhs) const
{
    return m_iterator == rhs.m_iterator;
}

template<typename TObject, typename TIterator>
bool PdfArrayIterator::Iterator<TObject, TIterator>::operator!=(const Iterator& rhs) const
{
    return m_iterator != rhs.m_iterator;
}

template<typename TObject, typename TIterator>
PdfArrayIterator::Iterator<TObject, TIterator>& PdfArrayIterator::Iterator<TObject, TIterator>::operator++()
{
    m_iterator++;
    m_resolved = false;
    return *this;
}

template<typename TObject, typename TIterator>
typename PdfArrayIterator::Iterator<TObject, TIterator>::reference PdfArrayIterator::Iterator<TObject, TIterator>::operator*()
{
    resolve();
    return *m_ptr;
}

template<typename TObject, typename TIterator>
typename PdfArrayIterator::Iterator<TObject, TIterator>::pointer PdfArrayIterator::Iterator<TObject, TIterator>::operator->()
{
    resolve();
    return m_ptr;
}

template<typename TObject, typename TIterator>
inline void PdfArrayIterator::Iterator<TObject, TIterator>::resolve()
{
    if (m_resolved)
        return;

    TObject& robj = *m_iterator;
    TObject* indirectobj;
    PdfReference ref;
    if (robj.TryGetReference(ref)
        && ref.IsIndirect()
        && (indirectobj = robj.GetDocument()->GetObjects().GetObject(ref)) != nullptr)
    {
        m_ptr = indirectobj;
    }
    else
    {
        m_ptr = &robj;
    }

    m_resolved = true;
}

};

#endif // PDF_ARRAY_H
