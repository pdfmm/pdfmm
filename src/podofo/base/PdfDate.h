/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_DATE_H
#define PDF_DATE_H

#include <chrono>

#include "PdfDefines.h"
#include "PdfString.h"

namespace PoDoFo {

/** This class is a date datatype as specified in the PDF 
 *  reference. You can easily convert from Unix time_t to
 *  the PDF time representation and back. Dates like these
 *  are used for example in the PDF info dictionary for the
 *  creation time and date of the PDF file.
 *
 *  PdfDate objects are immutable.
 *
 *  From the PDF reference:
 *
 *  PDF defines a standard date format, which closely follows 
 *  that of the international standard ASN.1 (Abstract Syntax
 *  Notation One), defined in ISO/IEC 8824 (see the Bibliography). 
 *  A date is a string of the form
 *  (D:YYYYMMDDHHmmSSOHH'mm')
 */
class PODOFO_API PdfDate final
{
public:
    /** Create a PdfDate object with the current date and time.
     */
    PdfDate();

    /** Create a PdfDate with a specified date and time
     *  \param t the date and time of this object
     *
     *  \see IsValid()
     */
    PdfDate(const std::chrono::seconds &secondsFromEpoch, const std::optional<std::chrono::minutes> &offsetFromUTC);

    /** Create a PdfDate with a specified date and time
     *  \param szDate the date and time of this object 
     *         in PDF format. It has to be a string of 
     *         the format  (D:YYYYMMDDHHmmSSOHH'mm').
     */
    PdfDate( const PdfString & sDate );

    /** \returns the date and time of this PdfDate in 
     *  seconds since epoch.
     */
    const std::chrono::seconds & GetSecondsFromEpoch() const { return m_secondsFromEpoch; }

    const std::optional<std::chrono::minutes> & GetMinutesFromUtc() const { return m_minutesFromUtc; }

    /** The value returned by this function can be used in any PdfObject
     *  where a date is needed
     */
    PdfString ToString() const;

    /** The value returned is a W3C compliant date representation
     */
    PdfString ToStringW3C() const;

private:
    /** Creates the internal string representation from
     *  a time_t value and writes it to m_szDate.
     */
    PdfString createStringRepresentation(bool w3cstring) const;

    /** Parse fixed length number from string
     *  \param in string to read number from
     *  \param length of number to read 
     *  \param min minimal value of number
     *  \param max maximal value of number
     *  \param ret parsed number
     */
    bool ParseFixLenNumber(const char *&in, unsigned length, int min, int max, int &ret);

private:
    std::chrono::seconds m_secondsFromEpoch;
    std::optional<std::chrono::minutes> m_minutesFromUtc;
};

};

#endif // PDF_DATE_H
