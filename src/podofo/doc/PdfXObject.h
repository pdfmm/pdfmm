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

#ifndef _PDF_XOBJECT_H_
#define _PDF_XOBJECT_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfArray.h"
#include "podofo/base/PdfCanvas.h"
#include "podofo/base/PdfRect.h"

#include "PdfElement.h"

namespace PoDoFo {

class PdfDictionary;
class PdfObject;

enum class EPdfXObject
{
    Form,
    Image,
    PostScript,
    Unknown = 0xff
};

/** A XObject is a content stream with several drawing commands and data
 *  which can be used throughout a PDF document.
 *
 *  You can draw on a XObject like you would draw onto a page and can draw
 *  this XObject later again using a PdfPainter.
 * 
 *  \see PdfPainter
 */
class PODOFO_DOC_API PdfXObject : public PdfElement, public PdfCanvas {
 public:
    /** Create a new XObject with a specified dimension
     *  in a given document
     * 
     *  \param rRect the size of the XObject
     *  \param pParent the parent document of the XObject
	 *  \param pszPrefix optional prefix for XObject-name
     *  \param bWithoutObjNum do not create an object identifier name
     */
    PdfXObject( const PdfRect & rRect, PdfDocument* pParent, const char* pszPrefix = NULL, bool bWithoutObjNum = false);

    /** Create a new XObject with a specified dimension
     *  in a given vector of PdfObjects
     * 
     *  \param rRect the size of the XObject
     *  \param pParent the parent vector of the XObject
	 *  \param pszPrefix optional prefix for XObject-name
     */
    PdfXObject( const PdfRect & rRect, PdfVecObjects* pParent, const char* pszPrefix = NULL );
    
    /** Create a new XObject from a page of another document
     *  in a given document
     * 
     *  \param rSourceDoc the document to create the XObject from
     *  \param nPage the page-number in rDoc to create the XObject from
     *  \param pParent the parent document of the XObject
	 *  \param pszPrefix optional prefix for XObject-name
 	 *	\param bUseTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObject( const PdfDocument & rSourceDoc, int nPage, PdfDocument* pParent, const char* pszPrefix = NULL, bool bUseTrimBox = false );

    /** Create a new XObject from an existing page
     * 
     *  \param pDoc the document to create the XObject at
     *  \param nPage the page-number in pDoc to create the XObject from
     *  \param pszPrefix optional prefix for XObject-name
     *  \param bUseTrimBox if true try to use trimbox for size of xobject
     */
    PdfXObject( PdfDocument *pDoc, int nPage, const char* pszPrefix = NULL, bool bUseTrimBox = false );

    /** Create a XObject from an existing PdfObject
     *  
     *  \param pObject an existing object which has to be
     *                 a XObject
     */
    PdfXObject( PdfObject* pObject );

protected:
    PdfXObject(EPdfXObject subType, PdfDocument* pParent, const char* pszPrefix = NULL);
    PdfXObject(EPdfXObject subType, PdfVecObjects* pParent, const char* pszPrefix = NULL);
    PdfXObject(EPdfXObject subType, PdfObject* pObject);

public:
    static bool TryCreateFromObject(PdfObject &obj, std::unique_ptr<PdfXObject> &xobj, EPdfXObject &type);

    static std::string ToString(EPdfXObject type);
    static EPdfXObject FromString(const std::string &str);

    /** Get the rectangle of this xobject
    *  \returns a rectangle
    */
    PdfRect GetRect() const override;

    /** Get the current canvas rotation
     * \param teta ignored
     * \returns always return false
     */
    bool HasRotation(double& teta) const override;

    /** Set the rectangle of this xobject
    *  \param rect a rectangle
    */
    void SetRect( const PdfRect &rect );

    /** Ensure resources initialized on this XObject
    */
    void EnsureResourcesInitialized();

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    PdfObject* GetResources() const override;

    /** Get the identifier used for drawig this object
     *  \returns identifier
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    /** Get the reference to the XObject in the PDF file
     *  without having to access the PdfObject.
     *
     *  This allows to work with XObjects which have been 
     *  written to disk already.
     *
     *  \returns the reference of the PdfObject for this XObject
     */
    inline const PdfReference& GetObjectReference() const { return m_Reference; }

    inline EPdfXObject GetType() const { return m_type; }

 private:
    static EPdfXObject getPdfXObjectType(const PdfObject &obj);
    void InitXObject( const PdfRect & rRect, const char* pszPrefix );
    void InitIdentifiers(EPdfXObject subType, const char* pszPrefix = NULL);
    void InitAfterPageInsertion(const PdfDocument & rDoc, int page);
    void InitResources();
    PdfObject* GetContents() const override;
    PdfStream & GetStreamForAppending(EPdfStreamAppendFlags flags) override;

 protected:
    PdfRect          m_rRect;

private:
    EPdfXObject      m_type;
    PdfArray         m_matrix;
    PdfObject*       m_pResources;
    PdfName          m_Identifier;
    PdfReference     m_Reference;
};

};

#endif /* _PDF_XOBJECT_H_ */


