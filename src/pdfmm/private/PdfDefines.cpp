/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include <utfcpp/utf8.h>

#ifndef WIN32
// NOTE: There's no <cstrings>, <strings.h> is a posix header
#include <strings.h>
#endif

using namespace std;
using namespace mm;

int compat::vsnprintf(char* buffer, size_t count, const char* format, va_list argptr)
{
    auto old = locale::global(locale::classic());
    int ret = std::vsnprintf(buffer, count, format, argptr);
    locale::global(old);
    return ret;
}

int compat::strcasecmp(const char* s1, const char* s2)
{
#if defined(WIN32)
    return ::_stricmp(s1, s2);
#else
    return ::strcasecmp(s1, s2);
#endif
}

int compat::strncasecmp(const char* s1, const char* s2, size_t n)
{
#if defined(_MSC_VER)
    return ::_strnicmp(s1, s2, n);
#else
    // POSIX.1-2001
    return ::strncasecmp(s1, s2, n);
#endif
}

size_t io::FileSize(const string_view& filename)
{
    streampos fbegin;

    auto stream = io::open_ifstream(filename, ios_base::in | ios_base::binary);
    if (stream.fail())
        goto Error;

    fbegin = stream.tellg();   // The file pointer is currently at the beginning
    if (stream.fail())
        goto Error;

    stream.seekg(0, ios::end);      // Place the file pointer at the end of file
    if (stream.fail())
        goto Error;

    return (size_t)(streamoff)(stream.tellg() - fbegin);
Error:
    PDFMM_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to read file size");
}

// Read from stream an amount of bytes or less
// without setting failbit
// https://stackoverflow.com/a/22593639/213871
size_t io::Read(istream& stream, char* buffer, size_t count)
{
    if (count == 0 || stream.eof())
        return 0;

    size_t offset = 0;
    size_t reads;
    do
    {
        // This consistently fails on gcc (linux) 4.8.1 with failbit set on read
        // failure. This apparently never fails on VS2010 and VS2013 (Windows 7)
        reads = (size_t)stream.rdbuf()->sgetn(buffer + offset, (streamsize)count);

        // This rarely sets failbit on VS2010 and VS2013 (Windows 7) on read
        // failure of the previous sgetn()
        (void)stream.rdstate();

        // On gcc (linux) 4.8.1 and VS2010/VS2013 (Windows 7) this consistently
        // sets eofbit when stream is EOF for the conseguences  of sgetn(). It
        // should also throw if exceptions are set, or return on the contrary,
        // and previous rdstate() restored a failbit on Windows. On Windows most
        // of the times it sets eofbit even on real read failure
        (void)stream.peek();

        if (stream.fail())
            PDFMM_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Stream I/O error while reading");

        offset += reads;
        count -= reads;
    } while (count != 0 && !stream.eof());

    return offset;
}

FILE* io::fopen(const string_view& filename, const string_view& mode)
{
#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    auto mode16 = utf8::utf8to16((string)mode);
    return _wfopen((wchar_t*)filename16.c_str(), (wchar_t*)mode16.c_str());
#else
    return std::fopen(filename.data(), mode.data());
#endif
}

ifstream io::open_ifstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return ifstream((wchar_t*)filename16.c_str(), mode);
#else
    return ifstream((string)filename, mode);
#endif
}

ofstream io::open_ofstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return ofstream((wchar_t*)filename16.c_str(), mode);
#else
    return ofstream((string)filename, mode);
#endif
}

fstream io::open_fstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return fstream((wchar_t*)filename16.c_str(), mode);
#else
    return fstream((string)filename, mode);
#endif
}

void usr::WriteCharHexTo(char buf[2], char ch)
{
    buf[0] = (ch & 0xF0) >> 4;
    buf[0] += (buf[0] > 9 ? 'A' - 10 : '0');

    buf[1] = (ch & 0x0F);
    buf[1] += (buf[1] > 9 ? 'A' - 10 : '0');
}

// Append the char to the supplied string as hexadecimal code
void usr::WriteCharHexTo(string& str, char ch, bool clear)
{
    if (clear)
    {
        str.resize(2);
        WriteCharHexTo(str.data(), ch);
    }
    else
    {
        size_t initialLen = str.length();
        str.resize(initialLen + 2);
        WriteCharHexTo(str.data() + initialLen, ch);
    }
}

void usr::WriteToUtf16BE(u16string& str, char32_t codePoint, bool clear)
{
    // FIX-ME: This is very inefficient. We should improve
    // utfcpp to avoit conversion to utf8 first
    string u8str;
    utf8::append(codePoint, u8str);
    u16string u16str = utf8::utf8to16(u8str, utf8::endianess::big_endian);
    if (clear)
        str = std::move(u16str);
    else
        str.append(u16str.data(), u16str.length());
}

unsigned char usr::GetCharCodeSize(unsigned ch)
{
    return (unsigned char)(std::log(ch) / std::log(256)) + 1;
}

unsigned usr::GetCharCodeMaxValue(unsigned char codeSize)
{
    return (unsigned)(std::pow(2, codeSize * CHAR_BIT)) - 1;
}
