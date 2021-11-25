/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_ENCODING_MAP_FACTORY_H
#define PDF_ENCODING_MAP_FACTORY_H

#include "PdfEncodingMap.h"

namespace mm {

/** This factory creates a PdfEncodingMap
 */
class PDFMM_API PdfEncodingMapFactory final
{
public:
    /** Singleton method which returns a global instance
     *  of PdfDocEncoding.
     *
     *  \returns global instance of PdfDocEncoding
     */
    static PdfEncodingMapConstPtr PdfDocEncodingInstance();

    /** Singleton method which returns a global instance
     *  of WinAnsiEncoding.
     *
     *  \returns global instance of WinAnsiEncoding
     *
     *  \see Win1250EncodingInstance, Iso88592EncodingInstance
     */
    static PdfEncodingMapConstPtr WinAnsiEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacRomanEncoding.
     *
     *  \returns global instance of MacRomanEncoding
     */
    static PdfEncodingMapConstPtr MacRomanEncodingInstance();

    /** Singleton method which returns a global instance
     *  of StandardEncoding.
     *
     *  \returns global instance of StandardEncoding
     */
    static PdfEncodingMapConstPtr StandardEncodingInstance();

    /** Singleton method which returns a global instance
     *  of MacExpertEncoding.
     *
     *  \returns global instance of MacExpertEncoding
     */
    static PdfEncodingMapConstPtr MacExpertEncodingInstance();

    /** Singleton method which returns a global instance
     *  of SymbolEncoding.
     *
     *  \returns global instance of SymbolEncoding
     */
    static PdfEncodingMapConstPtr SymbolEncodingInstance();

    /** Singleton method which returns a global instance
     *  of ZapfDingbatsEncoding.
     *
     *  \returns global instance of ZapfDingbatsEncoding
     */
    static PdfEncodingMapConstPtr ZapfDingbatsEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Horizontal IndentityEncoding
     *
     *  \returns global instance of Horizontal IdentityEncoding
     */
    static PdfEncodingMapConstPtr TwoBytesHorizontalIdentityEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Vertical IndentityEncoding
     *
     *  \returns global instance of Vertical IdentityEncoding
     */
    static PdfEncodingMapConstPtr TwoBytesVerticalIdentityEncodingInstance();

    /** Singleton method which returns a global instance
     *  of Win1250Encoding.
     *
     *  \returns global instance of Win1250Encoding
     *
     *  \see WinAnsiEncodingInstance, Iso88592EncodingInstance
     */
    static PdfEncodingMapConstPtr Win1250EncodingInstance();

    /** Singleton method which returns a global instance
     *  of Iso88592Encoding.
     *
     *  \returns global instance of Iso88592Encoding
     *
     *  \see WinAnsiEncodingInstance, Win1250EncodingInstance
     */
    static PdfEncodingMapConstPtr Iso88592EncodingInstance();

    static PdfEncodingMapConstPtr GetNullEncodingMap();

    /** Return the encoding map for the given standard font type or nullptr for unknown
     */
    static PdfEncodingMapConstPtr GetStandard14FontEncodingMap(PdfStandard14FontType stdFont);

private:
    PdfEncodingMapFactory() = delete;
};

}

#endif // PDF_ENCODING_MAP_FACTORY_H
