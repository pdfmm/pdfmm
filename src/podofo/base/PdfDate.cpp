/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfDate.h"
#include "PdfDefinesPrivate.h"

#include <algorithm>
#include <date/date.h>

using namespace PoDoFo;
namespace chrono = std::chrono;

#if WIN32
#define timegm _mkgmtime
#endif

// a PDF date has a maximum of 24 bytes incuding the terminating \0
#define PDF_DATE_BUFFER_SIZE 24

// a W3C date has a maximum of 26 bytes incuding the terminating \0
#define W3C_DATE_BUFFER_SIZE 26

#define PEEK_DATE_CHAR(str, zoneShift) if (*str == '\0')\
{\
    goto End;\
}\
else if (tryReadShiftChar(*str, zoneShift))\
{\
    str++;\
    goto ParseShift;\
}

static int getLocalOffesetFromUTCMinutes();
static bool tryReadShiftChar(char ch, int &zoneShift);

PdfDate::PdfDate()
{
    m_minutesFromUtc = chrono::minutes(getLocalOffesetFromUTCMinutes());
    // Cast now() to seconds. We assume system_clock epoch is
    //  always 1970/1/1 UTC in all platforms, like in C++20
    auto now = chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now());
    // We forget about realtionship with UTC, convert to local seconds
    m_secondsFromEpoch = chrono::seconds(now.time_since_epoch());
}

PdfDate::PdfDate(const chrono::seconds &secondsFromEpoch, const std::optional<chrono::minutes> &offsetFromUTC)
    : m_secondsFromEpoch(secondsFromEpoch), m_minutesFromUtc(offsetFromUTC)
{
}

PdfDate::PdfDate(const PdfString &sDate)
{
    if (!sDate.IsValid())
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Date is invalid");

    std::string date = sDate.GetStringUtf8();

    int y = 0;
    int m = 0;
    int d = 0;
    int h = 0;
    int M = 0;
    int s = 0;

    bool hasZoneShift = false;
    int nZoneShift = 0;
    int nZoneHour = 0;
    int nZoneMin = 0;

    const char * pszDate = date.data();
    if (pszDate == nullptr)
        goto Error;

    if (*pszDate == 'D')
    {
        pszDate++;
        if (*pszDate++ != ':')
            goto Error;
    }

    PEEK_DATE_CHAR(pszDate, nZoneShift);

    if (!ParseFixLenNumber(pszDate,4,0,9999, y))
        return;

    PEEK_DATE_CHAR(pszDate, nZoneShift);

    if (!ParseFixLenNumber(pszDate,2,1,12, m))
        goto Error;

    PEEK_DATE_CHAR(pszDate, nZoneShift);

    if (!ParseFixLenNumber(pszDate,2,1,31, d))
        goto Error;

    PEEK_DATE_CHAR(pszDate, nZoneShift);

    if (!ParseFixLenNumber(pszDate,2,0,23, h))
        goto Error;

    PEEK_DATE_CHAR(pszDate, nZoneShift);

    if (!ParseFixLenNumber(pszDate,2,0,59, M))
        goto Error;

    PEEK_DATE_CHAR(pszDate, nZoneShift);

    if (!ParseFixLenNumber(pszDate,2,0,59, s))
        goto Error;

    PEEK_DATE_CHAR(pszDate, nZoneShift);

ParseShift:
    hasZoneShift = true;
    if (nZoneShift != 0)
    {
        if (!ParseFixLenNumber(pszDate, 2, 0, 59, nZoneHour))
            goto Error;

        if (*pszDate == '\'')
        {
            pszDate++;
            if (!ParseFixLenNumber(pszDate, 2, 0, 59, nZoneMin))
                goto Error;
            if (*pszDate != '\'') return;
            pszDate++;
        }
    }

    if (*pszDate != '\0')
        goto Error;
End:
    m_secondsFromEpoch = (date::local_days(date::year(y) / m / d) + chrono::hours(h) + chrono::minutes(M) + chrono::seconds(s)).time_since_epoch();
    if (hasZoneShift)
    {
        m_minutesFromUtc = nZoneShift * (chrono::hours(nZoneHour) + chrono::minutes(nZoneMin));
        m_secondsFromEpoch -= *m_minutesFromUtc;
    }

    return;
Error:
    PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Date is invalid");
}

PdfString PdfDate::createStringRepresentation(bool w3cstring) const
{
    std::string date;
    if (w3cstring)
        date.resize(W3C_DATE_BUFFER_SIZE);
    else
        date.resize(PDF_DATE_BUFFER_SIZE);

    auto secondsFromEpoch = m_secondsFromEpoch;

    std::string offset;
    unsigned y;
    unsigned m;
    unsigned d;
    unsigned h;
    unsigned M;
    unsigned s;
    if (m_minutesFromUtc.has_value())
    {
        auto minutesFromUtc = *m_minutesFromUtc;
        int minutesFromUtci = (int)minutesFromUtc.count();
        unsigned offseth = std::abs(minutesFromUtci) / 60;
        unsigned offsetm = std::abs(minutesFromUtci) % 60;
        if (minutesFromUtci == 0)
        {
            offset = "Z";
        }
        else
        {
            bool plus = minutesFromUtci > 0 ? true : false;
            if (w3cstring)
            {
                offset.resize(6);
                snprintf(const_cast<char *>(offset.data()), offset.size() + 1, "%s%02u:%02u", plus ? "+" : "-", offseth, offsetm);
            }
            else
            {
                offset.resize(7);
                snprintf(const_cast<char *>(offset.data()), offset.size() + 1, "%s%02u'%02u'", plus ? "+" : "-", offseth, offsetm);
            }
        }

        // Assume sys time
        auto secondsFromEpoch = (date::sys_seconds)(m_secondsFromEpoch + minutesFromUtc);
        auto dp = date::floor<date::days>(secondsFromEpoch);
        date::year_month_day ymd{ dp };
        date::hh_mm_ss<chrono::seconds> time{ chrono::floor<chrono::seconds>(secondsFromEpoch - dp) };
        y = (int)ymd.year();
        m = (unsigned)ymd.month();
        d = (unsigned)ymd.day();
        h = (unsigned)time.hours().count();
        M = (unsigned)time.minutes().count();
        s = (unsigned)time.seconds().count();
    }
    else
    {
        // Assume local time
        auto secondsFromEpoch = (date::local_seconds)m_secondsFromEpoch;
        auto dp = date::floor<date::days>(secondsFromEpoch);
        date::year_month_day ymd{ dp };
        date::hh_mm_ss<chrono::seconds> time{ chrono::floor<chrono::seconds>(secondsFromEpoch - dp) };
        y = (int)ymd.year();
        m = (unsigned)ymd.month();
        d = (unsigned)ymd.day();
        h = (unsigned)time.hours().count();
        M = (unsigned)time.minutes().count();
        s = (unsigned)time.seconds().count();
    }

    if (w3cstring)
    {
        // e.g. "1998-12-23T19:52:07-08:00"
        snprintf(date.data(), W3C_DATE_BUFFER_SIZE, "%04u-%02u-%02uT%02u:%02u:%02u%s", y, m, d, h, M, s, offset.data());
    }
    else
    {
        // e.g. "D:19981223195207−08'00'"
        snprintf(date.data(), PDF_DATE_BUFFER_SIZE, "D:%04u%02u%02u%02u%02u%02u%s", y, m, d, h, M, s, offset.data());
    }

    return PdfString(date.data());
}


bool PdfDate::ParseFixLenNumber(const char *&in, unsigned int length, int min, int max, int &ret)
{
    ret = 0;
    for(unsigned int i=0;i<length;i++)
    {
        if ( in == NULL || !isdigit(*in)) return false;
        ret = ret*10+ (*in-'0');
        in++;
    }
    if ( ret < min || ret > max ) return false;
    return true;
}

PdfString PdfDate::ToString() const
{
    return createStringRepresentation(false);
}

PdfString PdfDate::ToStringW3C() const
{
    return createStringRepresentation(true);
}

// degski, https://stackoverflow.com/a/57520245/213871
// We use this function because we don't have C++20
// date library runtime support
int getLocalOffesetFromUTCMinutes()
{
    time_t t = time(NULL);
    struct tm * locg = localtime(&t);
    struct tm locl;
    memcpy(&locl, locg, sizeof(struct tm));
    return (int)(timegm(locg) - mktime(&locl)) / 60;
}

bool tryReadShiftChar(char ch, int &zoneShift)
{
    switch (ch)
    {
        case '+':
            zoneShift = 1;
            return true;
        case '-':
            zoneShift = -1;
            return true;
        case 'Z':
            zoneShift = 0;
            return true;
        default:
            return false;
    }
}
