/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#ifndef _PDF_ARRAY_H_
#define _PDF_ARRAY_H_

#include "PdfDefines.h"
#include "PdfContainerDataType.h"
#include "PdfObject.h"

namespace PoDoFo {

/** This class represents a PdfArray
 *  Use it for all arrays that are written to a PDF file.
 *  
 *  A PdfArray can hold any PdfVariant.
 *
 *  \see PdfVariant
 */
class PODOFO_API PdfArray : public PdfContainerDataType
{
public:
    typedef size_t                                          size_type;
    typedef PdfObject                                       value_type;
    typedef value_type &                                    reference;
    typedef const value_type &                              const_reference;
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
    explicit PdfArray( const PdfObject & var );

    /** Deep copy an existing PdfArray
     *
     *  \param rhs the array to copy
     */
    PdfArray( const PdfArray & rhs );

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

    /** Write the array to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object
     */
    void Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode,
        const PdfEncrypt* pEncrypt) const override;

    /** Get the object at the given index out of the array.
     *
     * Lookup in the indirect objects as well, if the shallow object was a reference.
     * The returned value is a pointer to the internal object in the dictionary
     * so it MUST not be deleted.
     *
     *  \param idx
     *  \returns pointer to the found value. nullptr if the index was out of the boundaries
     */
    const PdfObject & FindAt(unsigned idx) const;
    PdfObject & FindAt(unsigned idx );

    void RemoveAt(size_t index);

    void Add(const PdfObject& obj);

    void SetAt(const PdfObject& obj, size_t idx);
    
    void AddIndirect(const PdfObject& obj);

    void SetAtIndirect(const PdfObject& obj, size_t idx);

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

    PdfObject & operator[](size_t n);
    const PdfObject & operator[](size_t n) const;

    /**
     * Resize the internal vector.
     * \param count new size
     * \param value refernce value
     */
    void resize(size_t count, const PdfObject &val = PdfObject());
    
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

    iterator insert( const iterator &pos, const PdfObject &val );

    void erase( const iterator& pos );
    void erase( const iterator& first, const iterator& last );

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
    bool operator==( const PdfArray & rhs ) const;
    bool operator!=( const PdfArray & rhs ) const;

protected:
     void ResetDirtyInternal() override;
     void SetOwner( PdfObject* pOwner ) override;

private:
    void add(const PdfObject& obj);
    iterator insertAt(const iterator& pos, const PdfObject& val);
    PdfObject & findAt(unsigned idx) const;

 private:
    std::vector<PdfObject> m_objects;
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
    for ( ; it1 != last; it1++, it2++ )
    {
        it2 = m_objects.insert( it2, *it1 );
        if (document != nullptr)
            it2->SetDocument(*document);
    }

    SetDirty();
}

};

#endif // _PDF_ARRAY_H_
