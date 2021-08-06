/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <podofo/private/PdfDefinesPrivate.h>
#include "PdfFunction.h"

#include <podofo/base/PdfArray.h>
#include <podofo/base/PdfDictionary.h>
#include <podofo/base/PdfStream.h>

using namespace PoDoFo;

PdfFunction::PdfFunction(PdfDocument& doc, PdfFunctionType functionType, const PdfArray& domain)
    : PdfElement(doc)
{
    Init(functionType, domain);
}

void PdfFunction::Init(PdfFunctionType functionType, const PdfArray& domain)
{
    this->GetObject().GetDictionary().AddKey("FunctionType", static_cast<int64_t>(functionType));
    this->GetObject().GetDictionary().AddKey("Domain", domain);

}

PdfSampledFunction::PdfSampledFunction(PdfDocument& doc, const PdfArray& domain, const PdfArray& range, const PdfFunction::Sample& samples)
    : PdfFunction(doc, PdfFunctionType::Sampled, domain)
{
    Init(domain, range, samples);
}

void PdfSampledFunction::Init(const PdfArray& domain, const PdfArray& range, const PdfFunction::Sample& samples)
{
    PdfArray Size;
    for (unsigned i = 0; i < domain.GetSize() / 2; i++)
        Size.push_back(PdfObject(static_cast<int64_t>(domain.GetSize() / 2)));

    this->GetObject().GetDictionary().AddKey("Domain", domain);
    this->GetObject().GetDictionary().AddKey("Range", range);
    this->GetObject().GetDictionary().AddKey("Size", Size);
    this->GetObject().GetDictionary().AddKey("Order", PdfObject(static_cast<int64_t>(1)));
    this->GetObject().GetDictionary().AddKey("BitsPerSample", PdfObject(static_cast<int64_t>(8)));

    this->GetObject().GetOrCreateStream().BeginAppend();
    for (char c : samples)
        this->GetObject().GetOrCreateStream().Append(&c, 1);

    this->GetObject().GetOrCreateStream().EndAppend();
}

PdfExponentialFunction::PdfExponentialFunction(PdfDocument& doc, const PdfArray& domain, const PdfArray& c0, const PdfArray& c1, double exponent)
    : PdfFunction(doc, PdfFunctionType::Exponential, domain)
{
    Init(c0, c1, exponent);
}

void PdfExponentialFunction::Init(const PdfArray& c0, const PdfArray& c1, double exponent)
{
    this->GetObject().GetDictionary().AddKey("C0", c0);
    this->GetObject().GetDictionary().AddKey("C1", c1);
    this->GetObject().GetDictionary().AddKey("N", exponent);
}

PdfStitchingFunction::PdfStitchingFunction(PdfDocument& doc, const PdfFunction::List& functions, const PdfArray& domain, const PdfArray& bounds, const PdfArray& encode)
    : PdfFunction(doc, PdfFunctionType::Stitching, domain)
{
    Init(functions, bounds, encode);
}

void PdfStitchingFunction::Init(const PdfFunction::List& functions, const PdfArray& bounds, const PdfArray& encode)
{
    PdfArray functionsArr;

    functionsArr.reserve(functions.size());

    for (auto& fun : functions)
        functionsArr.push_back(fun.GetObject().GetIndirectReference());

    this->GetObject().GetDictionary().AddKey("Functions", functionsArr);
    this->GetObject().GetDictionary().AddKey("Bounds", bounds);
    this->GetObject().GetDictionary().AddKey("Encode", encode);
}
