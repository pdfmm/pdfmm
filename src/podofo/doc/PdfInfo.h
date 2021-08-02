/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_INFO_H
#define PDF_INFO_H

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfName.h"
#include "podofo/base/PdfDate.h"
#include "PdfElement.h"

namespace PoDoFo
{

class PdfString;

/** This class provides access to the documents
 *  info dictionary, which provides information
 *  about the PDF document.
 */
class PODOFO_DOC_API PdfInfo final : public PdfElement
{
public:

    /** Create a new PdfInfo object
     *  \param pParent the parent of this object
     *  \param eInitial which information should be
     *         writting initially to the information dictionary
     */
    PdfInfo(PdfDocument& doc,
        PdfInfoInitial eInitial = PdfInfoInitial::WriteCreationTime | PdfInfoInitial::WriteProducer);

    /** Create a PdfInfo object from an existing
     *  object in the PDF file.
     *  \param pObject must be an info dictionary.
     *  \param eInitial which information should be
     *         writting initially to the information
     */
    PdfInfo(PdfObject& obj, PdfInfoInitial eInitial = PdfInfoInitial::None);

    /** Set the author of the document.
     *  \param sAuthor author
     */
    void SetAuthor(const PdfString& sAuthor);

    /** Get the author of the document
     *  \returns the author
     */
    std::optional<PdfString> GetAuthor() const;

    /** Set the creator of the document.
     *  Typically the name of the application using the library.
     *  \param sCreator creator
     */
    void SetCreator(const PdfString& sCreator);

    /** Get the creator of the document
     *  \returns the creator
     */
    std::optional<PdfString> GetCreator() const;

    /** Set keywords for this document
     *  \param sKeywords a list of keywords
     */
    void SetKeywords(const PdfString& sKeywords);

    /** Get the keywords of the document
     *  \returns the keywords
     */
    std::optional<PdfString> GetKeywords() const;

    /** Set the subject of the document.
     *  \param sSubject subject
     */
    void SetSubject(const PdfString& sSubject);

    /** Get the subject of the document
     *  \returns the subject
     */
    std::optional<PdfString> GetSubject() const;

    /** Set the title of the document.
     *  \param sTitle title
     */
    void SetTitle(const PdfString& sTitle);

    /** Get the title of the document
     *  \returns the title
     */
    std::optional<PdfString> GetTitle() const;

    // Peter Petrov 27 April 2008
    /** Set the producer of the document.
     *  \param sProducer producer
     */
    void SetProducer(const PdfString& sProducer);

    // Peter Petrov 27 April 2008
    /** Get the producer of the document
     *  \returns the producer
     */
    std::optional<PdfString> GetProducer() const;

    /** Set the trapping state of the document.
     *  \param sTrapped trapped
     */
    void SetTrapped(const PdfName& sTrapped);

    /** Get the trapping state of the document
     *  \returns the title
     */
    const PdfName& GetTrapped() const;

    /** Get creation date of document
     *  \return creation date
     */
    PdfDate GetCreationDate() const;

    /** Get modification date of document
     *  \return modification date
     */
    PdfDate GetModDate() const;

    /** Set custom info key.
     * \param sName Name of the key.
     * \param sValue Value of the key.
     */
    void SetCustomKey(const PdfName& sName, const PdfString& sValue);
private:
    /** Add the initial document information to the dictionary.
     *  \param eInitial which information should be
     *         writting initially to the information
     */
    void Init(PdfInfoInitial eInitial);

    /** Get a value from the info dictionary as name
     *  \para rName the key to fetch from the info dictionary
     *  \return a value from the info dictionary
     */
    std::optional<PdfString> GetStringFromInfoDict(const PdfName& rName) const;

    /** Get a value from the info dictionary as name
    *  \para rName the key to fetch from the info dictionary
    *  \return a value from the info dictionary
    */
    const PdfName& GetNameFromInfoDict(const PdfName& rName) const;

};

};

#endif // PDF_INFO_H
