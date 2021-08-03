/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_RECT_H
#define PDF_RECT_H

#include "PdfDefines.h"

namespace PoDoFo {

class PdfArray;
class PdfPage;
class PdfVariant;

/** A rectangle as defined by the PDF reference
 */
class PODOFO_API PdfRect final
{
public:
    /** Create an empty rectangle with bottom=left=with=height=0
     */
    PdfRect();

    /** Create a rectangle with a given size and position
     *  All values are in PDF units
     *	NOTE: since PDF is bottom-left origined, we pass the bottom instead of the top
     */
    PdfRect(double left, double bottom, double width, double height);

    /** Create a rectangle from an array
     *  All values are in PDF units
     */
    PdfRect(const PdfArray& inArray);

    /** Copy constructor
     */
    PdfRect(const PdfRect& rhs);

public:
    /** Create a PdfRect from a couple of arbitrary points
     * \returns the created PdfRect
     */
    static PdfRect FromCorners(double x1, double y1, double x2, double y2);

    /** Converts the rectangle into an array
     *  based on PDF units and adds the array into an variant.
     *  \param var the variant to store the Rect
     */
    void ToVariant(PdfVariant& var) const;

    /** Returns a string representation of the PdfRect
     * \returns std::string representation as [ left bottom right top ]
     */
    std::string ToString() const;

    /** Assigns the values of this PdfRect from the 4 values in the array
     *  \param inArray the array to load the values from
     */
    void FromArray(const PdfArray& inArray);

    /** Intersect with another rect
     *  \param rRect the rect to intersect with
     */
    void Intersect(const PdfRect& rRect);

    /** Get the right coordinate of the rectangle
     *  \returns bottom
     */
    double GetRight() const;

    /** Get the top coordinate of the rectangle
     *  \returns bottom
     */
    double GetTop() const;

    /** Get the bottom coordinate of the rectangle
     *  \returns bottom
     */
    inline double GetBottom() const { return m_dBottom; }

    /** Set the bottom coordinate of the rectangle
     *  \param dBottom
     */
    inline void SetBottom(double dBottom) { m_dBottom = dBottom; }

    /** Get the left coordinate of the rectangle
     *  \returns left in PDF units
     */
    inline double GetLeft() const { return m_dLeft; }

    /** Set the left coordinate of the rectangle
     *  \param lLeft in PDF units
     */
    inline void SetLeft(double dLeft) { m_dLeft = dLeft; }

    /** Get the width of the rectangle
     *  \returns width in PDF units
     */
    inline double GetWidth() const { return m_dWidth; }

    /** Set the width of the rectangle
     *  \param lWidth in PDF units
     */
    inline void SetWidth(double dWidth) { m_dWidth = dWidth; }

    /** Get the height of the rectangle
     *  \returns height in PDF units
     */
    inline double GetHeight() const { return m_dHeight; }

    /** Set the height of the rectangle
     *  \param lHeight in PDF units
     */
    inline void SetHeight(double dHeight) { m_dHeight = dHeight; }

    PdfRect& operator=(const PdfRect& rhs);

private:
    double m_dLeft;
    double m_dBottom;
    double m_dWidth;
    double m_dHeight;
};

};

#endif // PDF_RECT_H
