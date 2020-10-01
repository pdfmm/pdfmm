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

#ifndef _PDF_REFERENCE_H_
#define _PDF_REFERENCE_H_

#include "PdfDefines.h"

#include "PdfDataType.h"

namespace PoDoFo {

class PdfOutputDevice;

/**
 * A reference is a pointer to a object in the PDF file of the form
 * "4 0 R", where 4 is the object number and 0 is the generation number.
 * Every object in the PDF file can be identified this way.
 *
 * This class is a indirect reference in a PDF file.
 */
class PODOFO_API PdfReference : public PdfDataType
{
public:
    /**
     * Create a PdfReference with object number and generation number
     * initialized to 0.
     */
    PdfReference()
        : m_nGenerationNo( 0 ), m_nObjectNo( 0 )
    {
    }

    /**
     * Create a PdfReference to an object with a given object and generation number.
     *
     * \param nObjectNo the object number
     * \param nGenerationNo the generation number
     */
    PdfReference( const uint32_t nObjectNo, const uint16_t nGenerationNo )
        : m_nGenerationNo( nGenerationNo ), m_nObjectNo( nObjectNo )
    {
    }

    /**
     * Create a copy of an existing PdfReference.
     * 
     * \param rhs the object to copy
     */
    PdfReference( const PdfReference & rhs ) : PdfDataType()
    {
        this->operator=( rhs );
    }

    /** Convert the reference to a string.
     *  \returns a string representation of the object.
     *
     *  \see PdfVariant::ToString
     */
    const std::string ToString() const;

   /**
     * Assign the value of another object to this PdfReference.
     *
     * \param rhs the object to copy
     */
    const PdfReference & operator=( const PdfReference & rhs );

    /** Write the complete variant to an output device.
     *  This is an overloaded member function.
     *
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or NULL to not encrypt this object
     */
    void Write( PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt ) const override;

    /** 
     * Compare to PdfReference objects.
     * \returns true if both reference the same object
     */
    bool operator==( const PdfReference & rhs ) const;

    /** 
     * Compare to PdfReference objects.
     * \returns false if both reference the same object
     */
    bool operator!=( const PdfReference & rhs ) const;

    /** 
     * Compare to PdfReference objects.
     * \returns true if this reference has a smaller object and generation number
     */
    bool operator<( const PdfReference & rhs ) const;

    /** Allows to check if a reference points to an indirect
     *  object.
     *
     *  A reference is indirect if object number and generation
     *  number are both not equal 0.
     *
     *  \returns true if this reference is the reference of
     *           an indirect object.
     */
    bool IsIndirect() const;

public:
    /** Set the object number of this object
     *  \param o the new object number
     */
    inline void SetObjectNumber(uint32_t o) { m_nObjectNo = o; }

    /** Get the object number.
     *  \returns the object number of this PdfReference
     */
    inline uint32_t ObjectNumber() const { return m_nObjectNo; }

    /** Set the generation number of this object
     *  \param g the new generation number
     */
    inline void SetGenerationNumber(const uint16_t g) { m_nGenerationNo = g; }

    /** Get the generation number.
     *  \returns the generation number of this PdfReference
     */
    inline uint16_t GenerationNumber() const { return m_nGenerationNo; }

 private:
    // uint16_t (2 bytes) should appear before uint32_t (4 bytes)
    // because this reduces sizeof(PdfObject) from 64 bytes to 56 bytes
    // on 64-bit platforms by eliminating compiler alignment padding
    // order has no effect on structure size on 32-bit platforms
    // can save up 12.5% on some documents
    uint16_t    m_nGenerationNo;
    uint32_t    m_nObjectNo;
};

};

#endif // _PDF_REFERENCE_H_


