/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfTilingPattern.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfColor.h"
#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"
#include "base/PdfRect.h"
#include "base/PdfStream.h"
#include "base/PdfWriter.h"

#include "PdfFunction.h"
#include "PdfImage.h"

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace PoDoFo;

PdfTilingPattern::PdfTilingPattern(PdfDocument& doc, PdfTilingPatternType eTilingType,
    double strokeR, double strokeG, double strokeB,
    bool doFill, double fillR, double fillG, double fillB,
    double offsetX, double offsetY,
    PdfImage* pImage)
    : PdfElement(doc, "Pattern")
{
    ostringstream out;
    // We probably aren't doing anything locale sensitive here, but it's
    // best to be sure.
    PdfLocaleImbue(out);

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ptrn" << this->GetObject().GetIndirectReference().ObjectNumber();

    m_Identifier = PdfName(out.str().c_str());

    this->Init(eTilingType, strokeR, strokeG, strokeB,
        doFill, fillR, fillG, fillB, offsetX, offsetY, pImage);
}

void PdfTilingPattern::AddToResources(const PdfName& rIdentifier, const PdfReference& rRef, const PdfName& rName)
{
    PdfObject* pResource = GetObject().GetDictionary().GetKey("Resources");

    if (!pResource)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    if (!pResource->GetDictionary().HasKey(rName))
        pResource->GetDictionary().AddKey(rName, PdfDictionary());

    if (EPdfDataType::Reference == pResource->GetDictionary().GetKey(rName)->GetDataType())
    {
        PdfObject* directObject = pResource->GetDocument()->GetObjects().GetObject(pResource->GetDictionary().GetKey(rName)->GetReference());

        if (directObject == nullptr)
            PODOFO_RAISE_ERROR(EPdfError::NoObject);

        if (!directObject->GetDictionary().HasKey(rIdentifier))
            directObject->GetDictionary().AddKey(rIdentifier, rRef);
    }
    else
    {
        if (!pResource->GetDictionary().GetKey(rName)->GetDictionary().HasKey(rIdentifier))
            pResource->GetDictionary().GetKey(rName)->GetDictionary().AddKey(rIdentifier, rRef);
    }
}

void PdfTilingPattern::Init(PdfTilingPatternType eTilingType,
    double strokeR, double strokeG, double strokeB,
    bool doFill, double fillR, double fillG, double fillB,
    double offsetX, double offsetY,
    PdfImage* pImage)
{
    if (eTilingType == PdfTilingPatternType::Image && pImage == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    if (eTilingType != PdfTilingPatternType::Image && pImage != nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    PdfRect rRect;
    rRect.SetLeft(0);
    rRect.SetBottom(0);

    if (pImage)
    {
        rRect.SetWidth(pImage->GetWidth());
        rRect.SetHeight(pImage->GetHeight()); // CHECK-ME: It was -pImage->GetHeight() but that appears to be wrong anyway
    }
    else
    {
        rRect.SetWidth(8);
        rRect.SetHeight(8);
    }

    PdfVariant var;
    rRect.ToVariant(var);

    this->GetObject().GetDictionary().AddKey("PatternType", static_cast<int64_t>(1)); // Tiling pattern
    this->GetObject().GetDictionary().AddKey("PaintType", static_cast<int64_t>(1)); // Colored
    this->GetObject().GetDictionary().AddKey("TilingType", static_cast<int64_t>(1)); // Constant spacing
    this->GetObject().GetDictionary().AddKey("BBox", var);
    this->GetObject().GetDictionary().AddKey("XStep", static_cast<int64_t>(rRect.GetWidth()));
    this->GetObject().GetDictionary().AddKey("YStep", static_cast<int64_t>(rRect.GetHeight()));
    this->GetObject().GetDictionary().AddKey("Resources", PdfObject(PdfDictionary()));

    if (offsetX < -1e-9 || offsetX > 1e-9 || offsetY < -1e-9 || offsetY > 1e-9)
    {
        PdfArray array;

        array.push_back(static_cast<int64_t>(1));
        array.push_back(static_cast<int64_t>(0));
        array.push_back(static_cast<int64_t>(0));
        array.push_back(static_cast<int64_t>(1));
        array.push_back(offsetX);
        array.push_back(offsetY);

        this->GetObject().GetDictionary().AddKey("Matrix", array);
    }

    ostringstream out;
    out.flags(std::ios_base::fixed);
    out.precision(1 /* clPainterDefaultPrecision */);
    PdfLocaleImbue(out);

    if (pImage) {
        AddToResources(pImage->GetIdentifier(), pImage->GetObjectReference(), "XObject");

        out << rRect.GetWidth() << " 0 0 "
            << rRect.GetHeight() << " "
            << rRect.GetLeft() << " "
            << rRect.GetBottom() << " cm" << std::endl;
        out << "/" << pImage->GetIdentifier().GetString() << " Do" << std::endl;
    }
    else
    {
        if (doFill)
        {
            out << fillR << " " << fillG << " " << fillB << " rg" << " ";
            out << rRect.GetLeft() << " " << rRect.GetBottom() << " " << rRect.GetWidth() << " " << rRect.GetHeight() << " re" << " ";
            out << "f" << " "; //fill rect
        }

        out << strokeR << " " << strokeG << " " << strokeB << " RG" << " ";
        out << "2 J" << " "; // line capability style
        out << "0.5 w" << " "; //line width

        double left, bottom, right, top, whalf, hhalf;
        left = rRect.GetLeft();
        bottom = rRect.GetBottom();
        right = left + rRect.GetWidth();
        top = bottom + rRect.GetHeight();
        whalf = rRect.GetWidth() / 2;
        hhalf = rRect.GetHeight() / 2;

        switch (eTilingType)
        {
            case PdfTilingPatternType::BDiagonal:
                out << left << " " << bottom << " m " << right << " " << top << " l ";
                out << left - whalf << " " << top - hhalf << " m " << left + whalf << " " << top + hhalf << " l ";
                out << right - whalf << " " << bottom - hhalf << " m " << right + whalf << " " << bottom + hhalf << " l" << std::endl;
                break;
            case PdfTilingPatternType::Cross:
                out << left << " " << bottom + hhalf << " m " << right << " " << bottom + hhalf << " l ";
                out << left + whalf << " " << bottom << " m " << left + whalf << " " << top << " l" << std::endl;
                break;
            case PdfTilingPatternType::DiagCross:
                out << left << " " << bottom << " m " << right << " " << top << " l ";
                out << left << " " << top << " m " << right << " " << bottom << " l" << std::endl;
                break;
            case PdfTilingPatternType::FDiagonal:
                out << left << " " << top << " m " << right << " " << bottom << " l ";
                out << left - whalf << " " << bottom + hhalf << " m " << left + whalf << " " << bottom - hhalf << " l ";
                out << right - whalf << " " << top + hhalf << " m " << right + whalf << " " << top - hhalf << " l" << std::endl;
                break;
            case PdfTilingPatternType::Horizontal:
                out << left << " " << bottom + hhalf << " m " << right << " " << bottom + hhalf << " l ";
                break;
            case PdfTilingPatternType::Vertical:
                out << left + whalf << " " << bottom << " m " << left + whalf << " " << top << " l" << std::endl;
                break;
            case PdfTilingPatternType::Image:
                /* This is handled above, based on the 'pImage' variable */
            default:
                PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
                break;

        }

        out << "S"; //stroke path
    }

    TVecFilters vecFlate;
    vecFlate.push_back(PdfFilterType::FlateDecode);

    string str = out.str();
    PdfMemoryInputStream stream(str.c_str(), str.length());

    GetObject().GetOrCreateStream().Set(stream, vecFlate);
}
