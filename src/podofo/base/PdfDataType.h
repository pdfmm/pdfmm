/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DATATYPE_H
#define PDF_DATATYPE_H

#include "PdfDefines.h"

namespace PoDoFo {

class PdfEncrypt;
class PdfOutputDevice;

/** An interface for all PDF datatype classes.
 *
 *  
 *  \see PdfName \see PdfArray \see PdfReference 
 *  \see PdfVariant \see PdfDictionary \see PdfString
 */
class PODOFO_API PdfDataType
{
protected:
    /** Create a new PdfDataType.
     *  Can only be called by subclasses
     */
    PdfDataType();

public:
    virtual ~PdfDataType();

    /** Write the complete datatype to a file.
     *  \param pDevice write the object to this device
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object
     */
    virtual void Write(PdfOutputDevice& pDevice, PdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt) const = 0;
};

}

#endif // PDF_DATATYPE_H
