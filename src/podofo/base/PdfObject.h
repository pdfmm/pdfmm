/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_OBJECT_H_
#define _PDF_OBJECT_H_

#include <memory>

#include "PdfName.h"
#include "PdfReference.h"
#include "PdfString.h"
#include "PdfVariant.h"
#include "PdfStream.h"
#include "PdfContainerDataType.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfObject;
class PdfOutputDevice;
class PdfVecObjects;
class PdfDictionary;
class PdfArray;
class PdfDocument;

/**
 * This class represents a PDF indirect Object in memory
 * 
 * It is possible to manipulate the stream which can be appended to the object
 * (if the object is of underlying type dictionary).  A PdfObject is uniquely
 * identified by an object number and a generation number which has to be
 * passed to the constructor.
 *
 * The object can be written to a file easily using the WriteObject() function.
 *
 * \see WriteObject()
 */
class PODOFO_API PdfObject : public PdfVariant
{
    friend class PdfVecObjects;
    friend class PdfArray;
    friend class PdfDictionary;
    friend class PdfDocument;
    friend class PdfStream;

public:

    /** Create a PDF object with object and generation number -1
     *  and the value of being an empty PdfDictionary.
     */
    PdfObject();

    /** Construct a new PDF object of type PdfDictionary.
     *
     *  \param rRef reference of this object
     *  \param pszType if this parameter is not null a key "/Type" will
     *                 be added to the dictionary with the parameter's value.
     */
    PdfObject( const PdfReference & rRef, const char* pszType);

    /** Create a PDF object with object and generation number -1
     *  and the value of the passed variant.
     *
     *  \param var the value of the object
     */
    PdfObject( const PdfVariant & var );

    /** Construct a PdfObject with object and generation number -1
     *  and a bool as value.
     *
     *  \param b the boolean value of this PdfObject
     */
    PdfObject( bool b );

    /** Construct a PdfObject with object and generation number -1
     *  and a int64_t as value.
     *
     *  \param l the int64_t value of this PdfObject
     */
    PdfObject( int64_t l );

    /** Construct a PdfObject with object and generation number -1
     *  and a double as value.
     *
     *  \param d the double value of this PdfObject
     */
    PdfObject( double d );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfString as value.
     *
     *  \param rsString the string value of this PdfObject
     */        
    PdfObject( const PdfString & rsString );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfName as value.
     *
     *  \param rName the value of this PdfObject
     */        
    PdfObject( const PdfName & rName );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfReference as value.
     *
     *  \param rRef the value of the this PdfObject
     */        
    PdfObject( const PdfReference & rRef );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfArray as value.
     *
     *  \param tList the value of the this PdfObject
     */        
    PdfObject( const PdfArray & tList );

    /** Construct a PdfObject with object and generation number -1
     *  and a PdfDictionary as value.
     *
     *  \param rDict the value of the this PdfObject
     */        
    PdfObject( const PdfDictionary & rDict );

    /** Creates a copy of an existing PdfObject.
     *  All associated objects and streams will be copied along with the PdfObject.
     *  \param rhs PdfObject to clone
     */
    PdfObject( const PdfObject & rhs );

    void ForceCreateStream();

    /** Get the key's value out of the dictionary. If the key is a reference, 
     *  the reference is resolved and the object pointed to by the reference is returned.
     * REMOVE-ME
     *  \param key look for the key named key in the dictionary
     * 
     *  \returns the found value or NULL if the value is not in the 
     *           dictionary or if this object is no dictionary
     */
    PdfObject* GetIndirectKey( const PdfName & key ) const;
    
    /**
     * MustGetIndirectKey() wraps GetIndirectKey to throw on null return.
     * This makes it MUCH more readable to look up deep chains of linked keys
     * with the cost that it's not easy to tell at which point a missing key/object
     * was encountered.
     * REMOVE-ME
     * \returns the found value, which is never null
     * \throws PdfError(EPdfError::NoObject) .
     */
    PdfObject* MustGetIndirectKey( const PdfName & key ) const;

    /** Write the complete object to a file.
     *  \param pDevice write the object to this device
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     *  \param eWriteMode additional options for writing the object
     *  \param keyStop if not KeyNull and a key == keyStop is found
     *                 writing will stop right before this key!
     */
    void WriteObject( PdfOutputDevice* pDevice, EPdfWriteMode eWriteMode, PdfEncrypt* pEncrypt,
                      const PdfName & keyStop = PdfName::KeyNull ) const;

    /** Get the length of the object in bytes if it was written to disk now.
     *  \param eWriteMode additional options for writing the object
     *  \returns  the length of the object
     */
    size_t GetObjectLength( EPdfWriteMode eWriteMode );

    /** Get a handle to a PDF stream object.
     *  If the PDF object does not have a stream,
     *  one will be created.
     *  \returns a PdfStream object
     */
    PdfStream & GetOrCreateStream();

    /** Get a handle to a const PDF stream object.
     *  If the PDF object does not have a stream,
     *  null is returned.
     *  \returns a PdfStream object or null
     */
    const PdfStream * GetStream() const;

    /** Get a handle to a const PDF stream object.
     *  If the PDF object does not have a stream,
     *  null is returned.
     *  \returns a PdfStream object or null
     */
    PdfStream * GetStream();

    /** Check if this object has a PdfStream object
     *  appended.
     * 
     *  \returns true if the object has a stream
     */
    bool HasStream() const;

    /** This operator is required for sorting a list of 
     *  PdfObject instances. It compares the object number. If object numbers
     *  are equal, the generation number is compared.
     */
    bool operator<( const PdfObject & rhs ) const;

    // CHECK-ME: Investigate better on equality of PdfObject

        // REWRITE-ME: The equality operator is pure shit
    /** Comparison operator.
     *  Compares two PDF object instances only based on their object and generation number.
     */
    bool operator==( const PdfObject & rhs ) const;

    /** Creates a copy of an existing PdfObject.
     *  All associated objects and streams will be copied along with the PdfObject.
     *  \param rhs PdfObject to clone
     *  \returns a reference to this object
     */
    const PdfObject & operator=( const PdfObject & rhs );

    /** This function compresses any currently set stream
     *  using the FlateDecode algorithm. JPEG compressed streams
     *  will not be compressed again using this function.
     *  Entries to the filter dictionary will be added if necessary.
     */
    void FlateCompressStream();

    /** Calculate the byte offset of the key pszKey from the start of the object
     *  if the object was written to disk at the moment of calling this function.
     *
     *  This function is very calculation intensive!
     *
     *  \param pszKey  key to calculate the byte offset of
     *  \param eWriteMode additional options for writing the PDF
     *  \returns the offset of the key 
     */
    size_t GetByteOffset( const char* pszKey, EPdfWriteMode eWriteMode );

public:
    /** Get the document of this object.
     *  \return the owner (if it wasn't changed anywhere, creator) of this object
     */
    inline PdfDocument* GetDocument() const { return m_Document; }

    /** Get an indirect reference to this object.
     *  \returns a PdfReference pointing to this object.
     */
    inline const PdfReference& GetIndirectReference() const { return m_reference; }

    inline const PdfContainerDataType* GetParent() const { return m_Parent; }

protected:
    /** Set the owner of this object, i.e. the PdfVecObjects to which
     *  this object belongs.
     *
     *  \param pVecObjects a vector of pdf objects
     */
    void SetDocument(PdfDocument &document);

    void SetIndirectReference(const PdfReference &reference);

    void AfterDelayedLoad() override;

    void SetVariantOwner();

    void FreeStream();

    PdfStream & getOrCreateStream();

    void forceCreateStream();

    PdfStream * getStream();

    void DelayedLoadStream() const;

    void delayedLoadStream() const;

    void EnableDelayedLoadingStream();

    virtual void DelayedLoadStreamImpl();

 private:
    /* See PdfVariant.h for a detailed explanation of this member, which is
     * here to prevent accidental construction of a PdfObject of integer type
     * when passing a pointer. */
    template<typename T> PdfObject(T*);

    void copyFrom(const PdfObject &obj);

    // Shared initialization between all the ctors
    void InitPdfObject();

 private:
     // Order of member variables has significant effect on sizeof(PdfObject) which
     // is important in PDFs with many objects (PDF32000_2008.pdf has 750,000 PdfObjects),
     // be very careful to test class sizes on 32-bit and 64-bit platforms when adding 
     // or re-ordering objects.
	 PdfReference m_reference;
     PdfDocument* m_Document;
     PdfContainerDataType* m_Parent;
     mutable bool m_DelayedLoadStreamDone;
	 std::unique_ptr<PdfStream> m_pStream;
    // Tracks whether deferred loading is still pending (in which case it'll be
    // false). If true, deferred loading is not required or has been completed.
};

};

#endif // _PDF_OBJECT_H_



