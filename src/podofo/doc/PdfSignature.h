/**
 * Copyright (C) 2011 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2011 by Petr Pytelka
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_SIGNATURE_H
#define PDF_SIGNATURE_H

#include "PdfAnnotation.h"
#include "PdfField.h"
#include "podofo/base/PdfDate.h"
#include "podofo/base/PdfData.h"

namespace PoDoFo {

enum class EPdfCertPermission
{
    NoPerms = 1,
    FormFill = 2,
    Annotations = 3,
};

struct PdfSignatureBeacons
{
    PdfSignatureBeacons();
    std::string ContentsBeacon;
    std::string ByteRangeBeacon;
    std::shared_ptr<size_t> ContentsOffset;
    std::shared_ptr<size_t> ByteRangeOffset;
};

class PODOFO_DOC_API PdfSignature : public PdfField
{
public:
    PdfSignature(PdfPage& page, const PdfRect& rect);

    /** Create a new PdfSignature
     *  \param bInit creates a signature field with/without a /V key
     */
    PdfSignature(PdfDocument& doc, PdfAnnotation* widget, bool insertInAcroform);

    /** Creates a PdfSignature from an existing PdfAnnotation, which should
     *  be an annotation with a field type Sig.
     *	\param pObject the object
     *	\param pWidget the annotation to create from
     */
    PdfSignature(PdfObject& obj, PdfAnnotation* widget);

    /** Set an appearance stream for this signature field
     *  to specify its visual appearance
     *  \param pObject an XObject
     *  \param eAppearance an appearance type to set
     *  \param state the state for which set it the pObject; states depend on the annotation type
     */
    void SetAppearanceStream(PdfXObject& pObject, PdfAnnotationAppearance eAppearance = PdfAnnotationAppearance::Normal, const PdfName& state = "");

    /** Create space for signature
     *
     * Structure of the PDF file - before signing:
     * <</ByteRange[ 0 1234567890 1234567890 1234567890]/Contents<signatureData>
     * Have to be replaiced with the following structure:
     * <</ByteRange[ 0 count pos count]/Contents<real signature ...0-padding>
     *
     * \param filter /Filter for this signature
     * \param subFilter /SubFilter for this signature
     * \param subFilter /Type for this signature
     * \param beacons Shared sentinels that will updated
     *                during writing of the document
     */
    void PrepareForSigning(const std::string_view& filter,
        const std::string_view& subFilter,
        const std::string_view& type,
        const PdfSignatureBeacons& beacons);

    /** Set the signer name
    *
    *  \param rsText the signer name
    */
    void SetSignerName(const PdfString & rsText);

    /** Set reason of the signature
     *
     *  \param rsText the reason of signature
     */
    void SetSignatureReason(const PdfString & rsText);

    /** Set location of the signature
     *
     *  \param rsText the location of signature
     */
    void SetSignatureLocation(const PdfString & rsText);

    /** Set the creator of the signature
     *
     *  \param creator the creator of the signature
     */
    void SetSignatureCreator( const PdfName & creator );

    /** Date of signature
     */
    void SetSignatureDate(const PdfDate &sigDate);

    /** Add certification dictionaries and references to document catalog.
     *
     *  \param pDocumentCatalog the catalog of current document
     *  \param perm document modification permission
     */
    void AddCertificationReference(PdfObject *pDocumentCatalog, EPdfCertPermission perm = EPdfCertPermission::NoPerms);

    /** Get the signer name
    *
    *  \returns the found signer object
    */
    const PdfObject * GetSignerName() const;

    /** Get the reason of the signature
    *
    *  \returns the found reason object
    */
    const PdfObject * GetSignatureReason() const;

    /** Get the location of the signature
    *
    *  \returns the found location object
    */
    const PdfObject * GetSignatureLocation() const;

    /** Get the date of the signature
    *
    *  \returns the found date object
    */
    const PdfObject * GetSignatureDate() const;

    /** Returns signature object for this signature field.
     *  It can be nullptr, when the signature field was created
     *  from an existing annotation and it didn't have set it.
     *
     *  \returns associated signature object, or nullptr
     */
    PdfObject* GetSignatureObject() const;

    /** Ensures that the signature field has set a signature object.
     *  The function does nothing, if the signature object is already
     *  set. This is useful for cases when the signature field had been
     *  created from an existing annotation, which didn't have it set.
     */
    void EnsureSignatureObject();

private:
    void Init(PdfAcroForm &acroForm);

private:
    PdfObject* m_pSignatureObj;
};

}

#endif // PDF_SIGNATURE_H