/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_REFERENCE_H
#define PDF_REFERENCE_H

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
class PODOFO_API PdfReference final : public PdfDataType
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

    void Write(PdfOutputDevice& pDevice, PdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt) const override;

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

#endif // PDF_REFERENCE_H
