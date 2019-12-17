/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
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

#ifndef _PDF_OWNED_DATATYPE_H_
#define _PDF_OWNED_DATATYPE_H_

#include "PdfDataType.h"

namespace PoDoFo {

class PdfObject;
class PdfVecObjects;
class PdfReference;

/**
 * A PdfDataType object with PdfObject owner
 */
class PODOFO_API PdfContainerDataType : public PdfDataType
{
    friend class PdfObject;
protected:
    /** Create a new PdfDataOwnedType.
     *  Can only be called by subclasses
     */
    PdfContainerDataType();

    PdfContainerDataType( const PdfContainerDataType &rhs );

public:
    /** \returns a pointer to a PdfVecObjects that is the
     *           owner of this data type.
     *           Might be NULL if the data type has no owner.
     */
    inline const PdfObject* GetOwner() const { return m_pOwner; }
    inline PdfObject* GetOwner() { return m_pOwner; }

    /** Sets the dirty flag of this container
     *
     *  \param bDirty true if this PdfVariant has been
     *                modified from the outside
     *
     *  \see IsDirty
     */
    void SetDirty();

    /**
     * Sets this object to immutable,
     * so that no keys can be edited or changed.
     *
     * @param bImmutable if true set the object to be immutable
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so 
     * that no keys can be added to an object after setting stream data on it.
     *
     */
    void SetImmutable(bool immutable) { m_isImmutable = immutable; }

    // TODO: IsDirty in a container should be modified automatically by its children??? YES! And stop on first parent not dirty
    // When done, remove virtual
    /** The dirty flag is set if this container
     *  has been modified after construction.
     *  
     *  Usually the dirty flag is also set
     *  if you call any non-const member function
     *  as we cannot determine if you actually changed 
     *  something or not.
     *
     *  \returns true if the value is dirty and has been 
     *                modified since construction
     */
    virtual bool IsDirty() const { return m_isDirty; }

    /**
     * Retrieve if an object is immutable.
     *
     * This is used by PdfImmediateWriter and PdfStreamedDocument so 
     * that no keys can be added to an object after setting stream data on it.
     *
     * @returns true if the object is immutable
     */
    bool IsImmutable() const { return m_isImmutable; }

    PdfContainerDataType & operator=( const PdfContainerDataType &rhs );

protected:
    void AssertMutable() const;
    virtual void ResetDirtyInternal() = 0;
    PdfObject & GetIndirectObject( const PdfReference &rReference ) const;
    PdfVecObjects * GetObjectOwner();
    virtual void SetOwner( PdfObject *pOwner );

private:
    void ResetDirty();
private:
    PdfObject *m_pOwner;
    bool m_isImmutable;
    bool m_isDirty;
};

}; // namespace PoDoFo

#endif /* _PDF_OWNED_DATATYPE_H_ */
