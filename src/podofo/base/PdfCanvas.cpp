/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfCanvas.h"

#include <doc/PdfDocument.h>
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfColor.h"
#include "PdfStream.h"

using namespace std;
using namespace PoDoFo;

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

void PdfCanvas::AddColorResource(const PdfColor& rColor)
{
    auto& resources = GetResources();
    switch (rColor.GetColorSpace())
    {
        case PdfColorSpace::Separation:
        {
            string csPrefix("ColorSpace");
            string csName = rColor.GetName();
            string temp(csPrefix + csName);

            if (!resources.GetDictionary().HasKey("ColorSpace")
                || !resources.GetDictionary().GetKey("ColorSpace")->GetDictionary().HasKey(csPrefix + csName))
            {
                // Build color-spaces for separation
                PdfObject* csp = rColor.BuildColorSpace(*GetContents().GetDocument());

                AddResource(csPrefix + csName, csp->GetIndirectReference(), PdfName("ColorSpace"));
            }
        }
        break;

        case PdfColorSpace::CieLab:
        {
            if (!resources.GetDictionary().HasKey("ColorSpace")
                || !resources.GetDictionary().GetKey("ColorSpace")->GetDictionary().HasKey("ColorSpaceLab"))
            {
                // Build color-spaces for CIE-lab
                PdfObject* csp = rColor.BuildColorSpace(*GetContents().GetDocument());

                AddResource("ColorSpaceCieLab", csp->GetIndirectReference(), PdfName("ColorSpace"));
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

void PdfCanvas::AddResource(const PdfName& rIdentifier, const PdfReference& rRef, const PdfName& rName)
{
    if (rName.GetLength() == 0 || rIdentifier.GetLength() == 0)
    {
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);
    }

    auto& resources = this->GetResources();
    if (!resources.GetDictionary().HasKey(rName))
        resources.GetDictionary().AddKey(rName, PdfDictionary());

    // Peter Petrov: 18 December 2008. Bug fix
    if (EPdfDataType::Reference == resources.GetDictionary().GetKey(rName)->GetDataType())
    {
        PdfObject* directObject = resources.GetDocument()->GetObjects().GetObject(resources.GetDictionary().GetKey(rName)->GetReference());

        if (directObject == nullptr)
        {
            PODOFO_RAISE_ERROR(EPdfError::NoObject);
        }

        if (!directObject->GetDictionary().HasKey(rIdentifier))
            directObject->GetDictionary().AddKey(rIdentifier, rRef);
    }
    else
    {
        if (!resources.GetDictionary().GetKey(rName)->GetDictionary().HasKey(rIdentifier))
            resources.GetDictionary().GetKey(rName)->GetDictionary().AddKey(rIdentifier, rRef);
    }
}
