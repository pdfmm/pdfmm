/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DATATYPE_H
#define PDF_DATATYPE_H

#include "PdfDeclarations.h"

namespace mm {

class PdfEncrypt;
class PdfOutputDevice;

/** An interface for data provider classes that are stored in a PdfVariant
 *  
 *  \see PdfName \see PdfArray \see PdfReference 
 *  \see PdfVariant \see PdfDictionary \see PdfString
 */
class PDFMM_API PdfDataProvider
{
protected:
    /** Create a new PdfDataProvider.
     *  Can only be called by subclasses
     */
    PdfDataProvider();

public:
    virtual ~PdfDataProvider();

    /** Write the complete datatype to a file.
     *  \param device write the object to this device
     *  \param writeMode additional options for writing this object
     *  \param encrypt an encryption object which is used to encrypt this object
     *                  or nullptr to not encrypt this object
     */
    virtual void Write(PdfOutputDevice& device, PdfWriteMode writeMode, const PdfEncrypt* encrypt) const = 0;
};

}

#endif // PDF_DATATYPE_H
