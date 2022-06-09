/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfColor.h"

#include <algorithm>
#include <cctype>
#include <pdfmm/private/charconv_compat.h>

#include "PdfDocument.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfObjectStream.h"
#include "PdfTokenizer.h"
#include "PdfVariant.h"
#include "PdfInputDevice.h"

using namespace std;
using namespace cmn;
using namespace mm;

/** A PdfNamedColor holds
 *  a PdfColor object and a name.
 */
class PdfNamedColor
{
public:
    /** Create a PdfNamedColor object.
     *
     *  \param name the name. The string must be allocated as static memory somewhere
     *         The string data will not be copied!
     *  \param color a PdfColor object
     */
    PdfNamedColor(const string_view& name, const PdfColor& color)
        : m_Name(utls::ToLower(name)), m_color(color)
    {
    }

    /** Create a PdfNamedColor object.
     *
     *  \param name the name. The string must be allocated as static memory somewhere
     *         The string data will not be copied!
     *  \param colorName RGB hex value (e.g. #FFABCD)
     */
    PdfNamedColor(const string_view& name, const string_view& colorCode)
        : m_Name(name), m_color(FromRGBString(colorCode))
    {
    }

    /** Copy constructor
     */
    PdfNamedColor(const PdfNamedColor& rhs)
        : m_Name(rhs.m_Name), m_color(rhs.m_color)
    {
    }

    /** Compare this color object to a name
     *  The comparison is case insensitive!
     *  \returns true if the passed string is smaller than the name
     *           of this color object.
     */
    inline bool operator<(const string_view& name) const
    {
        return m_Name < name;
    }

    /** Compare this color object to a PdfNamedColor comparing only the name.
     *  The comparison is case insensitive!
     *  \returns true if the passed string is smaller than the name
     *           of this color object.
     */
    inline bool operator<(const PdfNamedColor& rhs) const
    {
        return m_Name < rhs.GetName();
    }

    /** Compare this color object to a name
     *  The comparison is case insensitive!
     *  \returns true if the passed string is the name
     *           of this color object.
     */
    inline bool operator==(const string_view& name) const
    {
        return m_Name == name;
    }

    /**
     * \returns a reference to the internal color object
     */
    inline const PdfColor& GetColor() const
    {
        return m_color;
    }

    /**
     * \returns a pointer to the name of the color
     */
    inline const string& GetName() const
    {
        return m_Name;
    }

private:
    PdfNamedColor& operator=(const PdfNamedColor&) = delete;

    /** Creates a color object from a RGB string.
     *
     *  \param name a string describing a color.
     *
     *  Supported values are:
     *  - hex values (e.g. #FF002A (RGB))
     *
     *  \returns a PdfColor object
     */
    static PdfColor FromRGBString(const string_view& name);

    string m_Name;
    PdfColor m_color;
};

/**
 * Predicate to allow binary search in the list
 * of PdfNamedColor's using for example std::equal_range.
 */
class NamedColorComparatorPredicate
{
public:
    NamedColorComparatorPredicate()
    {
    }

    inline bool operator()(const PdfNamedColor& namedColor1, const PdfNamedColor& namedColor2) const
    {
        return namedColor1 < namedColor2;
    }
};

// Table based on http://cvsweb.xfree86.org/cvsweb/xc/programs/rgb/rgb.txt?rev=1.2
// Hex values have been copied from http://en.wikipedia.org/wiki/X11_color_names (21/11/2010)
static const size_t s_NamedColorsCount = 148;
static const PdfNamedColor s_NamedColors[s_NamedColorsCount] =
{
    PdfNamedColor( "aliceblue",     "#F0F8FF" ) ,
    PdfNamedColor( "antiquewhite",  "#FAEBD7" ) ,
    PdfNamedColor( "aqua",          "#00FFFF" ) ,
    PdfNamedColor( "aquamarine",    "#7FFFD4" ) ,
    PdfNamedColor( "azure",         "#F0FFFF" ) ,
    PdfNamedColor( "beige",         "#F5F5DC" ) ,
    PdfNamedColor( "bisque",        "#FFE4C4" ) ,
    PdfNamedColor( "black",         "#000000" ) ,
    PdfNamedColor( "blanchedalmond","#FFEBCD" ) ,
    PdfNamedColor( "blue",          "#0000FF" ) ,
    PdfNamedColor( "blueviolet",    "#8A2BE2" ) ,
    PdfNamedColor( "brown",         "#A52A2A" ) ,
    PdfNamedColor( "burlywood",     "#DEB887" ) ,
    PdfNamedColor( "cadetblue",     "#5F9EA0" ) ,
    PdfNamedColor( "chartreuse",    "#7FFF00" ) ,
    PdfNamedColor( "chocolate",     "#D2691E" ) ,
    PdfNamedColor( "coral",         "#FF7F50" ) ,
    PdfNamedColor( "cornflowerblue","#6495ED" ) ,
    PdfNamedColor( "cornsilk",      "#FFF8DC" ) ,
    PdfNamedColor( "crimson",       "#DC143C" ) ,
    PdfNamedColor( "cyan",          "#00FFFF" ) ,
    PdfNamedColor( "darkblue",      "#00008B" ) ,
    PdfNamedColor( "darkcyan",      "#008B8B" ) ,
    PdfNamedColor( "darkgoldenrod", "#B8860B" ) ,
    PdfNamedColor( "darkgray",      "#A9A9A9" ) ,
    PdfNamedColor( "darkgreen",     "#006400" ) ,
    PdfNamedColor( "darkgrey",      "#A9A9A9" ) ,
    PdfNamedColor( "darkkhaki",     "#BDB76B" ) ,
    PdfNamedColor( "darkmagenta",   "#8B008B" ) ,
    PdfNamedColor( "darkolivegreen","#556B2F" ) ,
    PdfNamedColor( "darkorange",    "#FF8C00" ) ,
    PdfNamedColor( "darkorchid",    "#9932CC" ) ,
    PdfNamedColor( "darkred",       "#8B0000" ) ,
    PdfNamedColor( "darksalmon",    "#E9967A" ) ,
    PdfNamedColor( "darkseagreen",  "#8FBC8F" ) ,
    PdfNamedColor( "darkslateblue", "#483D8B" ) ,
    PdfNamedColor( "darkslategray", "#2F4F4F" ) ,
    PdfNamedColor( "darkslategrey", "#2F4F4F" ) ,
    PdfNamedColor( "darkturquoise", "#00CED1" ) ,
    PdfNamedColor( "darkviolet",    "#9400D3" ) ,
    PdfNamedColor( "deeppink",      "#FF1493" ) ,
    PdfNamedColor( "deepskyblue",   "#00BFFF" ) ,
    PdfNamedColor( "dimgray",       "#696969" ) ,
    PdfNamedColor( "dimgrey",       "#696969" ) ,
    PdfNamedColor( "dodgerblue",    "#1E90FF" ) ,
    PdfNamedColor( "firebrick",     "#B22222" ) ,
    PdfNamedColor( "floralwhite",   "#FFFAF0" ) ,
    PdfNamedColor( "forestgreen",   "#228B22" ) ,
    PdfNamedColor( "fuchsia",       "#FF00FF" ) ,
    PdfNamedColor( "gainsboro",     "#DCDCDC" ) ,
    PdfNamedColor( "ghostwhite",    "#F8F8FF" ) ,
    PdfNamedColor( "gold",          "#FFD700" ) ,
    PdfNamedColor( "goldenrod",     "#DAA520" ) ,
    PdfNamedColor( "gray",          "#BEBEBE" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "green",         "#00FF00" ) ,
    PdfNamedColor( "greenyellow",   "#ADFF2F" ) ,
    PdfNamedColor( "grey",          "#BEBEBE" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "honeydew",      "#F0FFF0" ) ,
    PdfNamedColor( "hotpink",       "#FF69B4" ) ,
    PdfNamedColor( "indianred",     "#CD5C5C" ) ,
    PdfNamedColor( "indigo",        "#4B0082" ) ,
    PdfNamedColor( "ivory",         "#FFFFF0" ) ,
    PdfNamedColor( "khaki",         "#F0E68C" ) ,
    PdfNamedColor( "lavender",      "#E6E6FA" ) ,
    PdfNamedColor( "lavenderblush", "#FFF0F5" ) ,
    PdfNamedColor( "lawngreen",     "#7CFC00" ) ,
    PdfNamedColor( "lemonchiffon",  "#FFFACD" ) ,
    PdfNamedColor( "lightblue",     "#ADD8E6" ) ,
    PdfNamedColor( "lightcoral",    "#F08080" ) ,
    PdfNamedColor( "lightcyan",     "#E0FFFF" ) ,
    PdfNamedColor( "lightgoldenrod", "#EEDD82" ) ,
    PdfNamedColor( "lightgoldenrodyellow", "#FAFAD2" ) ,
    PdfNamedColor( "lightgray",     "#D3D3D3" ) ,
    PdfNamedColor( "lightgreen",    "#90EE90" ) ,
    PdfNamedColor( "lightgrey",     "#D3D3D3" ) ,
    PdfNamedColor( "lightpink",     "#FFB6C1" ) ,
    PdfNamedColor( "lightsalmon",   "#FFA07A" ) ,
    PdfNamedColor( "lightseagreen", "#20B2AA" ) ,
    PdfNamedColor( "lightskyblue",  "#87CEFA" ) ,
    PdfNamedColor( "lightslategray","#778899" ) ,
    PdfNamedColor( "lightslategrey","#778899" ) ,
    PdfNamedColor( "lightsteelblue","#B0C4DE" ) ,
    PdfNamedColor( "lightyellow",   "#FFFFE0" ) ,
    PdfNamedColor( "lime",          "#00FF00" ) ,
    PdfNamedColor( "limegreen",     "#32CD32" ) ,
    PdfNamedColor( "linen",         "#FAF0E6" ) ,
    PdfNamedColor( "magenta",       "#FF00FF" ) ,
    PdfNamedColor( "maroon",        "#B03060" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "mediumaquamarine", "#66CDAA" ) ,
    PdfNamedColor( "mediumblue",    "#0000CD" ) ,
    PdfNamedColor( "mediumorchid",  "#BA55D3" ) ,
    PdfNamedColor( "mediumpurple",  "#9370DB" ) ,
    PdfNamedColor( "mediumseagreen","#3CB371" ) ,
    PdfNamedColor( "mediumslateblue", "#7B68EE" ) ,
    PdfNamedColor( "mediumspringgreen", "#00FA9A" ) ,
    PdfNamedColor( "mediumturquoise", "#48D1CC" ) ,
    PdfNamedColor( "mediumvioletred", "#C71585" ) ,
    PdfNamedColor( "midnightblue",  "#191970" ) ,
    PdfNamedColor( "mintcream",     "#F5FFFA" ) ,
    PdfNamedColor( "mistyrose",     "#FFE4E1" ) ,
    PdfNamedColor( "moccasin",      "#FFE4B5" ) ,
    PdfNamedColor( "navajowhite",   "#FFDEAD" ) ,
    PdfNamedColor( "navy",          "#000080" ) ,
    PdfNamedColor( "oldlace",       "#FDF5E6" ) ,
    PdfNamedColor( "olive",         "#808000" ) ,
    PdfNamedColor( "olivedrab",     "#6B8E23" ) ,
    PdfNamedColor( "orange",        "#FFA500" ) ,
    PdfNamedColor( "orangered",     "#FF4500" ) ,
    PdfNamedColor( "orchid",        "#DA70D6" ) ,
    PdfNamedColor( "palegoldenrod", "#EEE8AA" ) ,
    PdfNamedColor( "palegreen",     "#98FB98" ) ,
    PdfNamedColor( "paleturquoise", "#AFEEEE" ) ,
    PdfNamedColor( "palevioletred", "#DB7093" ) ,
    PdfNamedColor( "papayawhip",    "#FFEFD5" ) ,
    PdfNamedColor( "peachpuff",     "#FFDAB9" ) ,
    PdfNamedColor( "peru",          "#CD853F" ) ,
    PdfNamedColor( "pink",          "#FFC0CB" ) ,
    PdfNamedColor( "plum",          "#DDA0DD" ) ,
    PdfNamedColor( "powderblue",    "#B0E0E6" ) ,
    PdfNamedColor( "purple",        "#A020F0" ) , //RG changed from W3C to X11 value
    PdfNamedColor( "red",           "#FF0000" ) ,
    PdfNamedColor( "rosybrown",     "#BC8F8F" ) ,
    PdfNamedColor( "royalblue",     "#4169E1" ) ,
    PdfNamedColor( "saddlebrown",   "#8B4513" ) ,
    PdfNamedColor( "salmon",        "#FA8072" ) ,
    PdfNamedColor( "sandybrown",    "#F4A460" ) ,
    PdfNamedColor( "seagreen",      "#2E8B57" ) ,
    PdfNamedColor( "seashell",      "#FFF5EE" ) ,
    PdfNamedColor( "sienna",        "#A0522D" ) ,
    PdfNamedColor( "silver",        "#C0C0C0" ) ,
    PdfNamedColor( "skyblue",       "#87CEEB" ) ,
    PdfNamedColor( "slateblue",     "#6A5ACD" ) ,
    PdfNamedColor( "slategray",     "#708090" ) ,
    PdfNamedColor( "slategrey",     "#708090" ) ,
    PdfNamedColor( "snow",          "#FFFAFA" ) ,
    PdfNamedColor( "springgreen",   "#00FF7F" ) ,
    PdfNamedColor( "steelblue",     "#4682B4" ) ,
    PdfNamedColor( "tan",           "#D2B48C" ) ,
    PdfNamedColor( "teal",          "#008080" ) ,
    PdfNamedColor( "thistle",       "#D8BFD8" ) ,
    PdfNamedColor( "tomato",        "#FF6347" ) ,
    PdfNamedColor( "turquoise",     "#40E0D0" ) ,
    PdfNamedColor( "violet",        "#EE82EE" ) ,
    PdfNamedColor( "wheat",         "#F5DEB3" ) ,
    PdfNamedColor( "white",         "#FFFFFF" ) ,
    PdfNamedColor( "whitesmoke",    "#F5F5F5" ) ,
    PdfNamedColor( "yellow",        "#FFFF00" ) ,
    PdfNamedColor( "yellowgreen",   "#9ACD32" ) 
};

inline void CheckDoubleRange(double val, double min, double max)
{
    if ((val < min) || (val > max))
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
}

PdfColor::PdfColor() :
    m_Color{ },
    m_SeparationName(),
    m_SeparationDensity(0.0),
    m_ColorSpace(PdfColorSpace::DeviceGray),
    m_AlternateColorSpace(PdfColorSpace::Unknown)
{
    m_Color.gray = 0;
}

PdfColor::PdfColor(double gray) :
    m_Color{ },
    m_SeparationName(),
    m_SeparationDensity(0.0),
    m_ColorSpace(PdfColorSpace::DeviceGray),
    m_AlternateColorSpace(PdfColorSpace::Unknown)
{
    CheckDoubleRange(gray, 0.0, 1.0);
    m_Color.gray = gray;
}

PdfColor::PdfColor(double red, double green, double blue) :
    m_Color{ },
    m_SeparationName(),
    m_SeparationDensity(0.0),
    m_ColorSpace(PdfColorSpace::DeviceRGB),
    m_AlternateColorSpace(PdfColorSpace::Unknown)
{
    CheckDoubleRange(red, 0.0, 1.0);
    CheckDoubleRange(green, 0.0, 1.0);
    CheckDoubleRange(blue, 0.0, 1.0);

    m_Color.rgb[0] = red;
    m_Color.rgb[1] = green;
    m_Color.rgb[2] = blue;
}

PdfColor::PdfColor(double cyan, double magenta, double yellow, double black) :
    m_Color{ },
    m_SeparationName(),
    m_SeparationDensity(0.0),
    m_ColorSpace(PdfColorSpace::DeviceCMYK),
    m_AlternateColorSpace(PdfColorSpace::Unknown)
{
    CheckDoubleRange(cyan, 0.0, 1.0);
    CheckDoubleRange(magenta, 0.0, 1.0);
    CheckDoubleRange(yellow, 0.0, 1.0);
    CheckDoubleRange(black, 0.0, 1.0);

    m_Color.cmyk[0] = cyan;
    m_Color.cmyk[1] = magenta;
    m_Color.cmyk[2] = yellow;
    m_Color.cmyk[3] = black;
}

PdfColor::PdfColor(const Color& data, string separationName, double separationDensity,
    PdfColorSpace colorSpace, PdfColorSpace alternateColorSpace) :
    m_Color(data),
    m_SeparationName(std::move(separationName)),
    m_SeparationDensity(separationDensity),
    m_ColorSpace(colorSpace),
    m_AlternateColorSpace(alternateColorSpace)
{
}

PdfColor PdfColor::CreateCieLab(double cieL, double cieA, double cieB)
{
    CheckDoubleRange(cieL, 0.0, 100.0);
    CheckDoubleRange(cieA, -128.0, 127.0);
    CheckDoubleRange(cieB, -128.0, 127.0);

    Color color{ };
    color.lab[0] = cieL;
    color.lab[1] = cieA;
    color.lab[2] = cieB;
    return PdfColor(color, string(), 0.0, PdfColorSpace::Lab, PdfColorSpace::Unknown);
}

PdfColor PdfColor::CreateSeparation(const std::string_view& name, double density, const PdfColor& alternateColor)
{
    Color color{ };
    switch (alternateColor.GetColorSpace())
    {
        case PdfColorSpace::DeviceGray:
        {
            color.gray = alternateColor.GetGrayScale();
            break;
        }
        case PdfColorSpace::DeviceRGB:
        {
            color.rgb[0] = alternateColor.GetRed();
            color.rgb[1] = alternateColor.GetGreen();
            color.rgb[2] = alternateColor.GetBlue();
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            color.cmyk[0] = alternateColor.GetCyan();
            color.cmyk[1] = alternateColor.GetMagenta();
            color.cmyk[2] = alternateColor.GetYellow();
            color.cmyk[3] = alternateColor.GetBlack();
            break;
        }
        case PdfColorSpace::Lab:
        {
            color.lab[0] = alternateColor.GetCieL();
            color.lab[1] = alternateColor.GetCieA();
            color.lab[2] = alternateColor.GetCieB();
            break;
        }
        case PdfColorSpace::Separation:
        {
            PDFMM_RAISE_LOGIC_IF(true, "PdfColor::PdfColorSeparation alternateColor must be Gray, RGB, CMYK or CieLab!");
            break;
        }
        case PdfColorSpace::Unknown:
        case PdfColorSpace::Indexed:
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    }

    return PdfColor(color, (string)name, density, PdfColorSpace::Separation, alternateColor.GetColorSpace());
}

PdfColor PdfColor::CreateSeparationNone()
{
    Color color{ };
    color.cmyk[0] = 0.0;
    color.cmyk[1] = 0.0;
    color.cmyk[2] = 0.0;
    color.cmyk[3] = 0.0;
    return PdfColor(color, "None", 0.0, PdfColorSpace::Separation, PdfColorSpace::DeviceCMYK);
}

PdfColor PdfColor::CreateSeparationAll()
{
    Color color{ };
    color.cmyk[0] = 1.0;
    color.cmyk[1] = 1.0;
    color.cmyk[2] = 1.0;
    color.cmyk[3] = 1.0;
    return PdfColor(color, "All", 1.0, PdfColorSpace::Separation, PdfColorSpace::DeviceCMYK);
}

PdfColor PdfColor::ConvertToGrayScale() const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpace::DeviceGray:
        {
            return *this;
        }
        case PdfColorSpace::DeviceRGB:
        {
            return PdfColor(0.299 * m_Color.rgb[0] + 0.587 * m_Color.rgb[1] + 0.114 * m_Color.rgb[2]);
        }
        case PdfColorSpace::DeviceCMYK:
        {
            return ConvertToRGB().ConvertToGrayScale();
        }
        case PdfColorSpace::Separation:
        {
            if (m_AlternateColorSpace == PdfColorSpace::DeviceCMYK)
            {
                double cyan = m_Color.cmyk[0];
                double magenta = m_Color.cmyk[1];
                double yellow = m_Color.cmyk[2];
                double black = m_Color.cmyk[3];

                double red = cyan * (1.0 - black) + black;
                double green = magenta * (1.0 - black) + black;
                double blue = yellow * (1.0 - black) + black;

                return PdfColor(1.0 - red, 1.0 - green, 1.0 - blue);
            }
            else
            {
                PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
            }
            break;
        }
        case PdfColorSpace::Lab:
        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    };

    return PdfColor();
}

PdfColor PdfColor::ConvertToRGB() const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpace::DeviceGray:
        {
            return PdfColor(m_Color.gray, m_Color.gray, m_Color.gray);
        }
        case PdfColorSpace::DeviceRGB:
        {
            return *this;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            double cyan = m_Color.cmyk[0];
            double magenta = m_Color.cmyk[1];
            double yellow = m_Color.cmyk[2];
            double black = m_Color.cmyk[3];

            double red = cyan * (1.0 - black) + black;
            double green = magenta * (1.0 - black) + black;
            double blue = yellow * (1.0 - black) + black;

            return PdfColor(1.0 - red, 1.0 - green, 1.0 - blue);
        }
        case PdfColorSpace::Separation:
        {
            if (m_AlternateColorSpace == PdfColorSpace::DeviceCMYK)
            {
                double cyan = m_Color.cmyk[0];
                double magenta = m_Color.cmyk[1];
                double yellow = m_Color.cmyk[2];
                double black = m_Color.cmyk[3];

                double red = cyan * (1.0 - black) + black;
                double green = magenta * (1.0 - black) + black;
                double blue = yellow * (1.0 - black) + black;

                return PdfColor(1.0 - red, 1.0 - green, 1.0 - blue);
            }
            else
            {
                PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
            }

            break;
        }
        case PdfColorSpace::Lab:
        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    };

    return PdfColor();
}

PdfColor PdfColor::ConvertToCMYK() const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpace::DeviceGray:
        {
            return ConvertToRGB().ConvertToCMYK();
        }
        case PdfColorSpace::DeviceRGB:
        {
            double red = m_Color.rgb[0];
            double green = m_Color.rgb[1];
            double blue = m_Color.rgb[2];

            double black = std::min(1.0 - red, std::min(1.0 - green, 1.0 - blue));

            double cyan = 0.0;
            double magenta = 0.0;
            double yellow = 0.0;
            if (black < 1.0)
            {
                cyan = (1.0 - red - black) / (1.0 - black);
                magenta = (1.0 - green - black) / (1.0 - black);
                yellow = (1.0 - blue - black) / (1.0 - black);
            }
            //else do nothing

            return PdfColor(cyan, magenta, yellow, black);
        }
        case PdfColorSpace::DeviceCMYK:
        {
            return *this;
        }
        case PdfColorSpace::Separation:
        case PdfColorSpace::Lab:
        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    }
}

PdfArray PdfColor::ToArray() const
{
    PdfArray array;

    switch (m_ColorSpace)
    {
        case PdfColorSpace::DeviceGray:
        {
            array.Add(m_Color.gray);
            break;
        }
        case PdfColorSpace::DeviceRGB:
        {
            array.Add(m_Color.rgb[0]);
            array.Add(m_Color.rgb[1]);
            array.Add(m_Color.rgb[2]);
            break;
        }
        case PdfColorSpace::DeviceCMYK:
        {
            array.Add(m_Color.cmyk[0]);
            array.Add(m_Color.cmyk[1]);
            array.Add(m_Color.cmyk[2]);
            array.Add(m_Color.cmyk[3]);
            break;
        }
        case PdfColorSpace::Lab:
        {
            array.Add(m_Color.lab[0]);
            array.Add(m_Color.lab[1]);
            array.Add(m_Color.lab[2]);
            break;
        }
        case PdfColorSpace::Separation:
        {
            array.Add(m_SeparationDensity);
            break;
        }
        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            break;
        }
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    }

    return array;
}

PdfColor PdfColor::FromString(const string_view& name)
{
    if (name.length() == 0)
        return PdfColor();

    // first see if it's a single number - if so, that's a single gray value
    if (isdigit(name[0]) || name[0] == '.')
    {
        double grayVal = 0.0;
        if (std::from_chars(name.data() + 1, name.data() + name.size(), grayVal, chars_format::fixed).ec != std::errc())
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoNumber, "Could not read number");

        return PdfColor(grayVal);
    }
    // now check for a hex value (#xxxxxx or #xxxxxxxx)
    else if (name[0] == '#')
    {
        if (name.size() == 7) // RGB
        {
            const unsigned R_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[1]));
            const unsigned R_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[2]));
            const unsigned R = (R_HI << 4) | R_LO;

            const unsigned G_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[3]));
            const unsigned G_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[4]));
            const unsigned G = (G_HI << 4) | G_LO;

            const unsigned B_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[5]));
            const unsigned B_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[6]));
            const unsigned B = (B_HI << 4) | B_LO;

            if (
                (R_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (R_LO != PdfTokenizer::HEX_NOT_FOUND) &&
                (G_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (G_LO != PdfTokenizer::HEX_NOT_FOUND) &&
                (B_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (B_LO != PdfTokenizer::HEX_NOT_FOUND)
                )
            {
                return PdfColor(static_cast<double>(R) / 255.0,
                    static_cast<double>(G) / 255.0,
                    static_cast<double>(B) / 255.0);
            }
            else
            {
                PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            }
        }
        else if (name.size() == 9) // CMYK
        {
            const unsigned C_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[1]));
            const unsigned C_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[2]));
            const unsigned C = (C_HI << 4) | C_LO;

            const unsigned M_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[3]));
            const unsigned M_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[4]));
            const unsigned M = (M_HI << 4) | M_LO;

            const unsigned Y_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[5]));
            const unsigned Y_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[6]));
            const unsigned Y = (Y_HI << 4) | Y_LO;

            const unsigned K_HI = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[7]));
            const unsigned K_LO = static_cast<unsigned>(PdfTokenizer::GetHexValue(name[8]));
            const unsigned K = (K_HI << 4) | K_LO;

            if (
                (C_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (C_LO != PdfTokenizer::HEX_NOT_FOUND) &&
                (M_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (M_LO != PdfTokenizer::HEX_NOT_FOUND) &&
                (Y_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (Y_LO != PdfTokenizer::HEX_NOT_FOUND) &&
                (K_HI != PdfTokenizer::HEX_NOT_FOUND) &&
                (K_LO != PdfTokenizer::HEX_NOT_FOUND)
                )
            {
                return PdfColor(static_cast<double>(C) / 255.0,
                    static_cast<double>(M) / 255.0,
                    static_cast<double>(Y) / 255.0,
                    static_cast<double>(K) / 255.0);
            }
            else
            {
                PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
            }
        }
        else
        {
            return PdfColor();
        }

    }
    // PdfArray
    else if (name[0] == '[')
    {
        PdfMemoryInputDevice device(name);
        PdfTokenizer tokenizer;
        PdfVariant var;

        tokenizer.ReadNextVariant(device, var); // No encryption...
        if (var.IsArray())
            return PdfColor::FromArray(var.GetArray());

        return PdfColor();
    }
    // it must be a named RGB color
    else
    {
        pair<const PdfNamedColor*, const PdfNamedColor*> iterators =
            std::equal_range(&(s_NamedColors[0]),
                s_NamedColors + s_NamedColorsCount,
                PdfNamedColor(name.data(), PdfColor()), NamedColorComparatorPredicate());

        if (iterators.first != iterators.second)
            return iterators.first->GetColor();

        return PdfColor();
    }
}

PdfColor PdfColor::FromArray(const PdfArray& arr)
{
    if (arr.GetSize() == 1) // grayscale
        return PdfColor(arr[0].GetReal());
    else if (arr.GetSize() == 3) // RGB or spot
        return PdfColor(arr[0].GetReal(), arr[1].GetReal(), arr[2].GetReal());
    else if (arr.GetSize() == 4) // CMYK
        return PdfColor(arr[0].GetReal(), arr[1].GetReal(), arr[2].GetReal(), arr[3].GetReal());

    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDataType, "PdfColor::FromArray supports only GrayScale, RGB and CMYK colors");
}

PdfObject* PdfColor::BuildColorSpace(PdfDocument& document) const
{
    switch (m_ColorSpace)
    {
        case PdfColorSpace::Separation:
        {
            // Build color-spaces for separation
            PdfObject* csTintFunc = document.GetObjects().CreateDictionaryObject();

            csTintFunc->GetDictionary().AddKey("BitsPerSample", static_cast<int64_t>(8));

            PdfArray decode;
            decode.Add(static_cast<int64_t>(0));
            decode.Add(static_cast<int64_t>(1));
            decode.Add(static_cast<int64_t>(0));
            decode.Add(static_cast<int64_t>(1));
            decode.Add(static_cast<int64_t>(0));
            decode.Add(static_cast<int64_t>(1));
            decode.Add(static_cast<int64_t>(0));
            decode.Add(static_cast<int64_t>(1));
            csTintFunc->GetDictionary().AddKey("Decode", decode);

            PdfArray domain;
            domain.Add(static_cast<int64_t>(0));
            domain.Add(static_cast<int64_t>(1));
            csTintFunc->GetDictionary().AddKey("Domain", domain);

            PdfArray encode;
            encode.Add(static_cast<int64_t>(0));
            encode.Add(static_cast<int64_t>(1));
            csTintFunc->GetDictionary().AddKey("Encode", encode);

            csTintFunc->GetDictionary().AddKey("Filter", PdfName("FlateDecode"));
            csTintFunc->GetDictionary().AddKey("FunctionType", PdfVariant(static_cast<int64_t>(0)));
            //csTintFunc->GetDictionary().AddKey( "FunctionType", 
            //                                    PdfVariant( static_cast<int64_t>(EPdfFunctionType::Sampled) ) );

            switch (m_AlternateColorSpace)
            {
                case PdfColorSpace::DeviceGray:
                {
                    char data[1 * 2];
                    data[0] = 0;
                    data[1] = static_cast<char> (m_Color.gray);

                    PdfArray range;
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    csTintFunc->GetDictionary().AddKey("Range", range);

                    PdfArray size;
                    size.Add(static_cast<int64_t>(2));
                    csTintFunc->GetDictionary().AddKey("Size", size);

                    PdfMemoryInputStream stream({ data, 1 * 2 });
                    csTintFunc->GetOrCreateStream().Set(stream);

                    PdfArray csArr;
                    csArr.Add(PdfName("Separation"));
                    csArr.Add(PdfName(m_SeparationName));
                    csArr.Add(PdfName("DeviceGray"));
                    csArr.Add(csTintFunc->GetIndirectReference());

                    PdfObject* csp = document.GetObjects().CreateObject(std::move(csArr));

                    return csp;
                }
                break;

                case PdfColorSpace::DeviceRGB:
                {
                    char data[3 * 2];
                    data[0] = 0;
                    data[1] = 0;
                    data[2] = 0;
                    data[3] = static_cast<char> (m_Color.rgb[0] * 255);
                    data[4] = static_cast<char> (m_Color.rgb[1] * 255);
                    data[5] = static_cast<char> (m_Color.rgb[2] * 255);

                    PdfArray range;
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    csTintFunc->GetDictionary().AddKey("Range", range);

                    PdfArray size;
                    size.Add(static_cast<int64_t>(2));
                    csTintFunc->GetDictionary().AddKey("Size", size);

                    PdfMemoryInputStream stream({ data, 3 * 2 });
                    csTintFunc->GetOrCreateStream().Set(stream);

                    PdfArray csArr;
                    csArr.Add(PdfName("Separation"));
                    csArr.Add(PdfName(m_SeparationName));
                    csArr.Add(PdfName("DeviceRGB"));
                    csArr.Add(csTintFunc->GetIndirectReference());

                    PdfObject* csp = document.GetObjects().CreateObject(std::move(csArr));

                    return csp;
                }
                break;

                case PdfColorSpace::DeviceCMYK:
                {
                    char data[4 * 2];
                    data[0] = 0;
                    data[1] = 0;
                    data[2] = 0;
                    data[3] = 0;
                    data[4] = static_cast<char> (m_Color.cmyk[0] * 255);
                    data[5] = static_cast<char> (m_Color.cmyk[1] * 255);
                    data[6] = static_cast<char> (m_Color.cmyk[2] * 255);
                    data[7] = static_cast<char> (m_Color.cmyk[3] * 255);

                    PdfArray range;
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    csTintFunc->GetDictionary().AddKey("Range", range);

                    PdfArray size;
                    size.Add(static_cast<int64_t>(2));
                    csTintFunc->GetDictionary().AddKey("Size", size);

                    PdfArray csArr;
                    csArr.Add(PdfName("Separation"));
                    csArr.Add(PdfName(m_SeparationName));
                    csArr.Add(PdfName("DeviceCMYK"));
                    csArr.Add(csTintFunc->GetIndirectReference());

                    PdfMemoryInputStream stream({ data, 4 * 2 });
                    csTintFunc->GetOrCreateStream().Set(stream); // set stream as last, so that it will work with PdfStreamedDocument

                    PdfObject* csp = document.GetObjects().CreateObject(std::move(csArr));

                    return csp;
                }
                break;

                case PdfColorSpace::Lab:
                {
                    char data[3 * 2];
                    data[0] = 0;
                    data[1] = 0;
                    data[2] = 0;
                    data[3] = static_cast<char> (m_Color.lab[0] * 255);
                    data[4] = static_cast<char> (m_Color.lab[1] * 255);
                    data[5] = static_cast<char> (m_Color.lab[2] * 255);

                    PdfArray range;
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    range.Add(static_cast<int64_t>(0));
                    range.Add(static_cast<int64_t>(1));
                    csTintFunc->GetDictionary().AddKey("Range", range);

                    PdfArray size;
                    size.Add(static_cast<int64_t>(2));
                    csTintFunc->GetDictionary().AddKey("Size", size);

                    PdfMemoryInputStream stream({ data, 3 * 2 });
                    csTintFunc->GetOrCreateStream().Set(stream);

                    PdfArray csArr;
                    csArr.Add(PdfName("Separation"));
                    csArr.Add(PdfName(m_SeparationName));
                    csArr.Add(PdfName("Lab"));
                    csArr.Add(csTintFunc->GetIndirectReference());

                    PdfObject* csp = document.GetObjects().CreateObject(std::move(csArr));

                    return csp;
                }
                break;

                case PdfColorSpace::Separation:
                case PdfColorSpace::Indexed:
                {
                    break;
                }

                case PdfColorSpace::Unknown:
                default:
                {
                    PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
                    break;
                }
            }
        }
        break;

        case PdfColorSpace::Lab:
        {
            // Build color-spaces for CIE-lab
            PdfDictionary labDict;

            // D65-whitepoint
            PdfArray wpArr;
            wpArr.Add(0.9505);
            wpArr.Add(1.0000);
            wpArr.Add(1.0890);
            labDict.AddKey("WhitePoint", wpArr);

            // Range for A,B, L is implicit 0..100
            PdfArray rangeArr;
            rangeArr.Add(static_cast<int64_t>(-128));
            rangeArr.Add(static_cast<int64_t>(127));
            rangeArr.Add(static_cast<int64_t>(-128));
            rangeArr.Add(static_cast<int64_t>(127));
            labDict.AddKey("Range", rangeArr);

            PdfArray labArr;
            labArr.Add(PdfName("Lab"));
            labArr.Add(labDict);

            PdfObject* labp = document.GetObjects().CreateObject(std::move(labArr));

            return labp;
        }
        break;

        case PdfColorSpace::DeviceGray:
        case PdfColorSpace::DeviceRGB:
        case PdfColorSpace::DeviceCMYK:
        case PdfColorSpace::Indexed:
        {
            break;
        }

        case PdfColorSpace::Unknown:
        default:
        {
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
            break;
        }
    }

    return nullptr;
}

PdfColorSpace PdfColor::GetColorSpaceForName(const PdfName& name)
{
    PdfColorSpace ePdfColorSpace = PdfColorSpace::Unknown;

    if (name == "DeviceGray")
    {
        ePdfColorSpace = PdfColorSpace::DeviceGray;
    }
    else if (name == "DeviceRGB")
    {
        ePdfColorSpace = PdfColorSpace::DeviceRGB;
    }
    else if (name == "DeviceCMYK")
    {
        ePdfColorSpace = PdfColorSpace::DeviceCMYK;
    }
    else if (name == "Indexed")
    {
        ePdfColorSpace = PdfColorSpace::Indexed;
    }
    else
    {
        // TODO: other are not supported at the moment
        mm::LogMessage(PdfLogSeverity::Information, "Unsupported colorspace name: {}", name.GetString());
    }

    return ePdfColorSpace;
}

PdfName PdfColor::GetNameForColorSpace(PdfColorSpace colorSpace)
{
    switch (colorSpace)
    {
        case PdfColorSpace::DeviceGray:
            return PdfName("DeviceGray");
        case PdfColorSpace::DeviceRGB:
            return PdfName("DeviceRGB");
        case PdfColorSpace::DeviceCMYK:
            return PdfName("DeviceCMYK");
        case PdfColorSpace::Separation:
            return PdfName("Separation");
        case PdfColorSpace::Lab:
            return PdfName("Lab");
        case PdfColorSpace::Indexed:
            return PdfName("Indexed");
        case PdfColorSpace::Unknown:
        default:
            mm::LogMessage(PdfLogSeverity::Information, "Unsupported colorspace enum: {}", (int)colorSpace);
            return PdfName();
    }

}

PdfColor PdfNamedColor::FromRGBString(const string_view& name)
{
    // This method cannot use PdfTokenizer::GetHexValue() as static values used there have
    // not been initialised yet. This function should used only during program startup
    // and the only purpose is use at s_NamedColors table

    if (name.length() == 7
        && name[0] == '#'
        && isxdigit(name[1]))
    {
        unsigned nameConverted;
        if (std::from_chars(name.data() + 1, name.data() + name.size(), nameConverted, 16).ec != std::errc())
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoNumber, "Could not read number");

        const unsigned R = (nameConverted & 0x00FF0000) >> 16;
        const unsigned G = (nameConverted & 0x0000FF00) >> 8;
        const unsigned B = (nameConverted & 0x000000FF);

        return PdfColor(static_cast<double>(R) / 255.0,
            static_cast<double>(G) / 255.0,
            static_cast<double>(B) / 255.0);
    }
    else
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::CannotConvertColor);
    }
}

bool PdfColor::operator==(const PdfColor& rhs) const
{
    if (m_ColorSpace == rhs.m_ColorSpace)
    {
        if (m_ColorSpace == PdfColorSpace::DeviceGray
            && m_Color.gray == rhs.m_Color.gray)
        {
            return true;
        }

        if (m_ColorSpace == PdfColorSpace::DeviceRGB
            && m_Color.rgb[0] == rhs.m_Color.rgb[0]
            && m_Color.rgb[1] == rhs.m_Color.rgb[1]
            && m_Color.rgb[2] == rhs.m_Color.rgb[2])
        {
            return true;
        }

        if (m_ColorSpace == PdfColorSpace::DeviceCMYK
            && m_Color.cmyk[0] == rhs.m_Color.cmyk[0]
            && m_Color.cmyk[1] == rhs.m_Color.cmyk[1]
            && m_Color.cmyk[2] == rhs.m_Color.cmyk[2]
            && m_Color.cmyk[3] == rhs.m_Color.cmyk[3])
        {
            return true;
        }

        if (m_ColorSpace == PdfColorSpace::Lab
            && m_Color.lab[0] == rhs.m_Color.lab[0]
            && m_Color.lab[1] == rhs.m_Color.lab[1]
            && m_Color.lab[2] == rhs.m_Color.lab[2])
        {
            return true;
        }

        if (m_ColorSpace == PdfColorSpace::Separation
            && m_SeparationDensity == rhs.m_SeparationDensity
            && m_SeparationName == rhs.m_SeparationName
            && m_AlternateColorSpace == rhs.m_AlternateColorSpace
            && ((m_AlternateColorSpace == PdfColorSpace::DeviceGray
                && m_Color.gray == rhs.m_Color.gray)
                || (m_AlternateColorSpace == PdfColorSpace::DeviceRGB
                    && m_Color.rgb[0] == rhs.m_Color.rgb[0]
                    && m_Color.rgb[1] == rhs.m_Color.rgb[1]
                    && m_Color.rgb[2] == rhs.m_Color.rgb[2])
                || (m_AlternateColorSpace == PdfColorSpace::DeviceCMYK
                    && m_Color.cmyk[0] == rhs.m_Color.cmyk[0]
                    && m_Color.cmyk[1] == rhs.m_Color.cmyk[1]
                    && m_Color.cmyk[2] == rhs.m_Color.cmyk[2]
                    && m_Color.cmyk[3] == rhs.m_Color.cmyk[3])
                || (m_AlternateColorSpace == PdfColorSpace::Lab
                    && m_Color.lab[0] == rhs.m_Color.lab[0]
                    && m_Color.lab[1] == rhs.m_Color.lab[1]
                    && m_Color.lab[2] == rhs.m_Color.lab[2])
                ))
        {
            return true;
        }

        if (m_ColorSpace == PdfColorSpace::Unknown)
            return true;
    }
    return false;
}

bool PdfColor::operator!=(const PdfColor& rhs) const
{
    return !(*this == rhs);
}

bool PdfColor::IsGrayScale() const
{
    return m_ColorSpace == PdfColorSpace::DeviceGray;
}

bool PdfColor::IsRGB() const
{
    return m_ColorSpace == PdfColorSpace::DeviceRGB;
}

bool PdfColor::IsCMYK() const
{
    return m_ColorSpace == PdfColorSpace::DeviceCMYK;
}

bool PdfColor::IsSeparation() const
{
    return m_ColorSpace == PdfColorSpace::Separation;
}

bool PdfColor::IsCieLab() const
{
    return m_ColorSpace == PdfColorSpace::Lab;
}

PdfColorSpace PdfColor::GetAlternateColorSpace() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsSeparation(), "PdfColor::GetAlternateColorSpace cannot be called on non separation color objects!");
    return m_AlternateColorSpace;
}

double PdfColor::GetGrayScale() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsGrayScale()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceGray)),
        "PdfColor::GetGrayScale cannot be called on non grayscale color objects!");

    return m_Color.gray;
}

double PdfColor::GetRed() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsRGB()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceRGB)),
        "PdfColor::GetRed cannot be called on non RGB color objects!");

    return m_Color.rgb[0];
}

double PdfColor::GetGreen() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsRGB()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceRGB)),
        "PdfColor::GetGreen cannot be called on non RGB color objects!");

    return m_Color.rgb[1];
}

double PdfColor::GetBlue() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsRGB()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceRGB)),
        "PdfColor::GetBlue cannot be called on non RGB color objects!");

    return m_Color.rgb[2];
}

double PdfColor::GetCyan() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCMYK()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceCMYK)),
        "PdfColor::GetCyan cannot be called on non CMYK color objects!");

    return m_Color.cmyk[0];
}

double PdfColor::GetMagenta() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCMYK()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceCMYK)),
        "PdfColor::GetMagenta cannot be called on non CMYK color objects!");

    return m_Color.cmyk[1];
}

double PdfColor::GetYellow() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCMYK()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceCMYK)),
        "PdfColor::GetYellow cannot be called on non CMYK color objects!");

    return m_Color.cmyk[2];
}

double PdfColor::GetBlack() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCMYK()
        && !(this->IsSeparation() && (this->m_AlternateColorSpace == PdfColorSpace::DeviceCMYK)),
        "PdfColor::GetBlack cannot be called on non CMYK color objects!");

    return m_Color.cmyk[3];
}

const string& PdfColor::GetName() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsSeparation(), "PdfColor::GetName cannot be called on non separation color objects!");

    return m_SeparationName;
}

double PdfColor::GetDensity() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsSeparation(), "PdfColor::GetDensity cannot be called on non separation color objects!");

    return m_SeparationDensity;
}

double PdfColor::GetCieL() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCieLab()
        && !(this->IsSeparation() && this->m_AlternateColorSpace == PdfColorSpace::Lab),
        "PdfColor::GetCieL cannot be called on non CIE-Lab color objects!");

    return m_Color.lab[0];
}

double PdfColor::GetCieA() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCieLab()
        && !(this->IsSeparation() && this->m_AlternateColorSpace == PdfColorSpace::Lab),
        "PdfColor::GetCieA cannot be called on non CIE-Lab color objects!");

    return m_Color.lab[1];
}

double PdfColor::GetCieB() const
{
    PDFMM_RAISE_LOGIC_IF(!this->IsCieLab()
        && !(this->IsSeparation() && this->m_AlternateColorSpace == PdfColorSpace::Lab),
        "PdfColor::GetCieB cannot be called on non CIE-Lab color objects!");

    return m_Color.lab[2];
}
