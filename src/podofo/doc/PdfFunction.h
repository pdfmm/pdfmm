/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_FUNCTION_H
#define PDF_FUNCTION_H

#include "podofo/base/PdfDefines.h"
#include "PdfElement.h"

#include <list>

namespace PoDoFo {

class PdfArray;

/**
 * The function type of a mathematical function in a PDF file.
 */
enum class EPdfFunctionType
{
    Sampled = 0, ///< A sampled function (Type1)
    Exponential = 2, ///< An exponential interpolation function (Type2)
    Stitching = 3, ///< A stitching function (Type3)
    PostScript = 4  ///< A PostScript calculator function (Type4)
};

/**
 * This class defines a PdfFunction.
 * A function can be used in various ways in a PDF file.
 * Examples are device dependent rasterization for high quality
 * printing or color transformation functions for certain colorspaces.
 */
class PODOFO_DOC_API PdfFunction : public PdfElement
{
public:

    typedef std::list<PdfFunction> List;
    typedef std::list<char> Sample;

protected:
    /** Create a new PdfFunction object.
     *
     *  \param eType the function type
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param pParent parent document
     *
     */
    PdfFunction(PdfDocument& doc, EPdfFunctionType eType, const PdfArray& rDomain);

private:
    /** Initialize this object.
     *
     *  \param eType the function type
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     */
    void Init(EPdfFunctionType eType, const PdfArray& rDomain);

};

/** This class is a PdfSampledFunction.
 */
class PODOFO_DOC_API PdfSampledFunction : public PdfFunction
{
public:
    /** Create a new PdfSampledFunction object.
     *
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rRange  this array describes the output parameters of this PdfFunction. If this
     *                 function has n input parameters, this array has to contain 2*n numbers
     *                 where each number describes either the lower or upper boundary of the output range.
     *  \param rlstSamples a list of bytes which are used to build up this function sample data
     *  \param pParent parent document
     */
    PdfSampledFunction(PdfDocument& doc, const PdfArray& rDomain, const PdfArray& rRange, const PdfFunction::Sample& rlstSamples);

private:
    /** Initialize this object.
     */
    void Init(const PdfArray& rDomain, const PdfArray& rRange, const PdfFunction::Sample& rlstSamples);

};

/** This class is a PdfExponentialFunction.
 */
class PODOFO_DOC_API PdfExponentialFunction : public PdfFunction
{
public:
    /** Create a new PdfExponentialFunction object.
     *
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rC0
     *  \param rC1
     *  \param dExponent
     *  \param pParent parent document
     */
    PdfExponentialFunction(PdfDocument& doc, const PdfArray& rDomain, const PdfArray& rC0, const PdfArray& rC1, double dExponent);

private:
    /** Initialize this object.
     */
    void Init(const PdfArray& rC0, const PdfArray& rC1, double dExponent);

};

/** This class is a PdfStitchingFunction, i.e. a PdfFunction that combines
 *  more than one PdfFunction into one.
 *
 *  It combines several PdfFunctions that take 1 input parameter to
 *  a new PdfFunction taking again only 1 input parameter.
 */
class PODOFO_DOC_API PdfStitchingFunction : public PdfFunction
{
public:
    /** Create a new PdfStitchingFunction object.
     *
     *  \param rlstFunctions a list of functions which are used to built up this function object
     *  \param rDomain this array describes the input parameters of this PdfFunction. If this
     *                 function has m input parameters, this array has to contain 2*m numbers
     *                 where each number describes either the lower or upper boundary of the input range.
     *  \param rBounds the bounds array
     *  \param rEncode the encode array
     *  \param pParent parent document
     *
     */
    PdfStitchingFunction(PdfDocument& doc, const PdfFunction::List& rlstFunctions, const PdfArray& rDomain, const PdfArray& rBounds, const PdfArray& rEncode);

private:
    /** Initialize this object.
     *
     *  \param rlstFunctions a list of functions which are used to built up this function object
     *  \param rBounds the bounds array
     *  \param rEncode the encode array
     */
    void Init(const PdfFunction::List& rlstFunctions, const PdfArray& rBounds, const PdfArray& rEncode);

};

};

#endif // PDF_FUNCTION_H
