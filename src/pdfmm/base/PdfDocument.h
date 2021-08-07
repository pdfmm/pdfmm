/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DOCUMENT_H
#define PDF_DOCUMENT_H

#include "PdfDefines.h"

#include "PdfObject.h"
#include "PdfVecObjects.h"
#include "PdfAcroForm.h"
#include "PdfFontCache.h"
#include "PdfInfo.h"
#include "PdfPagesTree.h"
#include "PdfNamesTree.h"

namespace mm {

class PdfDestination;
class PdfDictionary;
class PdfFileSpec;
class PdfFont;
class PdfInfo;
class PdfOutlines;
class PdfPage;
class PdfRect;
class PdfXObject;

/** PdfDocument is the core interface for working with PDF documents.
 *
 *  PdfDocument provides easy access to the individual pages
 *  in the PDF file and to certain special dictionaries.
 *
 *  PdfDocument cannot be used directly.
 *  Use PdfMemDocument whenever you want to change the object structure
 *  of a PDF file. 
 *
 *  When you are only creating PDF files, please use PdfStreamedDocument
 *  which is usually faster for creating PDFs.
 *
 *  \see PdfStreamedDocument
 *  \see PdfMemDocument
 */
class PDFMM_API PdfDocument
{
    friend class PdfElement;

public:
    /** Close down/destruct the PdfDocument
     */
    virtual ~PdfDocument();

    /** Get the write mode used for writing the PDF
     *  \returns the write mode
     */
    virtual PdfWriteMode GetWriteMode() const = 0;

    /** Get the PDF version of the document
     *  \returns EPdfVersion version of the pdf document
     */
    virtual PdfVersion GetPdfVersion() const = 0;

    /** Returns whether this PDF document is linearized, aka
     *  web-optimized
     *  \returns true if the PDF document is linearized
     */
    virtual bool IsLinearized() const = 0;

    /** Get access to the Outlines (Bookmarks) dictionary
     *  The returned outlines object is owned by the PdfDocument.
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \returns the Outlines/Bookmarks dictionary
     */
    PdfOutlines* GetOutlines(bool create = true);

    /** Get access to the Names dictionary (where all the named objects are stored)
     *  The returned PdfNamesTree object is owned by the PdfDocument.
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \returns the Names dictionary
     */
    PdfNamesTree* GetNamesTree(bool create = true);

    /** Get access to the AcroForm dictionary
     *
     *  \param create create the object if it does not exist (ePdfCreateObject)
     *                 or return nullptr if it does not exist
     *  \param eDefaultAppearance specifies if a default appearence shall be created
     *
     *  \returns PdfObject the AcroForm dictionary
     */
    PdfAcroForm* GetAcroForm(bool create = true,
        EPdfAcroFormDefaulAppearance eDefaultAppearance = EPdfAcroFormDefaulAppearance::BlackText12pt);

    /** Embeds all pending subset fonts, is automatically done on Write().
     *  Just call explicitly in case the PdfDocument is needed as PdfXObject.
     *
     */
    void EmbedSubsetFonts();

    /** Appends another PdfDocument to this document.
     *  \param doc the document to append
     *  \param appendAll specifies whether pages and outlines are appended too
     *  \returns this document
     */
    const PdfDocument& Append(const PdfDocument& doc, bool appendAll = true);

    /** Inserts existing page from another PdfDocument to this document.
     *  \param doc the document to append from
     *  \param pageIndex index of page to append from doc
     *  \param atIndex index at which to add the page in this document
     *  \returns this document
     */
    const PdfDocument& InsertExistingPageAt(const PdfDocument& doc, unsigned pageIndex, unsigned atIndex);

    /** Fill an existing empty PdfXObject from a page of another document.
     *  This will append the other document to this one.
     *  \param xobj pointer to the PdfXObject
     *  \param doc the document to embed into the PdfXObject
     *  \param pageIndex index of page to embed into the PdfXObject
     *  \param useTrimBox if true try to use page's TrimBox for size of PdfXObject
     *  \returns the bounding box
     */
    PdfRect FillXObjectFromDocumentPage(PdfXObject& xobj, const PdfDocument& doc, unsigned pageIndex, bool useTrimBox);

    /** Fill an existing empty PdfXObject from an existing page from the current document.
     *  If you need a page from another document use FillXObjectFromDocumentPage(), or
     *  append the document manually.
     *  \param xobj pointer to the PdfXObject
     *  \param pageIndex index of page to embed into the PdfXObject
     *  \param useTrimBox if true try to use page's TrimBox for size of PdfXObject
     *  \returns the bounding box
     */
    PdfRect FillXObjectFromExistingPage(PdfXObject& xobj, unsigned pageIndex, bool useTrimBox);

    /** Fill an existing empty PdfXObject from an existing page pointer from the current document.
     *  This is the implementation for FillXObjectFromDocumentPage() and FillXObjectFromExistingPage(),
     *  you should use those directly instead of this.
     *  \param xobj pointer to the PdfXObject
     *  \param page pointer to the page to embed into PdfXObject
     *  \param useTrimBox if true try to use page's TrimBox for size of PdfXObject
     *  \returns the bounding box
     */
    PdfRect FillXObjectFromPage(PdfXObject& xobj, const PdfPage& page, bool useTrimBox, unsigned difference);

    /** Attach a file to the document.
     *  \param rFileSpec a file specification
     */
    void AttachFile(const PdfFileSpec& fileSpec);

    /** Get an attached file's filespec.
     *  \param name the name of the attachment
     *  \return the file specification object if the file exists, nullptr otherwise
     *          The file specification object is not owned by the document and must be deleted by the caller
     */
    PdfFileSpec* GetAttachment(const PdfString& name);

    /** Adds a PdfDestination into the global Names tree
     *  with the specified name, optionally replacing one of the same name.
     *  \param rDest the destination to be assigned
     *  \param name the name for the destination
     */
    void AddNamedDestination(const PdfDestination& rDest, const PdfString& name);

    /** Sets the opening mode for a document.
     *  \param inMode which mode to set
     */
    void SetPageMode(PdfPageMode inMode);

    /** Gets the opening mode for a document.
     *  \returns which mode is set
     */
    PdfPageMode GetPageMode() const;

    /** Sets the opening mode for a document to be in full screen.
     */
    void SetUseFullScreen();

    /** Sets the page layout for a document.
     */
    void SetPageLayout(PdfPageLayout inLayout);

    /** Set the document's Viewer Preferences:
     *  Hide the toolbar in the viewer.
     */
    void SetHideToolbar();

    /** Set the document's Viewer Preferences:
     *  Hide the menubar in the viewer.
     */
    void SetHideMenubar();

    /** Set the document's Viewer Preferences:
     *  Show only the documents contents and no control
     *  elements such as buttons and scrollbars in the viewer.
     */
    void SetHideWindowUI();

    /** Set the document's Viewer Preferences:
     *  Fit the document in the viewer's window.
     */
    void SetFitWindow();

    /** Set the document's Viewer Preferences:
     *  Center the document in the viewer's window.
     */
    void SetCenterWindow();

    /** Set the document's Viewer Preferences:
     *  Display the title from the document information
     *  in the title of the viewer.
     *
     *  \see SetTitle
     */
    void SetDisplayDocTitle();

    /** Set the document's Viewer Preferences:
     *  Set the default print scaling of the document.
     *
     *  TODO: DS use an enum here!
     */
    void SetPrintScaling(const PdfName& scalingType);

    /** Set the document's Viewer Preferences:
     *  Set the base URI of the document.
     *
     *  TODO: DS document value!
     */
    void SetBaseURI(const std::string_view& baseURI);

    /** Set the document's Viewer Preferences:
     *  Set the language of the document.
     */
    void SetLanguage(const std::string_view& language);

    /** Set the document's Viewer Preferences:
        Set the document's binding direction.
     */
    void SetBindingDirection(const PdfName& direction);

    /** Checks if printing this document is allowed.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to print this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsPrintAllowed() const = 0;

    /** Checks if modifying this document (besides annotations, form fields or substituting pages) is allowed.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to modify this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsEditAllowed() const = 0;

    /** Checks if text and graphics extraction is allowed.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to extract text and graphics from this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsCopyAllowed() const = 0;

    /** Checks if it is allowed to add or modify annotations or form fields.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to add or modify annotations or form fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsEditNotesAllowed() const = 0;

    /** Checks if it is allowed to fill in existing form or signature fields.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to fill in existing form or signature fields
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsFillAndSignAllowed() const = 0;

    /** Checks if it is allowed to extract text and graphics to support users with disabilities.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to extract text and graphics to support users with disabilities
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsAccessibilityAllowed() const = 0;

    /** Checks if it is allowed to insert, create, rotate, or delete pages or add bookmarks.
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed  to insert, create, rotate, or delete pages or add bookmarks
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsDocAssemblyAllowed() const = 0;

    /** Checks if it is allowed to print a high quality version of this document
     *  Every PDF-consuming application has to adhere to this value!
     *
     *  \returns true if you are allowed to print a high quality version of this document
     *
     *  \see PdfEncrypt to set own document permissions.
     */
    virtual bool IsHighPrintAllowed() const = 0;

public:
    /** Get access to the internal Catalog dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    PdfObject& GetCatalog();

    /** Get access to the internal Catalog dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    const PdfObject& GetCatalog() const;

    /** Get access to the pages tree.
     *  \returns the PdfPagesTree of this document.
     */
    PdfPagesTree& GetPageTree();

    /** Get access to the pages tree.
     *  \returns the PdfPagesTree of this document.
     */
    const PdfPagesTree& GetPageTree() const;

    /** Get access to the internal trailer dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    PdfObject& GetTrailer();

    /** Get access to the internal trailer dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    const PdfObject& GetTrailer() const;

    /** Get access to the internal Info dictionary
     *  You can set the author, title etc. of the
     *  document using the info dictionary.
     *
     *  \returns the info dictionary
     */
    PdfInfo& GetInfo();

    /** Get access to the internal Info dictionary
     *  You can set the author, title etc. of the
     *  document using the info dictionary.
     *
     *  \returns the info dictionary
     */
    const PdfInfo& GetInfo() const;

    /** Get access to the internal vector of objects
     *  or root object.
     *
     *  \returns the vector of objects
     */
    inline PdfVecObjects& GetObjects() { return m_Objects; }

    /** Get access to the internal vector of objects
     *  or root object.
     *
     *  \returns the vector of objects
     */
    inline const PdfVecObjects& GetObjects() const { return m_Objects; }

    inline PdfFontCache& GetFontCache() { return m_FontCache; }

protected:
    /** Construct a new (empty) PdfDocument
     *  \param bEmpty if true NO default objects (such as catalog) are created.
     */
    PdfDocument(bool bEmpty = false);

    /** Set the info object containing meta information.
     *  Deletes any old info object.
     *
     *  \param info the new info object (will be owned by PdfDocument)
     */
    void SetInfo(std::unique_ptr<PdfInfo>& info);

    /** Set the trailer of this PdfDocument
     *  deleting the old one.
     *
     *  \param obj the new trailer object
     *         It will be owned by PdfDocument.
     */
    void SetTrailer(std::unique_ptr<PdfObject>& obj);

    /** Set the catalog of this PdfDocument
     *  deleting the old one.
     *
     *  \param obj the new catalog object
     *         It will be owned by PdfDocument.
     */
     // m_Catalog does not need to 
     // be reowned
    inline void SetCatalog(PdfObject* obj) { m_Catalog = obj; }

    /** Get a dictionary from the catalog dictionary by its name.
     *  \param name will be converted into a PdfName
     *  \returns the dictionary if it was found or nullptr
     */
    PdfObject* GetNamedObjectFromCatalog(const std::string_view& name) const;

    /** Internal method for initializing the pages tree for this document
     */
    void InitPagesTree();

    /** Recursively changes every PdfReference in the PdfObject and in any child
     *  that is either an PdfArray or a direct object.
     *  The reference is changed so that difference is added to the object number
     *  of the reference.
     *  \param obj object to change
     *  \param difference add this value to every reference that is encountered
     */
    void FixObjectReferences(PdfObject& obj, int difference);

    /** Low-level APIs for setting a viewer preference.
     *  \param whichPref the dictionary key to set
     *  \param valueObj the object to be set
     */
    void SetViewerPreference(const PdfName& whichPref, const PdfObject& valueObj);

    /** Low-level APIs for setting a viewer preference.
     *  Convenience overload.
     *  \param whichPref the dictionary key to set
     *  \param inValue the object to be set
     */
    void SetViewerPreference(const PdfName& whichPref, bool inValue);

    /** Clear all internal variables
     *  and reset PdfDocument to an intial state.
     */
    void Clear();

    /** Get access to the internal trailer dictionary
     *  or root object.
     *
     *  \returns PdfObject the documents catalog
     */
    inline PdfObject* getCatalog() { return m_Catalog; }

private:
    // Prevent use of copy constructor and assignment operator.  These methods
    // should never be referenced (given that code referencing them outside
    // PdfDocument won't compile), and calling them will result in a link error
    // as they're not defined.
    explicit PdfDocument(const PdfDocument&);
    PdfDocument& operator=(const PdfDocument&) = delete;

private:
    PdfVecObjects m_Objects;
    std::unique_ptr<PdfObject> m_Trailer;
    PdfObject* m_Catalog;
    std::unique_ptr<PdfInfo> m_Info;
    std::unique_ptr<PdfPagesTree> m_PageTree;
    std::unique_ptr<PdfAcroForm> m_AcroForms;
    std::unique_ptr<PdfOutlines> m_Outlines;
    std::unique_ptr<PdfNamesTree> m_NameTree;
    PdfFontCache m_FontCache;
};

};


#endif	// PDF_DOCUMENT_H
