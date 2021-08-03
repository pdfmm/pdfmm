/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfFunction.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfStream.h"

using namespace PoDoFo;

PdfFunction::PdfFunction(PdfDocument& doc, EPdfFunctionType eType, const PdfArray& rDomain)
    : PdfElement(doc)
{
    Init(eType, rDomain);
}

void PdfFunction::Init(EPdfFunctionType eType, const PdfArray& rDomain)
{
    this->GetObject().GetDictionary().AddKey("FunctionType", static_cast<int64_t>(eType));
    this->GetObject().GetDictionary().AddKey("Domain", rDomain);

}

PdfSampledFunction::PdfSampledFunction(PdfDocument& doc, const PdfArray& rDomain, const PdfArray& rRange, const PdfFunction::Sample& rlstSamples)
    : PdfFunction(doc, EPdfFunctionType::Sampled, rDomain)
{
    Init(rDomain, rRange, rlstSamples);
}

void PdfSampledFunction::Init(const PdfArray& rDomain, const PdfArray& rRange, const PdfFunction::Sample& rlstSamples)
{
    PdfArray Size;
    for (unsigned i = 0; i < rDomain.GetSize() / 2; i++)
        Size.push_back(PdfObject(static_cast<int64_t>(rDomain.GetSize() / 2)));

    this->GetObject().GetDictionary().AddKey("Domain", rDomain);
    this->GetObject().GetDictionary().AddKey("Range", rRange);
    this->GetObject().GetDictionary().AddKey("Size", Size);
    this->GetObject().GetDictionary().AddKey("Order", PdfObject(static_cast<int64_t>(1)));
    this->GetObject().GetDictionary().AddKey("BitsPerSample", PdfObject(static_cast<int64_t>(8)));

    this->GetObject().GetOrCreateStream().BeginAppend();
    PdfFunction::Sample::const_iterator it = rlstSamples.begin();
    while (it != rlstSamples.end())
    {
        this->GetObject().GetOrCreateStream().Append(&(*it), 1);
        ++it;
    }
    this->GetObject().GetOrCreateStream().EndAppend();
}

PdfExponentialFunction::PdfExponentialFunction(PdfDocument& doc, const PdfArray& rDomain, const PdfArray& rC0, const PdfArray& rC1, double dExponent)
    : PdfFunction(doc, EPdfFunctionType::Exponential, rDomain)
{
    Init(rC0, rC1, dExponent);
}

void PdfExponentialFunction::Init(const PdfArray& rC0, const PdfArray& rC1, double dExponent)
{
    this->GetObject().GetDictionary().AddKey("C0", rC0);
    this->GetObject().GetDictionary().AddKey("C1", rC1);
    this->GetObject().GetDictionary().AddKey("N", dExponent);
}

PdfStitchingFunction::PdfStitchingFunction(PdfDocument& doc, const PdfFunction::List& rlstFunctions, const PdfArray& rDomain, const PdfArray& rBounds, const PdfArray& rEncode)
    : PdfFunction(doc, EPdfFunctionType::Stitching, rDomain)
{
    Init(rlstFunctions, rBounds, rEncode);
}

void PdfStitchingFunction::Init(const PdfFunction::List& rlstFunctions, const PdfArray& rBounds, const PdfArray& rEncode)
{
    PdfArray functions;
    PdfFunction::List::const_iterator it = rlstFunctions.begin();

    functions.reserve(rlstFunctions.size());

    while (it != rlstFunctions.end())
    {
        functions.push_back(it->GetObject().GetIndirectReference());
        ++it;
    }

    this->GetObject().GetDictionary().AddKey("Functions", functions);
    this->GetObject().GetDictionary().AddKey("Bounds", rBounds);
    this->GetObject().GetDictionary().AddKey("Encode", rEncode);
}
