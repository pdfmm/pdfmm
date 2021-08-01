/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfShadingPattern.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfColor.h"
#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfStream.h"
#include "base/PdfWriter.h"

#include "PdfFunction.h"

#include <sstream>

using namespace PoDoFo;

PdfShadingPattern::PdfShadingPattern(PdfDocument& doc, EPdfShadingPatternType eShadingType)
    : PdfElement(doc, "Pattern")
{
    std::ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Sh" << this->GetObject().GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName(out.str().c_str());

    this->Init(eShadingType);
}

void PdfShadingPattern::Init(EPdfShadingPatternType eShadingType)
{
    /*
    switch( eShadingType )
    {
        case EPdfShadingPatternType::FunctionBase:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case EPdfShadingPatternType::Radial:
        {
            PdfArray coords;
            coords.push_back( 0.0 );
            coords.push_back( 0.0 );
            coords.push_back( 0.096 );
            coords.push_back( 0.0 );
            coords.push_back( 0.0 );
            coords.push_back( 1.0 );

            PdfArray domain;
            domain.push_back( 0.0 );
            domain.push_back( 1.0 );

            PdfArray c0;
            c0.push_back( 0.929 );
            c0.push_back( 0.357 );
            c0.push_back( 1.0 );
            c0.push_back( 0.298 );

            PdfArray c1a;
            c0.push_back( 0.631 );
            c0.push_back( 0.278 );
            c0.push_back( 1.0 );
            c0.push_back( 0.027 );

            PdfArray c1b;
            c0.push_back( 0.94 );
            c0.push_back( 0.4 );
            c0.push_back( 1.0 );
            c0.push_back( 0.102 );


            PdfExponentialFunction f1( domain, c0, c1a, 1.048, this->GetObject()->GetOwner() );
            PdfExponentialFunction f2( domain, c0, c1b, 1.374, this->GetObject()->GetOwner() );

            PdfFunction::List list;
            list.push_back( f1 );
            list.push_back( f2 );

            PdfArray bounds;
            bounds.push_back( 0.708 );

            PdfArray encode;
            encode.push_back( 1.0 );
            encode.push_back( 0.0 );
            encode.push_back( 0.0 );
            encode.push_back( 1.0 );

            PdfStitchingFunction function( list, domain, bounds, encode, this->GetObject()->GetOwner() );

            shading.AddKey( "Coords", coords );
            shading.AddKey( "Function", function.GetObject()->GetIndirectReference() );
            break;
        }
        case EPdfShadingPatternType::FreeForm:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case EPdfShadingPatternType::LatticeForm:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case EPdfShadingPatternType::CoonsPatch:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        case EPdfShadingPatternType::TensorProduct:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
            break;
        default:
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::InvalidEnumValue, "PdfShadingPattern::Init() failed because of an invalid shading pattern type." );
        }
        };
    */

    // keys common to all shading directories
    PdfDictionary shading;
    shading.AddKey("ShadingType", static_cast<int64_t>(eShadingType));

    this->GetObject().GetDictionary().AddKey("PatternType", static_cast<int64_t>(2)); // Shading pattern
    if (eShadingType < EPdfShadingPatternType::FreeForm)
    {
        this->GetObject().GetDictionary().AddKey("Shading", shading);
    }
    else
    {
        PdfObject* shadingObject = this->GetObject().GetDocument()->GetObjects().CreateObject(shading);
        this->GetObject().GetDictionary().AddKey("Shading", shadingObject->GetIndirectReference());
    }
}

PdfAxialShadingPattern::PdfAxialShadingPattern(PdfDocument& doc, double dX0, double dY0, double dX1, double dY1, const PdfColor& rStart, const PdfColor& rEnd)
    : PdfShadingPattern(doc, EPdfShadingPatternType::Axial)
{
    Init(dX0, dY0, dX1, dY1, rStart, rEnd);
}

void PdfAxialShadingPattern::Init(double dX0, double dY0, double dX1, double dY1, const PdfColor& rStart, const PdfColor& rEnd)
{
    PdfArray coords;
    coords.push_back(dX0);
    coords.push_back(dY0);
    coords.push_back(dX1);
    coords.push_back(dY1);

    if (rStart.GetColorSpace() != rEnd.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Colorspace of start and end color in PdfAxialShadingPattern does not match.");

    PdfArray c0 = rStart.ToArray();
    PdfArray c1 = rEnd.ToArray();
    PdfArray extend;

    extend.push_back(true);
    extend.push_back(true);

    PdfArray domain;
    domain.push_back(0.0);
    domain.push_back(1.0);

    PdfExponentialFunction function(*this->GetObject().GetDocument(), domain, c0, c1, 1.0);

    PdfDictionary& shading = this->GetObject().GetDictionary().GetKey(PdfName("Shading"))->GetDictionary();

    switch (rStart.GetColorSpace())
    {
        case PdfColorSpace::DeviceRGB:
            shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
            break;

        case PdfColorSpace::DeviceCMYK:
            shading.AddKey("ColorSpace", PdfName("DeviceCMYK"));
            break;

        case PdfColorSpace::DeviceGray:
            shading.AddKey("ColorSpace", PdfName("DeviceGray"));
            break;

        case PdfColorSpace::CieLab:
        {
            PdfObject* csp = rStart.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Separation:
        {
            PdfObject* csp = rStart.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::CannotConvertColor, "Colorspace not supported in PdfAxialShadingPattern.");
            break;
    }

    shading.AddKey("Coords", coords);
    shading.AddKey("Function", function.GetObject().GetIndirectReference());
    shading.AddKey("Extend", extend);
}

PdfFunctionBaseShadingPattern::PdfFunctionBaseShadingPattern(PdfDocument& doc, const PdfColor& rLL, const PdfColor& rUL, const PdfColor& rLR, const PdfColor& rUR, const PdfArray& rMatrix)
    : PdfShadingPattern(doc, EPdfShadingPatternType::FunctionBase)
{
    Init(rLL, rUL, rLR, rUR, rMatrix);
}

void PdfFunctionBaseShadingPattern::Init(const PdfColor& rLL, const PdfColor& rUL, const PdfColor& rLR, const PdfColor& rUR, const PdfArray& rMatrix)
{
    if (rLL.GetColorSpace() != rUL.GetColorSpace() || rUL.GetColorSpace() != rLR.GetColorSpace() || rLR.GetColorSpace() != rUR.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Colorspace of start and end color in PdfFunctionBaseShadingPattern does not match.");

    PdfArray domain;
    domain.push_back(0.0);
    domain.push_back(1.0);
    domain.push_back(0.0);
    domain.push_back(1.0);

    PdfDictionary& shading = this->GetObject().GetDictionary().GetKey("Shading")->GetDictionary();
    PdfArray range;
    PdfSampledFunction::Sample samples;

    switch (rLL.GetColorSpace())
    {
        case PdfColorSpace::DeviceRGB:
        {
            range.push_back(0.0);
            range.push_back(1.0);
            range.push_back(0.0);
            range.push_back(1.0);
            range.push_back(0.0);
            range.push_back(1.0);

            samples.insert(samples.end(), static_cast<char> (rLL.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLL.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLL.GetBlue() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rLR.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLR.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLR.GetBlue() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rUL.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUL.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUL.GetBlue() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rUR.GetRed() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUR.GetGreen() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUR.GetBlue() * 255.0));

            shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
        }
        break;

        case PdfColorSpace::DeviceCMYK:
        {
            range.push_back(0.0);
            range.push_back(1.0);
            range.push_back(0.0);
            range.push_back(1.0);
            range.push_back(0.0);
            range.push_back(1.0);
            range.push_back(0.0);
            range.push_back(1.0);

            samples.insert(samples.end(), static_cast<char> (rLL.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLL.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLL.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLL.GetBlack() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rLR.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLR.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLR.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLR.GetBlack() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rUL.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUL.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUL.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUL.GetBlack() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rUR.GetCyan() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUR.GetMagenta() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUR.GetYellow() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUR.GetBlack() * 255.0));

            shading.AddKey("ColorSpace", PdfName("DeviceCMYK"));
        }
        break;

        case PdfColorSpace::DeviceGray:
        {
            range.push_back(0.0);
            range.push_back(1.0);

            samples.insert(samples.end(), static_cast<char> (rLL.GetGrayScale() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rLR.GetGrayScale() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rUL.GetGrayScale() * 255.0));

            samples.insert(samples.end(), static_cast<char> (rUR.GetGrayScale() * 255.0));

            shading.AddKey("ColorSpace", PdfName("DeviceGray"));
        }
        break;

        case PdfColorSpace::CieLab:
        {
            range.push_back(0.0);
            range.push_back(100.0);
            range.push_back(-128.0);
            range.push_back(127.0);
            range.push_back(-128.0);
            range.push_back(127.0);

            samples.insert(samples.end(), static_cast<char> (rLL.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (rLL.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (rLL.GetCieB() + 128));

            samples.insert(samples.end(), static_cast<char> (rLR.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (rLR.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (rLR.GetCieB() + 128));

            samples.insert(samples.end(), static_cast<char> (rUL.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (rUL.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (rUL.GetCieB() + 128));

            samples.insert(samples.end(), static_cast<char> (rUR.GetCieL() * 2.55));
            samples.insert(samples.end(), static_cast<char> (rUR.GetCieA() + 128));
            samples.insert(samples.end(), static_cast<char> (rUR.GetCieB() + 128));

            PdfObject* csp = rLL.BuildColorSpace(*this->GetObject().GetDocument());

            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Separation:
        {
            range.push_back(0.0);
            range.push_back(1.0);

            samples.insert(samples.end(), static_cast<char> (rLL.GetDensity() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rLR.GetDensity() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUL.GetDensity() * 255.0));
            samples.insert(samples.end(), static_cast<char> (rUR.GetDensity() * 255.0));

            PdfObject* csp = rLL.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::CannotConvertColor, "Colorspace not supported in PdfFunctionBaseShadingPattern.");
            break;
    }

    PdfSampledFunction function(*this->GetObject().GetDocument(), domain, range, samples);
    shading.AddKey("Function", function.GetObject().GetIndirectReference());
    shading.AddKey("Domain", domain);
    shading.AddKey("Matrix", rMatrix);
}

PdfRadialShadingPattern::PdfRadialShadingPattern(PdfDocument& doc, double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor& rStart, const PdfColor& rEnd)
    : PdfShadingPattern(doc, EPdfShadingPatternType::Radial)
{
    Init(dX0, dY0, dR0, dX1, dY1, dR1, rStart, rEnd);
}

void PdfRadialShadingPattern::Init(double dX0, double dY0, double dR0, double dX1, double dY1, double dR1, const PdfColor& rStart, const PdfColor& rEnd)
{
    PdfArray coords;
    coords.push_back(dX0);
    coords.push_back(dY0);
    coords.push_back(dR0);
    coords.push_back(dX1);
    coords.push_back(dY1);
    coords.push_back(dR1);

    if (rStart.GetColorSpace() != rEnd.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Colorspace of start and end color in PdfRadialShadingPattern does not match.");

    PdfArray c0 = rStart.ToArray();
    PdfArray c1 = rEnd.ToArray();
    PdfArray extend;

    extend.push_back(true);
    extend.push_back(true);

    PdfArray domain;
    domain.push_back(0.0);
    domain.push_back(1.0);

    PdfExponentialFunction function(*this->GetObject().GetDocument(), domain, c0, c1, 1.0);

    PdfDictionary& shading = this->GetObject().GetDictionary().GetKey("Shading")->GetDictionary();

    switch (rStart.GetColorSpace())
    {
        case PdfColorSpace::DeviceRGB:
            shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
            break;

        case PdfColorSpace::DeviceCMYK:
            shading.AddKey("ColorSpace", PdfName("DeviceCMYK"));
            break;

        case PdfColorSpace::DeviceGray:
            shading.AddKey("ColorSpace", PdfName("DeviceGray"));
            break;

        case PdfColorSpace::CieLab:
        {
            PdfObject* csp = rStart.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Separation:
        {
            PdfObject* csp = rStart.BuildColorSpace(*this->GetObject().GetDocument());
            shading.AddKey("ColorSpace", csp->GetIndirectReference());
        }
        break;

        case PdfColorSpace::Indexed:
        case PdfColorSpace::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO(EPdfError::CannotConvertColor, "Colorspace not supported in PdfRadialShadingPattern.");
            break;
    }

    shading.AddKey("Coords", coords);
    shading.AddKey("Function", function.GetObject().GetIndirectReference());
    shading.AddKey("Extend", extend);
}

PdfTriangleShadingPattern::PdfTriangleShadingPattern(PdfDocument& doc, double dX0, double dY0, const PdfColor& color0, double dX1, double dY1, const PdfColor& color1, double dX2, double dY2, const PdfColor& color2)
    : PdfShadingPattern(doc, EPdfShadingPatternType::FreeForm)
{
    Init(dX0, dY0, color0, dX1, dY1, color1, dX2, dY2, color2);
}

void PdfTriangleShadingPattern::Init(double dX0, double dY0, const PdfColor& color0, double dX1, double dY1, const PdfColor& color1, double dX2, double dY2, const PdfColor& color2)
{
    if (color0.GetColorSpace() != color1.GetColorSpace() || color0.GetColorSpace() != color2.GetColorSpace())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Colorspace of start and end color in PdfTriangleShadingPattern does not match.");

    PdfColor rgb0 = color0.ConvertToRGB();
    PdfColor rgb1 = color1.ConvertToRGB();
    PdfColor rgb2 = color2.ConvertToRGB();

    PdfArray decode;
    double minx, maxx, miny, maxy;

    minx = std::min(std::min(dX0, dX1), dX2);
    maxx = std::max(std::max(dX0, dX1), dX2);
    miny = std::min(std::min(dY0, dY1), dY2);
    maxy = std::max(std::max(dY0, dY1), dY2);

    decode.push_back(minx);
    decode.push_back(maxx);
    decode.push_back(miny);
    decode.push_back(maxy);

    decode.push_back(static_cast<int64_t>(0));
    decode.push_back(static_cast<int64_t>(1));
    decode.push_back(static_cast<int64_t>(0));
    decode.push_back(static_cast<int64_t>(1));
    decode.push_back(static_cast<int64_t>(0));
    decode.push_back(static_cast<int64_t>(1));

    PdfObject* shadingObject = this->GetObject().GetDictionary().FindKey("Shading");
    PdfDictionary& shading = shadingObject->GetDictionary();

    shading.AddKey("ColorSpace", PdfName("DeviceRGB"));
    shading.AddKey("BitsPerCoordinate", static_cast<int64_t>(8));
    shading.AddKey("BitsPerComponent", static_cast<int64_t>(8));
    shading.AddKey("BitsPerFlag", static_cast<int64_t>(8));
    shading.AddKey("Decode", decode);

    // f x y c1 c2 c3
    int len = (1 + 1 + 1 + 1 + 1 + 1) * 3;
    char buff[18];

    buff[0] = 0; // flag - start new triangle
    buff[1] = static_cast<char>(255.0 * (dX0 - minx) / (maxx - minx));
    buff[2] = static_cast<char>(255.0 * (dY0 - miny) / (maxy - miny));
    buff[3] = static_cast<char>(255.0 * rgb0.GetRed());
    buff[4] = static_cast<char>(255.0 * rgb0.GetGreen());
    buff[5] = static_cast<char>(255.0 * rgb0.GetBlue());

    buff[6] = 0; // flag - start new triangle
    buff[7] = static_cast<char>(255.0 * (dX1 - minx) / (maxx - minx));
    buff[8] = static_cast<char>(255.0 * (dY1 - miny) / (maxy - miny));
    buff[9] = static_cast<char>(255.0 * rgb1.GetRed());
    buff[10] = static_cast<char>(255.0 * rgb1.GetGreen());
    buff[11] = static_cast<char>(255.0 * rgb1.GetBlue());

    buff[12] = 0; // flag - start new triangle
    buff[13] = static_cast<char>(255.0 * (dX2 - minx) / (maxx - minx));
    buff[14] = static_cast<char>(255.0 * (dY2 - miny) / (maxy - miny));
    buff[15] = static_cast<char>(255.0 * rgb2.GetRed());
    buff[16] = static_cast<char>(255.0 * rgb2.GetGreen());
    buff[17] = static_cast<char>(255.0 * rgb2.GetBlue());

    shadingObject->GetOrCreateStream().Set(buff, len);
}
