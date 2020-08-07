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

#ifndef _PDF_RECT_H_
#define _PDF_RECT_H_

#include "PdfDefines.h"


namespace PoDoFo {

class PdfArray;
class PdfPage;
class PdfVariant;
   
/** A rectangle as defined by the PDF reference
 */
class PODOFO_API PdfRect {
 public:
    /** Create an empty rectangle with bottom=left=with=height=0
     */
    PdfRect();

    /** Create a rectangle with a given size and position
     *  All values are in PDF units
     *	NOTE: since PDF is bottom-left origined, we pass the bottom instead of the top
     */
    PdfRect( double left, double bottom, double width, double height );
    
    /** Create a rectangle from an array
     *  All values are in PDF units
     */
    PdfRect( const PdfArray& inArray );
    
    /** Copy constructor 
     */
    PdfRect( const PdfRect & rhs );

public:
    /** Create a PdfRect from a couple of arbitrary points
     * \returns the created PdfRect
     */
    static PdfRect FromCorners(double x1, double y1, double x2, double y2);
    
    /** Converts the rectangle into an array
     *  based on PDF units and adds the array into an variant.
     *  \param var the variant to store the Rect
     */
    void ToVariant( PdfVariant & var ) const;

    /** Returns a string representation of the PdfRect
     * \returns std::string representation as [ left bottom right top ]
     */
    std::string ToString() const;

    /** Assigns the values of this PdfRect from the 4 values in the array
     *  \param inArray the array to load the values from
     */
    void FromArray( const PdfArray& inArray );

    /** Intersect with another rect
     *  \param rRect the rect to intersect with
     */
    void Intersect( const PdfRect & rRect );

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

    PdfRect & operator=( const PdfRect & rhs );

 private:
    double m_dLeft;
    double m_dBottom;
    double m_dWidth;
    double m_dHeight;
};

};

#endif /* _PDF_RECT_H_ */
