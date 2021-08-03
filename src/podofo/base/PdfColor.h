/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_COLOR_H
#define PDF_COLOR_H

#include "PdfDefines.h"

#include "PdfName.h"

namespace PoDoFo {

class PdfArray;
class PdfObject;
class PdfDocument;

/** A color object can represent either a grayscale
 *  value, a RGB color, a CMYK color, a separation color or
 *  a CieLab color.
 *
 *  All drawing functions in PoDoFo accept a PdfColor object
 *  to specify a drawing color in one of these colorspaces.
 *
 *  Derived classes PdfColorGray, PdfColorRGB, PdfColorCMYK, PdfColorSeparation
 *  and PdfColorCieLab are available for easy construction
 */
class PODOFO_API PdfColor
{
public:
    /** Create a PdfColor object that is RGB black.
     */
    PdfColor();

    /** Create a new PdfColor object with
     *  a grayscale value.
     *
     *  \param dGray a grayscale value between 0.0 and 1.0
     */
    explicit PdfColor(double dGray);

    /** Create a new PdfColor object with
     *  a RGB color
     *
     *  \param dRed the value of the red component, must be between 0.0 and 1.0
     *  \param dGreen the value of the green component, must be between 0.0 and 1.0
     *  \param dBlue the value of the blue component, must be between 0.0 and 1.0
     */
    PdfColor(double dRed, double dGreen, double dBlue);

    /** Create a new PdfColor object with
     *  a CMYK color
     *
     *  \param dCyan the value of the cyan component, must be between 0.0 and 1.0
     *  \param dMagenta the value of the magenta component, must be between 0.0 and 1.0
     *  \param dYellow the value of the yellow component, must be between 0.0 and 1.0
     *  \param dBlack the value of the black component, must be between 0.0 and 1.0
     */
    PdfColor(double dCyan, double dMagenta, double dYellow, double dBlack);

    /** Copy constructor
     *
     *  \param rhs copy rhs into this object
     */
    PdfColor(const PdfColor& rhs);

    /** Destructor
     */
    virtual ~PdfColor();

    /** Assignment operator
     *
     *  \param rhs copy rhs into this object
     *
     *  \returns a reference to this color object
     */
    const PdfColor& operator=(const PdfColor& rhs);

    /** Test for equality of colors.
     *
     *  \param rhs color to compare to
     *
     *  \returns true if object color is equal to rhs
     */
    bool operator==(const PdfColor& rhs) const;

    /** Test for inequality of colors.
     *
     *  \param rhs color to compare to
     *
     *  \returns true if object color is not equal to rhs
     */
    bool operator!=(const PdfColor& rhs) const;

    /** Test if this is a grayscale color.
     *
     *  \returns true if this is a grayscale PdfColor object
     */
    bool IsGrayScale() const;

    /** Test if this is a RGB color.
     *
     *  \returns true if this is a RGB PdfColor object
     */
    bool IsRGB() const;

    /** Test if this is a CMYK color.
     *
     *  \returns true if this is a CMYK PdfColor object
     */
    bool IsCMYK() const;

    /** Test if this is a separation color.
     *
     *  \returns true if this is a separation PdfColor object
     */
    bool IsSeparation() const;

    /** Test if this is a CIE-Lab color.
     *
     *  \returns true if this is a lab Color object
     */
    bool IsCieLab() const;

    /** Get the colorspace of this PdfColor object
     *
     *  \returns the colorspace of this PdfColor object
     */
    inline PdfColorSpace GetColorSpace() const { return m_eColorSpace; }

    /** Get the alternate colorspace of this PdfColor object
     *
     *  \returns the colorspace of this PdfColor object (must be separation)
     */
    PdfColorSpace GetAlternateColorSpace() const;

    /** Get the grayscale color value
     *  of this object.
     *
     *  Throws an exception if this is no grayscale color object.
     *
     *  \returns the grayscale color value of this object (between 0.0 and 1.0)
     *
     *  \see IsGrayScale
     */
    double GetGrayScale() const;

    /** Get the red color value
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the red color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    double GetRed() const;

    /** Get the green color value
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the green color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    double GetGreen() const;

    /** Get the blue color value
     *  of this object.
     *
     *  Throws an exception if this is no RGB color object.
     *
     *  \returns the blue color value of this object (between 0.0 and 1.0)
     *
     *  \see IsRGB
     */
    double GetBlue() const;

    /** Get the cyan color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the cyan color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetCyan() const;

    /** Get the magenta color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the magenta color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetMagenta() const;

    /** Get the yellow color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the yellow color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetYellow() const;

    /** Get the black color value
     *  of this object.
     *
     *  Throws an exception if this is no CMYK or separation color object.
     *
     *  \returns the black color value of this object (between 0.0 and 1.0)
     *
     *  \see IsCMYK
     */
    double GetBlack() const;

    /** Get the separation name of this object.
     *
     *  Throws an exception if this is no separation color object.
     *
     *  \returns the name of this object
     *
     *  \see IsSeparation
     */
    const std::string GetName() const;

    /** Get the density color value
     *  of this object.
     *
     *  Throws an exception if this is no separation color object.
     *
     *  \returns the density value of this object (between 0.0 and 1.0)
     *
     *  \see IsSeparation
     */
    double GetDensity() const;

    /** Get the L color value
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the L color value of this object (between 0.0 and 100.0)
     *
     *  \see IsCieLab
     */
    double GetCieL() const;

    /** Get the A color value
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the A color value of this object (between -128.0 and 127.0)
     *
     *  \see IsCieLab
     */
    double GetCieA() const;

    /** Get the B color value
     *  of this object.
     *
     *  Throws an exception if this is no CIE-Lab color object.
     *
     *  \returns the B color value of this object (between -128.0 and 127.0)
     *
     *  \see IsCieLab
     */
    double GetCieB() const;

    /** Converts the color object into a grayscale
     *  color object.
     *
     *  This is only a convenience function. It might be useful
     *  for on screen display but is in NO WAY suitable to
     *  professional printing!
     *
     *  \returns a grayscale color object
     *  \see IsGrayScale()
     */
    PdfColor ConvertToGrayScale() const;

    /** Converts the color object into a RGB
     *  color object.
     *
     *  This is only a convenience function. It might be useful
     *  for on screen display but is in NO WAY suitable to
     *  professional printing!
     *
     *  \returns a RGB color object
     *  \see IsRGB()
     */
    PdfColor ConvertToRGB() const;

    /** Converts the color object into a CMYK
     *  color object.
     *
     *  This is only a convenience function. It might be useful
     *  for on screen display but is in NO WAY suitable to
     *  professional printing!
     *
     *  \returns a CMYK color object
     *  \see IsCMYK()
     */
    PdfColor ConvertToCMYK() const;

    /** Creates a PdfArray which represents a color from a color.
     *  \returns a PdfArray object
     */
    PdfArray ToArray() const;

    /** Creates a color object from a string.
     *
     *  \param pszName a string describing a color.
     *
     *  Supported values are:
     *  - single gray values as string (e.g. '0.5')
     *  - a named color (e.g. 'aquamarine' or 'magenta')
     *  - hex values (e.g. #FF002A (RGB) or #FF12AB3D (CMYK))
     *  - PdfArray's
     *
     *  \returns a PdfColor object
     */
    static PdfColor FromString(const std::string_view& name);

    /** Creates a color object from a PdfArray which represents a color.
     *
     *  Raises an exception if this is no PdfColor!
     *
     *  \param rArray an array that must be a color PdfArray
     *  \returns a PdfColor object
     */
    static PdfColor FromArray(const PdfArray& rArray);

    /**
     *  Convert a name into a colorspace enum.
     *
     *  \param rName name representing a colorspace such as DeviceGray
     *  \returns colorspace enum or ePdfColorSpace_Unknown if name is unknown
     *  \see GetNameForColorSpace
     */
    static PdfColorSpace GetColorSpaceForName(const PdfName& rName);

    /*
     *  Convert a colorspace enum value into a name such as DeviceRGB
     *
     *  \param eColorSpace a colorspace
     *  \returns a name
     *  \see GetColorSpaceForName
     */
    static PdfName GetNameForColorSpace(PdfColorSpace eColorSpace);

    /** Creates a colorspace object from a color to insert into resources.
     *
     *  \param pOwner a pointer to the owner of the generated object
     *  \returns a PdfObject pointer, which can be insert into resources, nullptr if not needed
     */
    PdfObject* BuildColorSpace(PdfDocument& document) const;

protected:
    union
    {
        double cmyk[4];
        double rgb[3];
        double lab[3];
        double gray;
    } m_uColor;
    std::string m_separationName;
    double m_separationDensity;
    PdfColorSpace m_eColorSpace;
    PdfColorSpace m_eAlternateColorSpace;

private:
    static const unsigned* const m_hexDigitMap; // Mapping of hex sequences to int value
};

class PODOFO_API PdfColorGray : public PdfColor
{
public:

    /** Create a new PdfColor object with a grayscale value.
     *
     *  \param dGray a grayscale value between 0.0 and 1.0
     */
    explicit PdfColorGray(double dGray);

private:
    /** Default constructor, not implemented
     */
    PdfColorGray();

    /** Copy constructor, not implemented
     */
    PdfColorGray(const PdfColorGray&);

    /** Copy assignment operator, not implemented
     */
    PdfColorGray& operator=(const PdfColorGray&) = delete;
};

class PODOFO_API PdfColorRGB : public PdfColor
{
public:
    /** Create a new PdfColor object with
     *  a RGB color
     *
     *  \param dRed the value of the red component, must be between 0.0 and 1.0
     *  \param dGreen the value of the green component, must be between 0.0 and 1.0
     *  \param dBlue the value of the blue component, must be between 0.0 and 1.0
     */
    PdfColorRGB(double dRed, double dGreen, double dBlue);

private:
    /** Default constructor, not implemented
     */
    PdfColorRGB();

    /** Copy constructor, not implemented
     */
    PdfColorRGB(const PdfColorRGB&);

    /** Copy assignment operator, not implemented
     */
    PdfColorRGB& operator=(const PdfColorRGB&) = delete;
};

class PODOFO_API PdfColorCMYK : public PdfColor
{
public:

    /** Create a new PdfColor object with a CMYK color
     *
     *  \param dCyan the value of the cyan component, must be between 0.0 and 1.0
     *  \param dMagenta the value of the magenta component, must be between 0.0 and 1.0
     *  \param dYellow the value of the yellow component, must be between 0.0 and 1.0
     *  \param dBlack the value of the black component, must be between 0.0 and 1.0
     */
    PdfColorCMYK(double dCyan, double dMagenta, double dYellow, double dBlack);

private:
    /** Default constructor, not implemented
     */
    PdfColorCMYK();

    /** Copy constructor, not implemented
     */
    PdfColorCMYK(const PdfColorCMYK&);

    /** Copy assignment operator, not implemented
     */
    PdfColorCMYK& operator=(const PdfColorCMYK&) = delete;
};

class PODOFO_API PdfColorSeparationAll : public PdfColor
{
public:

    /** Create a new PdfColor object with
    *  Separation color All.
    *
    */
    PdfColorSeparationAll();

private:
    /** Copy constructor, not implemented
     */
    PdfColorSeparationAll(const PdfColorSeparationAll&);

    /** Copy assignment operator, not implemented
     */
    PdfColorSeparationAll& operator=(const PdfColorSeparationAll&) = delete;
};

class PODOFO_API PdfColorSeparationNone : public PdfColor
{
public:

    /** Create a new PdfColor object with
    *  Separation color None.
    *
    */
    PdfColorSeparationNone();

private:
    /** Copy constructor, not implemented
     */
    PdfColorSeparationNone(const PdfColorSeparationNone&);

    /** Copy assignment operator, not implemented
     */
    PdfColorSeparationNone& operator=(const PdfColorSeparationNone&) = delete;
};

class PODOFO_API PdfColorSeparation : public PdfColor
{
public:

    /** Create a new PdfColor object with
     *  a separation-name and an equivalent color
     *
     *  \param sName Name of the separation color
     *  \param sDensity the density value of the separation color
     *  \param alternateColor the alternate color, must be of type gray, rgb, cmyk or cie
     */
    PdfColorSeparation(const std::string_view& sName, double dDensity, const PdfColor& alternateColor);

private:
    /** Default constructor, not implemented
     */
    PdfColorSeparation();

    /** Copy constructor, not implemented
     */
    PdfColorSeparation(const PdfColorSeparation&);

    /** Copy assignment operator, not implemented
     */
    PdfColorSeparation& operator=(const PdfColorSeparation&) = delete;
};

class PODOFO_API PdfColorCieLab : public PdfColor
{
public:

    /** Create a new PdfColor object with a CIE-LAB-value
     *
     *  \param dCieL the value of the L component, must be between 0.0 and 100.0
     *  \param dCieA the value of the A component, must be between -128.0 and 127.0
     *  \param dCieB the value of the B component, must be between -128.0 and 127.0
     */
    PdfColorCieLab(double dCieL, double dCieA, double dCieB);

private:
    /** Default constructor, not implemented
     */
    PdfColorCieLab();

    /** Copy constructor, not implemented
     */
    PdfColorCieLab(const PdfColorCieLab&);

    /** Copy assignment operator, not implemented
     */
    PdfColorCieLab& operator=(const PdfColorCieLab&) = delete;
};

};

#endif // PDF_COLOR_H
