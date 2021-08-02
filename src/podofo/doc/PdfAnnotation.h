/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ANNOTATION_H
#define PDF_ANNOTATION_H

#include "podofo/base/PdfDefines.h"
#include "PdfAction.h"
#include "PdfDestination.h"
#include "PdfElement.h"

namespace PoDoFo {

class PdfFileSpec;
class PdfName;
class PdfPage;
class PdfRect;
class PdfReference;
class PdfString;
class PdfXObject;

/** The type of the annotation.
 *  PDF supports different annotation types, each of 
 *  them has different keys and propeties.
 *  
 *  Not all annotation types listed here are supported yet.
 *
 *  Please make also sure that the annotation type you use is
 *  supported by the PDF version you are using.
 */
enum class PdfAnnotationType
{
    Unknown = 0,
    Text,                       // - supported
    Link,                       // - supported
    FreeText,       // PDF 1.3  // - supported
    Line,           // PDF 1.3  // - supported
    Square,         // PDF 1.3
    Circle,         // PDF 1.3
    Polygon,        // PDF 1.5
    PolyLine,       // PDF 1.5
    Highlight,      // PDF 1.3
    Underline,      // PDF 1.3
    Squiggly,       // PDF 1.4
    StrikeOut,      // PDF 1.3
    Stamp,          // PDF 1.3
    Caret,          // PDF 1.5
    Ink,            // PDF 1.3
    Popup,          // PDF 1.3  // - supported
    FileAttachement,// PDF 1.3
    Sound,          // PDF 1.2
    Movie,          // PDF 1.2
    Widget,         // PDF 1.2  // - supported
    Screen,         // PDF 1.5
    PrinterMark,    // PDF 1.4
    TrapNet,        // PDF 1.3
    Watermark,      // PDF 1.6
    Model3D,        // PDF 1.6
    RichMedia,      // PDF 1.7 ADBE ExtensionLevel 3 ALX: Petr P. Petrov
    WebMedia,       // PDF 1.7 IPDF ExtensionLevel 3
};

/** Flags that control the appearance of a PdfAnnotation.
 *  You can OR them together and pass it to 
 *  PdfAnnotation::SetFlags.
 */
enum class PdfAnnotationFlags
{
    None         = 0x0000,
    Invisible    = 0x0001,
    Hidden       = 0x0002,
    Print        = 0x0004,
    NoZoom       = 0x0008,
    NoRotate     = 0x0010,
    NoView       = 0x0020,
    ReadOnly     = 0x0040,
    Locked       = 0x0080,
    ToggleNoView = 0x0100,
    LockedContents = 0x0200,
};

/**
 * Type of the annotation appearance.
 */
enum class PdfAnnotationAppearance
{
    Normal = 0, ///< Normal appearance
    Rollover,   ///< Rollover appearance; the default is PdfAnnotationAppearance::Normal
    Down        ///< Down appearance; the default is PdfAnnotationAppearance::Normal
};

/** An annotation to a PdfPage 
 *  To create an annotation use PdfPage::CreateAnnotation
 * 
 *  \see PdfPage::CreateAnnotation
 */
class PODOFO_DOC_API PdfAnnotation : public PdfElement
{
public:
    /** Create a new annotation object
     *
     *  \param pPage the parent page of this annotation
     *  \param eAnnot type of the annotation
     *  \param rRect the rectangle in which the annotation will appear on the page
     *  \param pParent parent of this annotation
     *
     *  \see PdfPage::CreateAnnotation
     */
    PdfAnnotation(PdfPage& pPage, PdfAnnotationType eAnnot, const PdfRect& rRect);

    /** Create a PdfAnnotation from an existing object
     *
     *  \param pObject the annotations object
     *  \param pPage the page of the annotation
     */
    PdfAnnotation(PdfPage& pPage, PdfObject& pObject);

    ~PdfAnnotation();

    /** Set an appearance stream for this object
     *  to specify its visual appearance
     *  \param pObject an XObject
     *  \param eApperance an apperance type to set
     *  \param state the state for which set it the pObject; states depend on the annotation type
     */
    void SetAppearanceStream(PdfXObject& pObject, PdfAnnotationAppearance eAppearance = PdfAnnotationAppearance::Normal, const PdfName& state = "");

    /**
     * \returns true if this annotation has an appearance stream
     */
    bool HasAppearanceStream() const;

    /**
    * \returns the appearance object /AP for this object
    */
    PdfObject* GetAppearanceDictionary();

    /**
    * \returns the appearance stream for this object
     *  \param eApperance an apperance type to get
     *  \param state a child state. Meaning depends on the annotation type
    */
    PdfObject* GetAppearanceStream(PdfAnnotationAppearance eAppearance = PdfAnnotationAppearance::Normal, const PdfName& state = "");

    /** Get the rectangle of this annotation.
     *  \returns a rectangle
     */
    PdfRect GetRect() const;

    /** Set the rectangle of this annotation.
     * \param rRect rectangle to set
     */
    void SetRect(const PdfRect& rRect);

    /** Set the flags of this annotation.
     *  \param uiFlags is an unsigned 32bit integer with different
     *                 PdfAnnotationFlags OR'ed together.
     *  \see GetFlags
     */
    void SetFlags(PdfAnnotationFlags uiFlags);

    /** Get the flags of this annotation.
     *  \returns the flags which is an unsigned 32bit integer with different
     *           PdfAnnotationFlags OR'ed together.
     *
     *  \see SetFlags
     */
    PdfAnnotationFlags GetFlags() const;

    /** Set the annotations border style.
     *  \param dHCorner horitzontal corner radius
     *  \param dVCorner vertical corner radius
     *  \param dWidth width of border
     */
    void SetBorderStyle(double dHCorner, double dVCorner, double dWidth);

    /** Set the annotations border style.
     *  \param dHCorner horitzontal corner radius
     *  \param dVCorner vertical corner radius
     *  \param dWidth width of border
     *  \param rStrokeStyle a custom stroke style pattern
     */
    void SetBorderStyle(double dHCorner, double dVCorner, double dWidth, const PdfArray& rStrokeStyle);

    /** Set the title of this annotation.
     *  \param sTitle title of the annoation as string in PDF format
     *
     *  \see GetTitle
     */
    void SetTitle(const PdfString& sTitle);

    /** Get the title of this annotation
     *
     *  \returns the title of this annotation
     *
     *  \see SetTitle
     */
    std::optional<PdfString> GetTitle() const;

    /** Set the text of this annotation.
     *
     *  \param sContents text of the annoation as string in PDF format
     *
     *  \see GetContents
     */
    void SetContents(const PdfString& sContents);

    /** Get the text of this annotation
     *
     *  \returns the contents of this annotation
     *
     *  \see SetContents
     */
    std::optional<PdfString> GetContents() const;

    /** Set the destination for link annotations
     *  \param rDestination target of the link
     *
     *  \see GetDestination
     */
    void SetDestination(const std::shared_ptr<PdfDestination>& destination);

    /** Get the destination of a link annotations
     * 
     *  \returns a destination object
     *  \see SetDestination
     */
    std::shared_ptr<PdfDestination> GetDestination() const;

    /**
     *  \returns true if this annotation has an destination
     */
    bool HasDestination() const;

    /** Set the action that is executed for this annotation
     *  \param rAction an action object
     *
     *  \see GetAction
     */
    void SetAction(const std::shared_ptr<PdfAction>& action);

    /** Get the action that is executed for this annotation
     *  \returns an action object. The action object is owned
     *           by the PdfAnnotation.
     *
     *  \see SetAction
     */
    std::shared_ptr<PdfAction> GetAction() const;

    /**
     *  \returns true if this annotation has an action
     */
    bool HasAction() const;

    /** Sets whether this annotation is initialy open.
     *  You should always set this true for popup annotations.
     *  \param b if true open it
     */
    void SetOpen(bool b);

    /**
     * \returns true if this annotation should be opened immediately
     *          by the viewer
     */
    bool GetOpen() const;

    /**
     * \returns true if this annotation has a file attachement
     */
    bool HasFileAttachement() const;

    /** Set a file attachment for this annotation.
     *  The type of this annotation has to be
     *  EPdfAnnotation::FileAttachement for file
     *  attachements to work.
     *
     *  \param rFileSpec a file specification
     */
    void SetFileAttachement(const std::shared_ptr<PdfFileSpec>& fileSpec);

    /** Get a file attachement of this annotation.
     *  \returns a file specification object. The file specification object is owned
     *           by the PdfAnnotation.
     *
     *  \see SetFileAttachement
     */
    std::shared_ptr<PdfFileSpec> GetFileAttachement() const;


    /** Get the quad points associated with the annotation (if appropriate).
     *  This array is used in text markup annotations to describe the
     *  regions affected by the markup (i.e. the hilighted words, one
     *  quadrilateral per word)
     *
     *  \returns a PdfArray of 8xn numbers describing the
     *           x,y coordinates of BL BR TR TL corners of the
     *           quadrilaterals. If inappropriate, returns
     *           an empty array.
     */
    PdfArray GetQuadPoints() const;

    /** Set the quad points associated with the annotation (if appropriate).
     *  This array is used in text markup annotations to describe the
     *  regions affected by the markup (i.e. the hilighted words, one
     *  quadrilateral per word)
     *
     *  \param rQuadPoints a PdfArray of 8xn numbers describing the
     *           x,y coordinates of BL BR TR TL corners of the
     *           quadrilaterals.
     */
    void SetQuadPoints(const PdfArray& rQuadPoints);

    /** Get the color key of the Annotation dictionary
     *  which defines the color of the annotation,
     *  as per 8.4 of the pdf spec. The PdfArray contains
     *  0 to four numbers, depending on the colorspace in
     *  which the color is specified
     *  0 numbers means the annotation is transparent
     *  1 number specifies the intensity of the color in grayscale
     *  3 numbers specifie the color in the RGB colorspace and
     *  4 numbers specify the color in the CMYK colorspace
     *
     *  \returns a PdfArray of either 0, 1, 3 or 4 numbers
     *           depending on the colorspace in which the color
     *           is specified
     */

    PdfArray GetColor() const;

    /** Set the C key of the Annotation dictionary, which defines the
     *  color of the annotation, as per 8.4 of the pdf spec. Parameters
     *  give the color in rgb colorspace coordinates
     *
     *  \param r number from 0 to 1, the intensity of the red channel
     *  \param g number from 0 to 1, the intensity of the green channel
     *  \param b number from 0 to 1, the intensity of the blue channel
     */

    void SetColor(double r, double g, double b);

    /** Set the C key of the Annotation dictionary, which defines the
     *  color of the annotation, as per 8.4 of the pdf spec. Parameters
     *  give the color in cmyk colorspace coordinates
     *
     *  \param c number from 0 to 1, the intensity of the cyan channel
     *  \param m number from 0 to 1, the intensity of the magneta channel
     *  \param y number from 0 to 1, the intensity of the yellow channel
     *  \param k number from 0 to 1, the intensity of the black channel
     */

    void SetColor(double c, double m, double y, double k);

    /** Set the C key of the Annotation dictionary, which defines the
     *  color of the annotation, as per 8.4 of the pdf spec. Parameters
     *  give the color in grayscale colorspace coordinates
     *
     *  \param gray  number from 0 to 1, the intensity of the black
     */

    void SetColor(double gray);

    /** Set the C key of the Annotation dictionary to an empty array, which,
     *  as per 8.4 of the pdf spec., makes the annotation transparent
     *
     */

    void SetColor();

public:
    /** Get the type of this annotation
     *  \returns the annotation type
     */
    inline PdfAnnotationType GetType() const { return m_eAnnotation; }

    /** Get the page of this PdfField
     *
     *  \returns the page of this PdfField
     */
    inline PdfPage* GetPage() const { return m_pPage; }

private:
    std::shared_ptr<PdfDestination> getDestination();
    std::shared_ptr<PdfAction> getAction();
    std::shared_ptr<PdfFileSpec> getFileAttachment();

private:
    PdfAnnotationType m_eAnnotation;
    std::shared_ptr<PdfDestination> m_Destination;
    std::shared_ptr<PdfAction> m_Action;
    std::shared_ptr<PdfFileSpec> m_pFileSpec;
    PdfPage* m_pPage;
};

// helper function, to avoid code duplication
void SetAppearanceStreamForObject(PdfObject& pForObject, PdfXObject& pObject, PdfAnnotationAppearance eAppearance, const PdfName& state);

};

ENABLE_BITMASK_OPERATORS(PoDoFo::PdfAnnotationFlags);

#endif // PDF_ANNOTATION_H
