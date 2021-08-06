/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ELEMENT_H
#define PDF_ELEMENT_H

#include "PdfDefines.h"

#include "PdfObject.h"

namespace PoDoFo {

class PdfStreamedDocument;
class PdfVecObjects;

/** PdfElement is a common base class for all elements
 *  in a PDF file. For example pages, action and annotations.
 *
 *  Every PDF element has one PdfObject and provides an easier
 *  interface to modify the contents of the dictionary. 
 *  
 *  A PdfElement base class can be created from an existing PdfObject
 *  or created from scratch. In the later case, the PdfElement creates
 *  a PdfObject and adds it to a vector of objects.
 *
 *  A PdfElement cannot be created directly. Use one
 *  of the subclasses which implement real functionallity.
 *
 *  \see PdfPage \see PdfAction \see PdfAnnotation
 */
class PODOFO_DOC_API PdfElement
{
public:

    virtual ~PdfElement();

    /** Get access to the internal object
     *  \returns the internal PdfObject
     */
    inline PdfObject& GetObject() { return *m_Object; }

    /** Get access to the internal object
     *  This is an overloaded member function.
     *
     *  \returns the internal PdfObject
     */
    inline const PdfObject& GetObject() const { return *m_Object; }

    PdfDocument& GetDocument();

    const PdfDocument& GetDocument() const;

protected:
    /** Creates a new PdfElement
     *  \param type type entry of the elements object
     *  \param parent parent PdfDocument.
     *                 Add a newly created object to this vector.
     */
    PdfElement(PdfDocument& parent, const std::string_view& type = { });

    /** Create a PdfElement from an existing PdfObject
     *  The object must be a dictionary.
     *
     *  \param obj pointer to the PdfObject that is modified
     *                 by this PdfElement
     */
    PdfElement(PdfObject& obj);

    /** Create a PdfElement from an existing PdfObject
     *  The object might be of any data type,
     *  PdfElement will throw an exception if the PdfObject
     *  if not of the same datatype as the expected one.
     *  This is necessary in rare cases. E.g. in PdfContents.
     *
     *  \param expectedDataType the expected datatype of this object
     *  \param obj pointer to the PdfObject that is modified
     *                 by this PdfElement
     */
    PdfElement(EPdfDataType expectedDataType, PdfObject& obj);

    PdfElement(const PdfElement& element);

    /** Convert an enum or index to its string representation
     *  which can be written to the PDF file.
     *
     *  This is a helper function for various PdfElement
     *  subclasses that need strings and enums for their
     *  SubTypes keys.
     *
     *  \param index the index or enum value
     *  \param types an array of strings containing
     *         the string mapping of the index
     *  \param len the length of the string array
     *
     *  \returns the string representation or nullptr for
     *           values out of range
     */
    const char* TypeNameForIndex(unsigned index, const char** types, unsigned len) const;

    /** Convert a string type to an array index or enum.
     *
     *  This is a helper function for various PdfElement
     *  subclasses that need strings and enums for their
     *  SubTypes keys.
     *
     *  \param type the type as string
     *  \param types an array of strings containing
     *         the string mapping of the index
     *  \param len the length of the string array
     *  \param unknownValue the value that is returned when the type is unknown
     *
     *  \returns the index of the string in the array
     */
    int TypeNameToIndex(const char* type, const char** types, unsigned len, int unknownValue) const;

    /** Create a PdfObject in the parent of this PdfElement which
     *  might either be a PdfStreamedDocument, a PdfDocument or
     *  a PdfVecObjects
     *
     *  Use this function in an own subclass of PdfElement to create new
     *  PdfObjects.
     *
     *  \param type an optional /Type key of the created object
     *
     *  \returns a PdfObject which is owned by the parent
     */
    PdfObject* CreateObject(const std::string_view& type = { });

private:
    PdfObject* m_Object;
};

};

#endif // PDF_ELEMENT_H
