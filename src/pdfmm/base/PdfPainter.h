/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_PAINTER_H
#define PDF_PAINTER_H

#include "PdfDefines.h"

#include <sstream>

#include "PdfRect.h"
#include "PdfColor.h"
#include "PdfCanvas.h"
#include "PdfTextState.h"

#include <pdfmm/extra/PdfShadingPattern.h>
#include <pdfmm/extra/PdfTilingPattern.h>

namespace mm {

class PdfExtGState;
class PdfFont;
class PdfImage;
class PdfMemDocument;
class PdfName;
class PdfObject;
class PdfReference;
class PdfStream;
class PdfXObject;

enum class PdfPainterFlags
{
    None = 0,
    Prepend = 1,            ///< Does nothing for now
    NoSaveRestorePrior = 2, ///< Do not perform a Save/Restore or previous content. Implies RawCoordinates
    NoSaveRestore = 4,      ///< Do not perform a Save/Restore of added content in this painting session
    RawCoordinates = 8,     ///< Does nothing for now
};

/**
 * This class provides an easy to use painter object which allows you to draw on a PDF page
 * object.
 *
 * During all drawing operations, you are still able to access the stream of the object you are
 * drawing on directly.
 *
 * All functions that take coordinates expect these to be in PDF User Units. Keep in mind that PDF has
 * its coordinate system origin at the bottom left corner.
 */
class PDFMM_API PdfPainter final
{
public:
    /** Create a new PdfPainter object.
     *
     *  \param saveRestore do save/restore state before appending
     */
    PdfPainter(PdfPainterFlags flags = PdfPainterFlags::None);

    virtual ~PdfPainter() noexcept(false);

    /** Set the page on which the painter should draw.
     *  The painter will draw of course on the pages
     *  contents object.
     *
     *  Calls FinishPage() on the last page if it was not yet called.
     *
     *  \param page a PdfCanvas object (most likely a PdfPage or PdfXObject).
     *
     *  \see PdfPage \see PdfXObject
     *  \see FinishPage()
     */
    void SetCanvas(PdfCanvas* page);

    /** Finish drawing onto a canvas.
     *
     *  This has to be called whenever a page has been drawn complete.
     */
    void FinishDrawing();

    /** Set the shading pattern for all following stroking operations.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param rPattern a shading pattern
     */
    void SetStrokingShadingPattern(const PdfShadingPattern& pattern);

    /** Set the shading pattern for all following non-stroking operations.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param rPattern a shading pattern
     */
    void SetShadingPattern(const PdfShadingPattern& pattern);

    /** Set the tiling pattern for all following stroking operations.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param rPattern a tiling pattern
     */
    void SetStrokingTilingPattern(const PdfTilingPattern& pattern);

    /** Set the tiling pattern for all following stroking operations by pattern name,
     *  Use when it's already in resources.
     *  This operation uses the 'SCN' PDF operator.
     *
     *  \param rPatternName a tiling pattern name
     */
    void SetStrokingTilingPattern(const std::string_view& patternName);

    /** Set the tiling pattern for all following non-stroking operations.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param rPattern a tiling pattern
     */
    void SetTilingPattern(const PdfTilingPattern& pattern);

    /** Set the tiling pattern for all following non-stroking operations by pattern name.
      *  Use when it's already in resources.
     *  This operation uses the 'scn' PDF operator.
     *
     *  \param rPattern a tiling pattern
     */
    void SetTilingPattern(const std::string_view& patternName);

    /** Set the color for all following stroking operations.
     *
     *  \param color a PdfColor object
     */
    void SetStrokingColor(const PdfColor& color);

    /** Set the color for all following non-stroking operations.
     *
     *  \param color a PdfColor object
     */
    void SetColor(const PdfColor& color);

    /** Set the line width for all stroking operations.
     *  \param width in PDF User Units.
     */
    void SetStrokeWidth(double width);

    /** Set the stoke style for all stroking operations.
     *  \param strokeStyle style of the stroking operations
     *  \param custom a custom stroking style which is used when
     *                   strokeStyle == PdfStrokeStyle::Custom.
      *  \param inverted inverted dash style (gaps for drawn spaces),
      *                  it is ignored for None, Solid and Custom styles
      *  \param scale scale factor of the stroke style
      *                  it is ignored for None, Solid and Custom styles
      *  \param subtractJoinCap if true, subtracts scaled width on filled parts,
      *                       thus the line capability still draws into the cell;
      *                        is used only if scale is not 1.0
     *
     *  Possible values:
     *    PdfStrokeStyle::None
     *    PdfStrokeStyle::Solid
     *    PdfStrokeStyle::Dash
     *    PdfStrokeStyle::Dot
     *    PdfStrokeStyle::DashDot
     *    PdfStrokeStyle::DashDotDot
     *    PdfStrokeStyle::Custom
     *
     */
    void SetStrokeStyle(PdfStrokeStyle strokeStyle, const std::string_view& custom = { }, bool inverted = false, double scale = 1.0, bool subtractJoinCap = false);

    /** Set the line cap style for all stroking operations.
     *  \param eCapStyle the cap style.
     *
     *  Possible values:
     *    PdfLineCapStyle::Butt,
     *    PdfLineCapStyle::Round,
     *    PdfLineCapStyle::Square
     */
    void SetLineCapStyle(PdfLineCapStyle capStyle);

    /** Set the line join style for all stroking operations.
     *  \param eJoinStyle the join style.
     *
     *  Possible values:
     *    PdfLineJoinStyle::Miter
     *    PdfLineJoinStyle::Round
     *    PdfLineJoinStyle::Bevel
     */
    void SetLineJoinStyle(PdfLineJoinStyle joinStyle);

    /** Set the font for all text drawing operations
     *  \param font a handle to a valid PdfFont object
     *
     *  \see DrawText
     */
    void SetFont(PdfFont* font);

    /** Set the text rendering mode
     *  \param mode What text rendering mode to use.
     *
     *  Possible values:
     *    PdfTextRenderingMode::Fill (default mode)
     *    PdfTextRenderingMode::Stroke
     *    PdfTextRenderingMode::FillAndStroke
     *    PdfTextRenderingMode::Invisible
     *    PdfTextRenderingMode::FillToClipPath
     *    PdfTextRenderingMode::StrokeToClipPath
     *    PdfTextRenderingMode::FillAndStrokeToClipPath
     *    PdfTextRenderingMode::ToClipPath
     */
    void SetTextRenderingMode(PdfTextRenderingMode mode);

    /** Set a clipping rectangle
     *
     *  \param x x coordinate of the rectangle (left coordinate)
     *  \param y y coordinate of the rectangle (bottom coordinate)
     *  \param width width of the rectangle
     *  \param height absolute height of the rectangle
     */
    void SetClipRect(double x, double y, double width, double height);

    /** Set a clipping rectangle
     *
     *  \param rect rectangle
     */
    void SetClipRect(const PdfRect& rect);

    /** Set miter limit.
     */
    void SetMiterLimit(double value);

    /** Draw a line with the current color and line settings.
     *  \param startX x coordinate of the starting point
     *  \param startY y coordinate of the starting point
     *  \param endX x coordinate of the ending point
     *  \param endY y coordinate of the ending point
     */
    void DrawLine(double startX, double startY, double endX, double endY);

    /** Add a rectangle into the current path
     *  \param x x coordinate of the rectangle (left coordinate)
     *  \param y y coordinate of the rectangle (bottom coordinate)
     *  \param width width of the rectangle
     *  \param height absolute height of the rectangle
     *  \param roundX rounding factor, x direction
     *  \param roundY rounding factor, y direction
     */
    void Rectangle(double x, double y, double width, double height,
        double roundX = 0.0, double roundY = 0.0);

    /** Add a rectangle into the current path
     *
     *  \param rect the rectangle area
     *  \param roundX rounding factor, x direction
     *  \param roundY rounding factor, y direction
     *
     *  \see DrawRect
     */
    void Rectangle(const PdfRect& rect, double roundX = 0.0, double roundY = 0.0);

    /** Add an ellipse into the current path
     *  \param x x coordinate of the ellipse (left coordinate)
     *  \param y y coordinate of the ellipse (top coordinate)
     *  \param width width of the ellipse
     *  \param height absolute height of the ellipse
     */
    void Ellipse(double x, double y, double width, double height);

    /** Add a circle into the current path
     *  \param x x center coordinate of the circle
     *  \param y y coordinate of the circle
     *  \param dRadius radius of the circle
     */
    void Circle(double x, double y, double radius);

    /** Draw a single-line text string on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *  \param x the x coordinate
     *  \param y the y coordinate
     *  \param sText the text string which should be printed
     *
     *  \see SetFont()
     */
    void DrawText(double x, double y, const std::string_view& str);

    /** Draw multiline text into a rectangle doing automatic wordwrapping.
     *  The current font is used and SetFont has to be called at least once
     *  before using this function
     *
     *  \param x the x coordinate of the text area (left)
     *  \param y the y coordinate of the text area (bottom)
     *  \param width width of the text area
     *  \param height height of the text area
     *  \param text the text which should be drawn
     *  \param eAlignment alignment of the individual text lines in the given bounding box
     *  \param eVertical vertical alignment of the text in the given bounding box
     *  \param bClip set the clipping rectangle to the given rect, otherwise no clipping is performed
     *  \param bSkipSpaces whether the trailing whitespaces should be skipped, so that next line doesn't start with whitespace
     */
    void DrawMultiLineText(double x, double y, double width, double height,
        const std::string_view& str, PdfHorizontalAlignment hAlignment = PdfHorizontalAlignment::Left,
        PdfVerticalAlignment vAlignment = PdfVerticalAlignment::Top, bool clip = true, bool skipSpaces = true);

    /** Draw multiline text into a rectangle doing automatic wordwrapping.
     *  The current font is used and SetFont has to be called at least once
     *  before using this function
     *
     *  \param rect bounding rectangle of the text
     *  \param text the text which should be drawn
     *  \param eAlignment alignment of the individual text lines in the given bounding box
     *  \param eVertical vertical alignment of the text in the given bounding box
     *  \param bClip set the clipping rectangle to the given rect, otherwise no clipping is performed
     *  \param bSkipSpaces whether the trailing whitespaces should be skipped, so that next line doesn't start with whitespace
     */
    void DrawMultiLineText(const PdfRect& rect, const std::string_view& str, PdfHorizontalAlignment hAlignment = PdfHorizontalAlignment::Left,
        PdfVerticalAlignment vAlignment = PdfVerticalAlignment::Top, bool clip = true, bool skipSpaces = true);

    /** Draw a single line of text horizontally aligned.
     *  \param x the x coordinate of the text line
     *  \param y the y coordinate of the text line
     *  \param width the width of the text line
     *  \param text the text to draw
     *  \param eAlignment alignment of the text line
     */
    void DrawTextAligned(double x, double y, double width, const std::string_view& str, PdfHorizontalAlignment hAlignment);

    /** Begin drawing multiple text strings on a page using a given font object.
     *  You have to call SetFont before calling this function.
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \param x the x coordinate
     *  \param y the y coordinate
     *
     *  \see SetFont()
     *  \see AddText()
     *  \see MoveTextPos()
     *  \see EndText()
     */
    void BeginText(double x, double y);

    /** Draw a string on a page.
     *  You have to call BeginText before the first call of this function
     *  and EndText after the last call.
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \param sText the text string which should be printed
     *
     *  \see SetFont()
     *  \see MoveTextPos()
     *  \see EndText()
     */
    void AddText(const std::string_view& str);

    /** Move position for text drawing on a page.
     *  You have to call BeginText before calling this function
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \param x the x offset relative to pos of BeginText or last MoveTextPos
     *  \param y the y offset relative to pos of BeginText or last MoveTextPos
     *
     *  \see BeginText()
     *  \see AddText()
     *  \see EndText()
     */
    void MoveTextPos(double x, double y);

    /** End drawing multiple text strings on a page
     *
     *  If you want more simpler text output and do not need
     *  the advanced text position features of MoveTextPos
     *  use DrawText which is easier.
     *
     *  \see BeginText()
     *  \see AddText()
     *  \see MoveTextPos()
     */
    void EndText();

    /** Draw an image on the current page.
     *  \param x the x coordinate (bottom left position of the image)
     *  \param y the y coordinate (bottom position of the image)
     *  \param obj an PdfXObject
     *  \param scaleX option scaling factor in x direction
     *  \param scaleY option scaling factor in y direction
     */
    void DrawImage(double x, double y, const PdfImage& obj, double scaleX = 1.0, double scaleY = 1.0);

    /** Draw an XObject on the current page. For PdfImage use DrawImage.
     *
     *  \param x the x coordinate (bottom left position of the XObject)
     *  \param y the y coordinate (bottom position of the XObject)
     *  \param obj an PdfXObject
     *  \param scaleX option scaling factor in x direction
     *  \param scaleY option scaling factor in y direction
     *
     *  \see DrawImage
     */
    void DrawXObject(double x, double y, const PdfXObject& obj, double scaleX = 1.0, double scaleY = 1.0);

    /** Closes the current path by drawing a line from the current point
     *  to the starting point of the path. Matches the PDF 'h' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void ClosePath();

    /** Append a line segment to the current path. Matches the PDF 'l' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param x x position
     *  \param y y position
     */
    void LineTo(double x, double y);

    /** Begin a new path. Matches the PDF 'm' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *  \param x x position
     *  \param y y position
     */
    void MoveTo(double x, double y);

    /** Append a cubic bezier curve to the current path
     *  Matches the PDF 'c' operator.
     *
     *  \param x1 x coordinate of the first control point
     *  \param y1 y coordinate of the first control point
     *  \param x2 x coordinate of the second control point
     *  \param y2 y coordinate of the second control point
     *  \param x3 x coordinate of the end point, which is the new current point
     *  \param y3 y coordinate of the end point, which is the new current point
     */
    void CubicBezierTo(double x1, double y1, double x2, double y2, double x3, double y3);

    /** Append a horizontal line to the current path
     *  Matches the SVG 'H' operator
     *
     *  \param x x coordinate to draw the line to
     */
    void HorizontalLineTo(double x);

    /** Append a vertical line to the current path
     *  Matches the SVG 'V' operator
     *
     *  \param y y coordinate to draw the line to
     */
    void VerticalLineTo(double y);

    /** Append a smooth bezier curve to the current path
     *  Matches the SVG 'S' operator.
     *
     *  \param x2 x coordinate of the second control point
     *  \param y2 y coordinate of the second control point
     *  \param x3 x coordinate of the end point, which is the new current point
     *  \param y3 y coordinate of the end point, which is the new current point
     */
    void SmoothCurveTo(double x2, double y2, double x3, double y3);

    /** Append a quadratic bezier curve to the current path
     *  Matches the SVG 'Q' operator.
     *
     *  \param x1 x coordinate of the first control point
     *  \param y1 y coordinate of the first control point
     *  \param x3 x coordinate of the end point, which is the new current point
     *  \param y3 y coordinate of the end point, which is the new current point
     */
    void QuadCurveTo(double x1, double y1, double x3, double y3);

    /** Append a smooth quadratic bezier curve to the current path
     *  Matches the SVG 'T' operator.
     *
     *  \param x3 x coordinate of the end point, which is the new current point
     *  \param y3 y coordinate of the end point, which is the new current point
     */
    void SmoothQuadCurveTo(double x3, double y3);

    /** Append a Arc to the current path
     *  Matches the SVG 'A' operator.
     *
     *  \param x x coordinate of the start point
     *  \param y y coordinate of the start point
     *  \param dRadiusX x coordinate of the end point, which is the new current point
     *  \param dRadiusY y coordinate of the end point, which is the new current point
     *	\param dRotation degree of rotation in radians
     *	\param bLarge large or small portion of the arc
     *	\param bSweep sweep?
     */
    void ArcTo(double x, double y, double radiusX, double radiusY,
        double rotation, bool large, bool sweep);

    // Peter Petrov 5 January 2009 was delivered from libHaru
    /**
    */
    bool Arc(double x, double y, double radius, double angle1, double angle2);

    /** Close the current path. Matches the PDF 'h' operator.
     */
    void Close();

    /** Stroke the current path. Matches the PDF 'S' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     */
    void Stroke();

    /** Fill the current path. Matches the PDF 'f' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
      *
     *  \param useEvenOddRule select even-odd rule instead of nonzero winding number rule
     */
    void Fill(bool useEvenOddRule = false);

    /** Fill then stroke the current path. Matches the PDF 'B' operator.
     *
    *  \param useEvenOddRule select even-odd rule instead of nonzero winding number rule
    */
    void FillAndStroke(bool useEvenOddRule = false);

    /** Clip the current path. Matches the PDF 'W' operator.
     *  This function is useful to construct an own path
     *  for drawing or clipping.
     *
     *  \param useEvenOddRule select even-odd rule instead of nonzero winding number rule
     */
    void Clip(bool useEvenOddRule = false);

    /** End current pathm without filling or stroking it.
     *  Matches the PDF 'n' operator.
     */
    void EndPath();

    /** Save the current graphics settings onto the graphics
     *  stack. Operator 'q' in PDF.
     *  This call has to be balanced with a corresponding call
     *  to Restore()!
     *
     *  \see Restore
     */
    void Save();

    /** Restore the current graphics settings from the graphics
     *  stack. Operator 'Q' in PDF.
     *  This call has to be balanced with a corresponding call
     *  to Save()!
     *
     *  \see Save
     */
    void Restore();

    /** Set the transformation matrix for the current coordinate system
     *  See the operator 'cm' in PDF.
     *
     *  The six parameters are a standard 3x3 transformation matrix
     *  where the 3 left parameters are 0 0 1.
     *
     *  \param a scale in x direction
     *  \param b rotation
     *  \param c rotation
     *  \param d scale in y direction
     *  \param e translate in x direction
     *  \param f translate in y direction
     *
     *  \see Save()
     *  \see Restore()
     */
    void SetTransformationMatrix(double a, double b, double c, double d, double e, double f);

    /** Sets a specific PdfExtGState as being active
     *	\param inGState the specific ExtGState to set
     */
    void SetExtGState(const PdfExtGState& inGState);

    /** Sets a specific rendering intent
     *	\param intent the specific intent to set
     */
    void SetRenderingIntent(const std::string_view& intent);

    /** Set the floating point precision.
     *
     *  \param precision write this many decimal places
     */
    void SetPrecision(unsigned short precision);

    /** Get the currently set floating point precision
     *  \returns how many decimal places will be written out for any floating point value
     */
    unsigned short GetPrecision() const;

    /** Set rgb color that depend on color space setting, "cs" tag.
     *
     * \param color a PdfColor object
     * \param csTag a CS tag used in PdfPage::SetICCProfile
     *
     * \see PdfPage::SetICCProfile()
     */
    void SetDependICCProfileColor(const PdfColor& color, const std::string_view& csTag);

public:
    inline const PdfTextState& GetTextState() const { return m_textState; }
    inline PdfTextState& GetTextState() { return m_textState; }

    /** Gets current text rendering mode.
     *  Default mode is PdfTextRenderingMode::Fill.
     */
    inline PdfTextRenderingMode GetTextRenderingMode() const { return m_currentTextRenderingMode; }

    /** Get the current font:
     *  \returns a font object or nullptr if no font was set.
     */
    inline PdfFont* GetFont() const { return m_Font; }

    /** Set the tab width for the DrawText operation.
     *  Every tab '\\t' is replaced with tabWidth
     *  spaces before drawing text. Default is a value of 4
     *
     *  \param tabWidth replace every tabulator by this much spaces
     *
     *  \see DrawText
     *  \see TabWidth
     */
    inline void SetTabWidth(unsigned short tabWidth) { m_TabWidth = tabWidth; }

    /** Get the currently set tab width
     *  \returns by how many spaces a tabulator will be replaced
     *
     *  \see DrawText
     *  \see TabWidth
     */
    inline unsigned short GetTabWidth() const { return m_TabWidth; }

    /** Return the current page that is that on the painter.
     *
     *  \returns the current page of the painter or nullptr if none is set
     */
    inline PdfCanvas* GetCanvas() const { return m_canvas; }

    /** Return the current canvas stream that is set on the painter.
     *
     *  \returns the current page canvas stream of the painter or nullptr if none is set
     */
    inline PdfStream* GetStream() const { return m_stream; }

    /** Get current path string stream.
     * Stroke/Fill commands clear current path.
     * \returns std::ostringstream representing current path
     */
    inline std::ostringstream& GetCurrentPath() { return m_curPath; }

    /** Get current temporary stream
     */
    inline std::ostringstream& GetStream() { return m_tmpStream; }

private:
    /** Gets the text divided into individual lines, using the current font and clipping rectangle.
     *
     *  \param width width of the text area
     *  \param text the text which should be drawn
     *  \param bSkipSpaces whether the trailing whitespaces should be skipped, so that next line doesn't start with whitespace
     */
    std::vector<std::string> GetMultiLineTextAsLines(double width, const std::string_view& str, bool skipSpaces);

    /** Coverts a rectangle to an array of points which can be used
     *  to draw an ellipse using 4 bezier curves.
     *
     *  The arrays plPointX and plPointY need space for at least 12 longs
     *  to be stored.
     *
     *  \param x x position of the bounding rectangle
     *  \param y y position of the bounding rectangle
     *  \param width width of the bounding rectangle
     *  \param height height of the bounding rectangle
     *  \param pointsX pointer to an array were the x coordinates
     *                  of the resulting points will be stored
     *  \param pointsY pointer to an array were the y coordinates
     *                  of the resulting points will be stored
     */
    void ConvertRectToBezier(double x, double y, double width, double height, double pointsX[], double pointsY[]);

    /** Register an object in the resource dictionary of this page
     *  so that it can be used for any following drawing operations.
     *
     *  \param identifier identifier of this object, e.g. /Ft0
     *  \param ref reference to the object you want to register
     *  \param name register under this key in the resource dictionary
     */
    void AddToPageResources(const PdfName& identifier, const PdfReference& ref, const PdfName& name);

    /** Sets the color that was last set by the user as the current stroking color.
      *  You should always enclose this function by Save() and Restore()
      *
      *  \see Save() \see Restore()
      */
    void SetCurrentStrokingColor();

    void InternalArc(double x, double y, double ray, double ang1, double ang2, bool contFlg);

    /** Expand all tab characters in a string
     *  using spaces.
     *
     *  \param str expand all tabs in this string using spaces
     *  \param len use only len characters of the string. If negative use all
     *      string length
     *  \returns an expanded copy of the passed string
     *  \see SetTabWidth
     */
    std::string ExpandTabs(const std::string_view& str) const;
    void CheckStream();
    void finishDrawing();
    void SetCurrentTextRenderingMode();

private:
    PdfPainterFlags m_flags;

    /** All drawing operations work on this stream.
     *  This object may not be nullptr. If it is nullptr any function accessing it should
     *  return ERROR_PDF_INVALID_HANDLE
     */
    PdfStream* m_stream;

    /** The page object is needed so that fonts etc. can be added
     *  to the page resource dictionary as appropriate.
     */
    PdfCanvas* m_canvas;

    PdfTextState m_textState;

    /** Font for all drawing operations
     */
    PdfFont* m_Font;

    /** Every tab '\\t' is replaced with m_TabWidth
     *  spaces before drawing text. Default is a value of 4
     */
    unsigned short m_TabWidth;

    /** Save the current color for non stroking colors
     */
    PdfColor m_curColor;

    /** Is between BT and ET
     */
    bool m_isTextOpen;

    /** temporary stream buffer
     */
    std::ostringstream  m_tmpStream;

    /** current path
     */
    std::ostringstream  m_curPath;

    /** True if should use color with ICC Profile
     */
    bool m_isCurColorICCDepend;
    /** ColorSpace tag
     */
    std::string m_CSTag;

    PdfTextRenderingMode m_currentTextRenderingMode;

    // TODO: Next comment was found like this and it's is really bad.
    // Document the next fields accurately, possibly moving them
    // to a structure

    // points for this operation
    // last "current" point
    // "reflect points"
    double m_lpx;
    double m_lpy;
    double m_lpx2;
    double m_lpy2;
    double m_lpx3;
    double m_lpy3;
    double m_lcx;
    double m_lcy;
    double m_lrx;
    double m_lry;
};

}

ENABLE_BITMASK_OPERATORS(mm::PdfPainterFlags);

#endif // PDF_PAINTER_H
