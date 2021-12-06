/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ACRO_FORM_H
#define PDF_ACRO_FORM_H

#include "PdfDeclarations.h"

#include "PdfElement.h"

namespace mm {

class PdfDocument;

enum  class PdfAcroFormDefaulAppearance
{
    None, ///< Do not add a default appearrance
    BlackText12pt ///< Add a default appearance with Arial embedded and black text 12pt if no other DA key is present
};

class PDFMM_API PdfAcroForm final : public PdfDictionaryElement
{
public:

    /** Create a new PdfAcroForm dictionary object
     *  \param doc parent of this action
     *  \param defaultAppearance specifies if a default appearance should be added
     */
    PdfAcroForm(PdfDocument & doc,
                 PdfAcroFormDefaulAppearance defaultAppearance = PdfAcroFormDefaulAppearance::BlackText12pt);

    /** Create a PdfAcroForm dictionary object from an existing PdfObject
     *	\param obj the object to create from
     */
    PdfAcroForm(PdfObject& obj);

    PdfArray& GetFieldsArray();

    /** Set the value of the NeedAppearances key in the interactive forms
     *  dictionary.
     *
     *  \param bNeedAppearances A flag specifying whether to construct appearance streams
     *                          and appearance dictionaries for all widget annotations in
     *                          the document. Default value is false.
     */
    void SetNeedAppearances(bool needAppearances);

    /** Retrieve the value of the NeedAppearances key in the interactive forms
     *  dictionary.
     *
     *  \returns value of the NeedAppearances key
     *
     *  \see SetNeedAppearances
     */
    bool GetNeedAppearances() const;

private:
    /** Initialize this object
     *  with a default appearance
     *  \param defaultAppearance specifies if a default appearance should be added
     */
    void Init(PdfAcroFormDefaulAppearance defaultAppearance);
};

};

#endif // PDF_ACRO_FORM_H
