/**
 * SPDX-FileCopyrightText: (C) 2006 Dominik Seichter <domseichter@web.de>
 * SPDX-FileCopyrightText: (C) 2020 Francesco Pretto <ceztko@gmail.com>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PDF_ANNOTATION_H
#define PDF_ANNOTATION_H

#include "PdfElement.h"
#include "PdfRect.h"

namespace mm {

class PdfPage;
class PdfXObjectForm;

struct PdfAppearanceIdentity final
{
    const PdfObject* Object;
    PdfAppearanceType Type;
    PdfName State;
};

/** An annotation to a PdfPage
 *  To create an annotation use PdfPage::CreateAnnotation
 *
 *  \see PdfPage::CreateAnnotation
 */
class PDFMM_API PdfAnnotation : public PdfDictionaryElement
{
    friend class PdfAnnotationCollection;

protected:
    PdfAnnotation(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect);
    PdfAnnotation(PdfObject& obj, PdfAnnotationType annotType);

public:
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<PdfAnnotation>& xobj);

    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const PdfAnnotation>& xobj);

    template <typename TAnnotation>
    static bool TryCreateFromObject(PdfObject& obj, std::unique_ptr<TAnnotation>& xobj);

    template <typename TAnnotation>
    static bool TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const TAnnotation>& xobj);

public:
    /** Set an appearance stream for this object
     *  to specify its visual appearance
     *  \param obj an XObject
     *  \param appearance an apperance type to set
     *  \param state the state for which set it the obj; states depend on the annotation type
     */
    void SetAppearanceStream(PdfXObjectForm& obj, PdfAppearanceType appearance = PdfAppearanceType::Normal, const PdfName& state = "");

    void GetAppearanceStreams(std::vector<PdfAppearanceIdentity>& streams) const;

    /**
    * \returns the appearance /AP object for this annotation
    */
    PdfObject* GetAppearanceDictionaryObject();
    const PdfObject* GetAppearanceDictionaryObject() const;

    /**
    * \returns the appearance stream for this object
     *  \param appearance an apperance type to get
     *  \param state a child state. Meaning depends on the annotation type
    */
    PdfObject* GetAppearanceStream(PdfAppearanceType appearance = PdfAppearanceType::Normal, const PdfName& state = "");
    const PdfObject* GetAppearanceStream(PdfAppearanceType appearance = PdfAppearanceType::Normal, const PdfName& state = "") const;

    /** Get the rectangle of this annotation.
     *  \returns a rectangle
     */
    PdfRect GetRect() const;

    /** Set the rectangle of this annotation.
     * \param rect rectangle to set
     */
    void SetRect(const PdfRect& rect);

    /** Set the flags of this annotation.
     *  \see GetFlags
     */
    void SetFlags(PdfAnnotationFlags flags);

    /** Get the flags of this annotation.
     *  \returns the flags which is an unsigned 32bit integer with different
     *           PdfAnnotationFlags OR'ed together.
     *
     *  \see SetFlags
     */
    PdfAnnotationFlags GetFlags() const;

    /** Set the annotations border style.
     *  \param hCorner horitzontal corner radius
     *  \param vCorner vertical corner radius
     *  \param width width of border
     */
    void SetBorderStyle(double hCorner, double vCorner, double width);

    /** Set the annotations border style.
     *  \param hCorner horitzontal corner radius
     *  \param dVCorner vertical corner radius
     *  \param width width of border
     *  \param strokeStyle a custom stroke style pattern
     */
    void SetBorderStyle(double hCorner, double vCorner, double width, const PdfArray& strokeStyle);

    /** Set the title of this annotation.
     *  \param title title of the annoation as string in PDF format
     *
     *  \see GetTitle
     */
    void SetTitle(const PdfString& title);

    /** Get the title of this annotation
     *
     *  \returns the title of this annotation
     *
     *  \see SetTitle
     */
    nullable<PdfString> GetTitle() const;

    /** Set the text of this annotation.
     *
     *  \param contents text of the annoation as string in PDF format
     *
     *  \see GetContents
     */
    void SetContents(const PdfString& contents);

    /** Get the text of this annotation
     *
     *  \returns the contents of this annotation
     *
     *  \see SetContents
     */
    nullable<PdfString> GetContents() const;

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
    void ResetColor();

public:
    /** Get the type of this annotation
     *  \returns the annotation type
     */
    inline PdfAnnotationType GetType() const { return m_AnnotationType; }

    /** Get the page of this PdfField
     *
     *  \returns the page of this PdfField
     */
    inline PdfPage* GetPage() { return m_Page; }
    inline const PdfPage* GetPage() const { return m_Page; }
    PdfPage& MustGetPage();
    const PdfPage& MustGetPage() const;

private:
    static std::unique_ptr<PdfAnnotation> Create(PdfPage& page, PdfAnnotationType annotType, const PdfRect& rect);

    static std::unique_ptr<PdfAnnotation> Create(PdfPage& page, const std::type_info& typeInfo, const PdfRect& rect);

    void SetPage(PdfPage& page) { m_Page = &page; }

private:
    static bool tryCreateFromObject(const PdfObject& obj, PdfAnnotationType targetType, PdfAnnotation*& xobj);
    static bool tryCreateFromObject(const PdfObject& obj, const std::type_info& typeInfo, PdfAnnotation*& xobj);
    static PdfAnnotationType getAnnotationType(const std::type_info& typeInfo);
    static PdfAnnotationType getAnnotationType(const PdfObject& obj);
    PdfObject* getAppearanceStream(PdfAppearanceType appearance, const PdfName& state) const;
    PdfDictionary* getAppearanceDictionary() const;

private:
    PdfAnnotationType m_AnnotationType;
    PdfPage* m_Page;
};

template<typename TAnnotation>
bool PdfAnnotation::TryCreateFromObject(PdfObject& obj, std::unique_ptr<TAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, typeid(TAnnotation), xobj_))
        return false;

    xobj.reset((TAnnotation*)xobj_);
    return true;
}

template<typename TAnnotation>
bool PdfAnnotation::TryCreateFromObject(const PdfObject& obj, std::unique_ptr<const TAnnotation>& xobj)
{
    PdfAnnotation* xobj_;
    if (!tryCreateFromObject(obj, typeid(TAnnotation), xobj_))
        return false;

    xobj.reset((const TAnnotation*)xobj_);
    return true;
}

// helper function, to avoid code duplication
void SetAppearanceStreamForObject(PdfObject& obj, PdfXObjectForm& xobj, PdfAppearanceType appearance, const PdfName& state);

};

#endif // PDF_ANNOTATION_H
