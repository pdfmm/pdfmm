/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfCanvas.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfColor.h"
#include "PdfStream.h"

using namespace std;
using namespace mm;

PdfArray PdfCanvas::GetProcSet()
{
    PdfArray procset;
    procset.push_back(PdfName("PDF"));
    procset.push_back(PdfName("Text"));
    procset.push_back(PdfName("ImageB"));
    procset.push_back(PdfName("ImageC"));
    procset.push_back(PdfName("ImageI"));
    return procset;
}

void PdfCanvas::AddColorResource(const PdfColor& color)
{
    auto& resources = GetResources();
    switch (color.GetColorSpace())
    {
        case PdfColorSpace::Separation:
        {
            string csPrefix("ColorSpace");
            string csName = color.GetName();
            string temp(csPrefix + csName);

            if (!resources.GetDictionary().HasKey("ColorSpace")
                || !resources.GetDictionary().MustFindKey("ColorSpace").GetDictionary().HasKey(csPrefix + csName))
            {
                // Build color-spaces for separation
                PdfObject* csp = color.BuildColorSpace(*GetContents().GetDocument());

                AddResource(csPrefix + csName, csp->GetIndirectReference(), "ColorSpace");
            }
        }
        break;

        case PdfColorSpace::CieLab:
        {
            if (!resources.GetDictionary().HasKey("ColorSpace")
                || !resources.GetDictionary().MustFindKey("ColorSpace").GetDictionary().HasKey("ColorSpaceLab"))
            {
                // Build color-spaces for CIE-lab
                PdfObject* csp = color.BuildColorSpace(*GetContents().GetDocument());

                AddResource("ColorSpaceCieLab", csp->GetIndirectReference(), "ColorSpace");
            }
        }
        break;

        case PdfColorSpace::DeviceGray:
        case PdfColorSpace::DeviceRGB:
        case PdfColorSpace::DeviceCMYK:
        case PdfColorSpace::Indexed:
            // No colorspace needed
        case PdfColorSpace::Unknown:
        default:
            break;
    }
}

void PdfCanvas::AddResource(const PdfName& identifier, const PdfReference& ref, const PdfName& name)
{
    if (name.GetLength() == 0 || identifier.GetLength() == 0)
        PDFMM_RAISE_ERROR(PdfErrorCode::InvalidHandle);

    auto& resources = this->GetResources();
    if (!resources.GetDictionary().HasKey(name))
        resources.GetDictionary().AddKey(name, PdfDictionary());

    if (resources.GetDictionary().GetKey(name)->GetDataType() == PdfDataType::Reference)
    {
        auto directObject = resources.GetDocument()->GetObjects().GetObject(resources.GetDictionary().GetKey(name)->GetReference());
        if (directObject == nullptr)
            PDFMM_RAISE_ERROR(PdfErrorCode::NoObject);

        if (!directObject->GetDictionary().HasKey(identifier))
            directObject->GetDictionary().AddKey(identifier, ref);
    }
    else
    {
        if (!resources.GetDictionary().GetKey(name)->GetDictionary().HasKey(identifier))
            resources.GetDictionary().GetKey(name)->GetDictionary().AddKey(identifier, ref);
    }
}
