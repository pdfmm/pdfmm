/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_CANVAS_H
#define PDF_CANVAS_H

#include "PdfDefines.h"
#include "PdfArray.h"

namespace PoDoFo {

class PdfDictionary;
class PdfObject;
class PdfRect;
class PdfColor;

enum class EPdfStreamAppendFlags
{
    None = 0,
    Prepend = 1,
    NoSaveRestorePrior = 2
};

/** A interface that provides the necessary features 
 *  for a painter to draw onto a PdfObject.
 */
class PODOFO_API PdfCanvas
{
public:
    /** Virtual destructor
     *  to avoid compiler warnings
     */
    virtual ~PdfCanvas() {};

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfObject& GetContents() = 0;

    /** Get access an object that you can use to ADD drawing to.
     *  If you want to draw onto the page, you have to add
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    virtual PdfStream& GetStreamForAppending(EPdfStreamAppendFlags flags) = 0;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    virtual PdfObject& GetResources() = 0;

    /** Get the current canvas size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    virtual PdfRect GetRect() const = 0;

    /** Get the current canvas rotation
     * \param teta counterclockwise rotation in radians
     * \returns true if the canvas has a rotation
     */
    virtual bool HasRotation(double& teta) const = 0;

    /** Get a copy of procset PdfArray.
     *  \returns a procset PdfArray
     */
    static PdfArray GetProcSet();

    /** Register a colourspace for a (separation) colour in the resource dictionary
     *  of this page or XObject so that it can be used for any following drawing 
     *  operations.
     *
     *  \param rColor reference to the PdfColor
     */
    void AddColorResource(const PdfColor& rColor);

    /** Register an object in the resource dictionary of this page or XObbject
     *  so that it can be used for any following drawing operations.
     *
     *  \param rIdentifier identifier of this object, e.g. /Ft0
     *  \param rRef reference to the object you want to register
     *  \param rName register under this key in the resource dictionary
     */
    void AddResource(const PdfName& rIdentifier, const PdfReference& rRef, const PdfName& rName);
};

};

ENABLE_BITMASK_OPERATORS(PoDoFo::EPdfStreamAppendFlags);

#endif // PDF_CANVAS_H
