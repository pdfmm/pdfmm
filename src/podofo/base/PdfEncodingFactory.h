/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_FACTORY_H
#define PDF_ENCODING_FACTORY_H

#include "PdfEncoding.h"

namespace PoDoFo {

/** This factory creates a PdfEncoding
 *  from an existing object in the PDF.
 */
class PODOFO_API PdfEncodingFactory final
{
public:
    /** Create a new PdfEncoding from either an
     *  encoding name or an encoding dictionary.
     *
     *  \param obj must be a name or an encoding dictionary
     *  \param toUnicode the optional ToUnicode dictionary
     *
     *  \returns a PdfEncoding or nullptr
     */
    static PdfEncoding CreateEncoding(PdfObject& fontObj);

public:
    /** Singleton method which returns a global instance
     *  of PdfDocEncoding.
     *
     *  \returns global instance of PdfDocEncoding
     */
    static PdfEncoding CreateDynamicEncoding();

    /** Singleton method which returns a global instance
     *  of PdfDocEncoding.
     *
     *  \returns global instance of PdfDocEncoding
     */
    static PdfEncoding CreatePdfDocEncoding();

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
     *  of StandardEncoding.
     *
     *  \returns global instance of StandardEncoding
     */
    static PdfEncoding CreateStandardEncoding();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfEncoding CreateMacExpertEncoding();

    /** Singleton method which returns a global instance
     *  of SymbolEncoding.
     *
     *  \returns global instance of SymbolEncoding
     */
    static PdfEncoding CreateSymbolEncoding();

    /** Singleton method which returns a global instance
     *  of ZapfDingbatsEncoding.
     *
     *  \returns global instance of ZapfDingbatsEncoding
     */
    static PdfEncoding CreateZapfDingbatsEncoding();

    /** Singleton method which returns a global instance
     *  of Win1250Encoding.
     *
     *  \returns global instance of Win1250Encoding
     *
     *  \see GlobalWinAnsiEncodingInstance, GlobalIso88592EncodingInstance
     */
    static PdfEncoding CreateWin1250Encoding();

    /** Singleton method which returns a global instance
     *  of Iso88592Encoding.
     *
     *  \returns global instance of Iso88592Encoding
     *
     *  \see GlobalWinAnsiEncodingInstance, GlobalWin1250EncodingInstance
     */
    static PdfEncoding CreateIso88592Encoding();

private:
    static PdfEncodingMapConstPtr createEncodingMap(PdfObject& obj, bool explicitNames);

private:
    PdfEncodingFactory() = delete;
};

}

#endif // PDF_ENCODING_FACTORY_H
