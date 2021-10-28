/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include <utfcpp/utf8.h>

#include <pdfmm/base/PdfInputDevice.h>
#include <pdfmm/base/PdfOutputDevice.h>

#ifdef _WIN32
#include <pdfmm/common/WindowsLeanMean.h>
#else
 // NOTE: There's no <cstrings>, <strings.h> is a posix header
#include <strings.h>
#endif

using namespace std;
using namespace mm;

int compat::strcasecmp(const char* s1, const char* s2)
{
#if defined(_WIN32)
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

const char* utls::TypeNameForIndex(unsigned index, const char** types, unsigned len)
{
    return index < len ? types[index] : nullptr;
}

int utls::TypeNameToIndex(const char* type, const char** types, unsigned len, int unknownValue)
{
    if (type == nullptr)
        return unknownValue;

    for (unsigned i = 0; i < len; i++)
    {
        if (types[i] != nullptr && strcmp(type, types[i]) == 0)
            return i;
    }

    return unknownValue;
}

size_t utls::FileSize(const string_view& filename)
{
    streampos fbegin;

    auto stream = utls::open_ifstream(filename, ios_base::in | ios_base::binary);
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
    PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Failed to read file size");
}

// Read from stream an amount of bytes or less
// without setting failbit
// https://stackoverflow.com/a/22593639/213871
size_t utls::Read(istream& stream, char* buffer, size_t count)
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
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Stream I/O error while reading");

        offset += reads;
        count -= reads;
    } while (count != 0 && !stream.eof());

    return offset;
}

FILE* utls::fopen(const string_view& filename, const string_view& mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    auto mode16 = utf8::utf8to16((string)mode);
    return _wfopen((wchar_t*)filename16.c_str(), (wchar_t*)mode16.c_str());
#else
    return std::fopen(filename.data(), mode.data());
#endif
}

ifstream utls::open_ifstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return ifstream((wchar_t*)filename16.c_str(), mode);
#else
    return ifstream((string)filename, mode);
#endif
}

ofstream utls::open_ofstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return ofstream((wchar_t*)filename16.c_str(), mode);
#else
    return ofstream((string)filename, mode);
#endif
}

fstream utls::open_fstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef _WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return fstream((wchar_t*)filename16.c_str(), mode);
#else
    return fstream((string)filename, mode);
#endif
}

void utls::WriteCharHexTo(char buf[2], char ch)
{
    buf[0] = (ch & 0xF0) >> 4;
    buf[0] += (buf[0] > 9 ? 'A' - 10 : '0');

    buf[1] = (ch & 0x0F);
    buf[1] += (buf[1] > 9 ? 'A' - 10 : '0');
}

// Append the char to the supplied string as hexadecimal code
void utls::WriteCharHexTo(string& str, char ch, bool clear)
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

void utls::WriteToUtf16BE(u16string& str, char32_t codePoint, bool clear)
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

#ifdef _WIN32

string utls::GetWin32ErrorMessage(unsigned rc)
{
    LPWSTR psz{ nullptr };
    const DWORD cchMsg = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL, // (not used with FORMAT_MESSAGE_FROM_SYSTEM)
        rc,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&psz),
        0,
        NULL);

    if (cchMsg == 0)
        return string();

    // Assign buffer to smart pointer with custom deleter so that memory gets released
    // in case String's c'tor throws an exception.
    auto deleter = [](void* p) { ::LocalFree(p); };
    unique_ptr<TCHAR, decltype(deleter)> ptrBuffer(psz, deleter);
    return utf8::utf16to8((char16_t*)psz);
}

#endif // _WIN322

unsigned char utls::GetCharCodeSize(unsigned ch)
{
    return (unsigned char)(std::log(ch) / std::log(256)) + 1;
}

unsigned utls::GetCharCodeMaxValue(unsigned char codeSize)
{
    return (unsigned)(std::pow(2, codeSize * CHAR_BIT)) - 1;
}

chars::chars(const string_view& view)
    : string(string(view))
{
}

chars::chars(string&& str)
    : string(std::move(str))
{
}

chars::chars()
{
}

chars::chars(size_t size)
{
    resize(size);
}

void utls::WriteUInt32BE(PdfOutputDevice& output, uint32_t value)
{
    char buf[4];
    WriteUInt32BE(buf, value);
    output.Write(string_view(buf, 4));
}

void utls::WriteInt32BE(mm::PdfOutputDevice& output, int32_t value)
{
    char buf[4];
    WriteInt32BE(buf, value);
    output.Write(string_view(buf, 4));
}

void utls::WriteUInt16BE(PdfOutputDevice& output, uint16_t value)
{
    char buf[2];
    WriteUInt16BE(buf, value);
    output.Write(string_view(buf, 2));
}

void utls::WriteInt16BE(mm::PdfOutputDevice& output, int16_t value)
{
    char buf[2];
    WriteInt16BE(buf, value);
    output.Write(string_view(buf, 2));
}

void utls::WriteUInt32BE(char* buf, uint32_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0 ) & 0xFF);
    buf[1] = static_cast<char>((value >> 8 ) & 0xFF);
    buf[2] = static_cast<char>((value >> 16) & 0xFF);
    buf[3] = static_cast<char>((value >> 24) & 0xFF);
}

void utls::WriteInt32BE(char* buf, int32_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0 ) & 0xFF);
    buf[1] = static_cast<char>((value >> 8 ) & 0xFF);
    buf[2] = static_cast<char>((value >> 16) & 0xFF);
    buf[3] = static_cast<char>((value >> 24) & 0xFF);
}

void utls::WriteUInt16BE(char* buf, uint16_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0) & 0xFF);
    buf[1] = static_cast<char>((value >> 8) & 0xFF);
}

void utls::WriteInt16BE(char* buf, int16_t value)
{
    value = AS_BIG_ENDIAN(value);
    buf[0] = static_cast<char>((value >> 0) & 0xFF);
    buf[1] = static_cast<char>((value >> 8) & 0xFF);
}

void utls::ReadUInt32BE(PdfInputDevice& input, uint32_t& value)
{
    char buf[4];
    input.Read(buf, 4);
    ReadUInt32BE(buf, value);
}

void utls::ReadInt32BE(mm::PdfInputDevice& input, int32_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadInt32BE(buf, value);
}

void utls::ReadUInt16BE(PdfInputDevice& input, uint16_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadUInt16BE(buf, value);
}

void utls::ReadInt16BE(mm::PdfInputDevice& input, int16_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadInt16BE(buf, value);
}

void utls::ReadUInt32BE(const char* buf, uint32_t& value)
{
    value =
          (uint32_t)(buf[0] & 0xFF) << 0
        | (uint32_t)(buf[1] & 0xFF) << 8
        | (uint32_t)(buf[2] & 0xFF) << 16
        | (uint32_t)(buf[3] & 0xFF) << 24;
    value = AS_BIG_ENDIAN(value);
}

void utls::ReadInt32BE(const char* buf, int32_t& value)
{
    value =
          (int32_t)(buf[0] & 0xFF) << 0
        | (int32_t)(buf[1] & 0xFF) << 8
        | (int32_t)(buf[2] & 0xFF) << 16
        | (int32_t)(buf[3] & 0xFF) << 24;
    value = AS_BIG_ENDIAN(value);
}

void utls::ReadUInt16BE(const char* buf, uint16_t& value)
{
    value =
          (uint16_t)(buf[0] & 0xFF) << 0
        | (uint16_t)(buf[1] & 0xFF) << 8;
    value = AS_BIG_ENDIAN(value);
}

void utls::ReadInt16BE(const char* buf, int16_t& value)
{
    value =
          (int16_t)(buf[0] & 0xFF) << 0
        | (int16_t)(buf[1] & 0xFF) << 8;
    value = AS_BIG_ENDIAN(value);
}
