/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_FACTORY_H
#define PDF_ENCODING_FACTORY_H

#include "PdfEncoding.h"

namespace mm {

class PdfFontMetrics;

/** This factory creates a PdfEncoding
 *  from an existing object in the PDF.
 */
class PDFMM_API PdfEncodingFactory final
{
public:
    /** Create a new PdfEncoding from either an
     *  encoding name or an encoding dictionary.
     *
     *  \param fontObj font object
     *  \param toUnicode the optional ToUnicode dictionary
     *
     *  \returns a PdfEncoding or nullptr
     */
    static PdfEncoding CreateEncoding(
        const PdfObject& fontObj, const PdfFontMetrics& metrics);

public:
    /** Singleton method which returns a global instance
     *  of WinAnsiEncoding.
     *
     *  \returns global instance of WinAnsiEncoding
     *
     *  \see GlobalWin1250EncodingInstance, GlobalIso88592EncodingInstance
     */
    static PdfEncoding CreateWinAnsiEncoding();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     *  \returns global instance of MacRomanEncoding
     */
    static PdfEncoding CreateMacRomanEncoding();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfEncoding CreateMacExpertEncoding();

private:
    static PdfEncodingMapConstPtr createEncodingMap(
        const PdfObject& obj, const PdfFontMetrics& metrics);

private:
    PdfEncodingFactory() = delete;
};

}

#endif // PDF_ENCODING_FACTORY_H
