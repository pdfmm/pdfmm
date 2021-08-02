/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_SHADING_PATTERN_H
#define PDF_SHADING_PATTERN_H

#include "podofo/base/PdfDefines.h"
#include "podofo/base/PdfName.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfColor;
class PdfObject;
class PdfPage;
class PdfWriter;

enum class EPdfShadingPatternType
{
    FunctionBase  = 1,    
    Axial         = 2,
    Radial        = 3,
    FreeForm      = 4,
    LatticeForm   = 5,
    CoonsPatch    = 6,
    TensorProduct = 7
};

/** 
 * This class defined a shading pattern which can be used
 * to fill abitrary shapes with a pattern using PdfPainter.
 */
class PODOFO_DOC_API PdfShadingPattern : public PdfElement
{
public:
    /** Returns the identifier of this ShadingPattern how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Sh13)
     */
    inline const PdfName& GetIdentifier() const;

protected:
    /** Create a new PdfShadingPattern object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  \param doc parent document
     *  \param eShadingType the type of this shading pattern
     *
     */
    PdfShadingPattern(PdfDocument& doc, EPdfShadingPatternType eShadingType);

private:
    /** Initialize the object
     *
     *  \param eShadingType the type of this shading pattern
     */
    void Init(EPdfShadingPatternType eShadingType);

private:
    PdfName m_Identifier;
};

const PdfName & PdfShadingPattern::GetIdentifier() const
{
    return m_Identifier;
}

/** A shading pattern that is a simple axial
 *  shading between two colors.
 */
class PODOFO_DOC_API PdfAxialShadingPattern final : public PdfShadingPattern
{
public:
    /** Create an axial shading pattern
     *
     *  \param doc the parent
     *  \param dX0 the starting x coordinate
     *  \param dY0 the starting y coordinate
     *  \param dX1 the ending x coordinate
     *  \param dY1 the ending y coordinate
     *  \param rStart the starting color
     *  \param rEnd the ending color
     */
    PdfAxialShadingPattern(PdfDocument& doc, double dX0, double dY0, double dX1, double dY1, const PdfColor& rStart, const PdfColor& rEnd);

private:

    /** Initialize an axial shading pattern
     *
     *  \param dX0 the starting x coordinate
     *  \param dY0 the starting y coordinate
     *  \param dX1 the ending x coordinate
     *  \param dY1 the ending y coordinate
     *  \param rStart the starting color
     *  \param rEnd the ending color
     */
    void Init(double dX0, double dY0, double dX1, double dY1, const PdfColor& rStart, const PdfColor& rEnd);
};

/** A shading pattern that is an 2D
 *  shading between four colors.
 */
class PODOFO_DOC_API PdfFunctionBaseShadingPattern final : public PdfShadingPattern
{
public:
    /** Create an 2D shading pattern
     *
     *  \param doc the parent
     *  \param rLL the color on lower left corner
     *  \param rUL the color on upper left corner
     *  \param rLR the color on lower right corner
     *  \param rUR the color on upper right corner
     *  \param rMatrix the transformation matrix mapping the coordinate space
     *         specified by the Domain entry into the shading's target coordinate space
     */
    PdfFunctionBaseShadingPattern(PdfDocument& doc, const PdfColor& rLL, const PdfColor& rUL, const PdfColor& rLR, const PdfColor& rUR, const PdfArray& rMatrix);

private:

    /** Initialize an 2D shading pattern
     *
     *  \param rLL the color on lower left corner
     *  \param rUL the color on upper left corner
     *  \param rLR the color on lower right corner
     *  \param rUR the color on upper right corner
     *  \param rMatrix the transformation matrix mapping the coordinate space
     *         specified by the Domain entry into the shading's target coordinate space
     */
    void Init(const PdfColor& rLL, const PdfColor& rUL, const PdfColor& rLR, const PdfColor& rUR, const PdfArray& rMatrix);
};

/** A shading pattern that is a simple radial
 *  shading between two colors.
 */
class PODOFO_DOC_API PdfRadialShadingPattern final : public PdfShadingPattern
{
public:
    /** Create an radial shading pattern
     *
     *  \param doc the parent
     *  \param dX0 the inner circles x coordinate
     *  \param dY0 the inner circles y coordinate
     *  \param dR0 the inner circles radius
     *  \param dX1 the outer circles x coordinate
     *  \param dY1 the outer circles y coordinate
     *  \param dR1 the outer circles radius
     *  \param rStart the starting color
     *  \param rEnd the ending color
     */
    PdfRadialShadingPattern(PdfDocument& doc, double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor& rStart, const PdfColor& rEnd);

private:

    /** Initialize an radial shading pattern
     *
     *  \param dX0 the inner circles x coordinate
     *  \param dY0 the inner circles y coordinate
     *  \param dR0 the inner circles radius
     *  \param dX1 the outer circles x coordinate
     *  \param dY1 the outer circles y coordinate
     *  \param dR1 the outer circles radius
     *  \param rStart the starting color
     *  \param rEnd the ending color
     */
    void Init(double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor& rStart, const PdfColor& rEnd);
};

/** A shading pattern that is a simple triangle
 *  shading between three colors. It's a single-triangle
 *  simplified variation of a FreeForm shadding pattern.
 */
class PODOFO_DOC_API PdfTriangleShadingPattern final : public PdfShadingPattern
{
public:
    /** Create a triangle shading pattern
     *
     *  \param dX0 triangle x coordinate of point 0
     *  \param dY0 triangle y coordinate of point 0
      *  \param color0 color of point 0
     *  \param dX1 triangle x coordinate of point 1
     *  \param dY1 triangle y coordinate of point 1
      *  \param color1 color of point 1
     *  \param dX2 triangle x coordinate of point 2
     *  \param dY2 triangle y coordinate of point 2
      *  \param color2 color of point 2
     *  \param pParent the parent
     */
    PdfTriangleShadingPattern(PdfDocument& doc, double dX0, double dY0, const PdfColor& color0, double dX1, double dY1, const PdfColor& color1, double dX2, double dY2, const PdfColor& color2);

private:

    /** Initialize a triangle shading pattern
     *
     *  \param dX0 triangle x coordinate of point 0
     *  \param dY0 triangle y coordinate of point 0
      *  \param color0 color of point 0
     *  \param dX1 triangle x coordinate of point 1
     *  \param dY1 triangle y coordinate of point 1
      *  \param color1 color of point 1
     *  \param dX2 triangle x coordinate of point 2
     *  \param dY2 triangle y coordinate of point 2
      *  \param color2 color of point 2
     */
    void Init(double dX0, double dY0, const PdfColor& color0, double dX1, double dY1, const PdfColor& color1, double dX2, double dY2, const PdfColor& color2);
};

};

#endif // PDF_SHADING_PATTERN_H

