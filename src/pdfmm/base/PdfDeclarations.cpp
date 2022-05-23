/**
 * Copyright (C) 2005 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>

#include <utfcpp/utf8.h>
#include <pdfmm/private/charconv_compat.h>
#include <pdfmm/private/utfcpp_extensions.h>

#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"

#ifdef _WIN32
#include <pdfmm/private/WindowsLeanMean.h>
#else
 // NOTE: There's no <cstrings>, <strings.h> is a posix header
#include <strings.h>
#endif

using namespace std;
using namespace mm;

static void removeTrailingZeroes(string& str);

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
    u16string u16str = utf8::utf8to16(u8str);
#ifdef PDFMM_IS_LITTLE_ENDIAN
    ByteSwap(u16str);
#endif
    if (clear)
        str = std::move(u16str);
    else
        str.append(u16str.data(), u16str.length());
}

void utls::ReadUtf16BEString(const bufferview& buffer, string& utf8str)
{
    utf8::u16bechariterable iterable(buffer.data(), buffer.size());
    utf8::utf16to8(iterable.begin(), iterable.end(), std::back_inserter(utf8str));
}

void utls::ReadUtf16LEString(const bufferview& buffer, string& utf8str)
{
    utf8::u16lechariterable iterable(buffer.data(), buffer.size());
    utf8::utf16to8(iterable.begin(), iterable.end(), std::back_inserter(utf8str));
}

void utls::FormatTo(string& str, float value, unsigned short precision)
{
    cmn::FormatTo(str, "{:.{}f}", value, precision);
    removeTrailingZeroes(str);
}

void utls::FormatTo(string& str, double value, unsigned short precision)
{
    cmn::FormatTo(str, "{:.{}f}", value, precision);
    removeTrailingZeroes(str);
}

string utls::ToLower(const string_view& str)
{
    string ret = (string)str;
    std::transform(ret.begin(), ret.end(), ret.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return ret;
}

string utls::Trim(const string_view& str, char ch)
{
    string ret = (string)str;
    ret.erase(std::remove(ret.begin(), ret.end(), ch), ret.end());
    return ret;
}

void utls::ByteSwap(u16string& str)
{
    for (unsigned i = 0; i < str.length(); i++)
        str[i] = (char16_t)utls::ByteSwap((uint16_t)str[i]);
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
        reinterpret_cast<LPWSTR>(&psz),
        0,
        NULL);

    if (cchMsg == 0)
        return string();

    // Assign buffer to smart pointer with custom deleter so that memory gets released
    // in case String's c'tor throws an exception.
    auto deleter = [](void* p) { ::LocalFree(p); };
    unique_ptr<WCHAR, decltype(deleter)> ptrBuffer(psz, deleter);
    return utf8::utf16to8((char16_t*)psz);
}

#endif // _WIN322

unsigned char utls::GetCharCodeSize(unsigned code)
{
    return (unsigned char)(std::log(code) / std::log(256)) + 1;
}

unsigned utls::GetCharCodeMaxValue(unsigned char codeSize)
{
    return (unsigned)(std::pow(2, codeSize * CHAR_BIT)) - 1;
}

// TODO: Optimize, maintaining string compatibility
// Use basic_string::resize_and_overwrite in C++23
// https://en.cppreference.com/w/cpp/string/basic_string/resize_and_overwrite
charbuff::charbuff() { }

charbuff::charbuff(const bufferview& view)
    : string(view.data(), view.size()) { }

charbuff::charbuff(const string& str)
    : string(str)
{
}

charbuff::charbuff(const string_view& view)
    : string(view) { }

charbuff::charbuff(string&& str)
    : string(std::move(str))
{
}

charbuff& charbuff::operator=(const bufferview& view)
{
    string::assign(view.data(), view.size());
    return *this;
}

charbuff& charbuff::operator=(const string_view& view)
{
    string::assign(view.data(), view.size());
    return *this;
}

charbuff& charbuff::operator=(const string& str)
{
    string::assign(str.data(), str.size());
    return *this;
}

charbuff& charbuff::operator=(string&& str)
{
    string::operator=(std::move(str));
    return *this;
}

charbuff::operator bufferview() const
{
    return bufferview(data(), size());
}

charbuff::charbuff(size_t size)
{
    resize(size);
}

void utls::WriteUInt32BE(PdfOutputDevice& output, uint32_t value)
{
    char buf[4];
    WriteUInt32BE(buf, value);
    output.Write(string_view(buf, 4));
}

void utls::WriteInt32BE(PdfOutputDevice& output, int32_t value)
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

void utls::WriteInt16BE(PdfOutputDevice& output, int16_t value)
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

void utls::ReadInt32BE(PdfInputDevice& input, int32_t& value)
{
    char buf[4];
    input.Read(buf, 4);
    ReadInt32BE(buf, value);
}

void utls::ReadUInt16BE(PdfInputDevice& input, uint16_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadUInt16BE(buf, value);
}

void utls::ReadInt16BE(PdfInputDevice& input, int16_t& value)
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

datahandle::datahandle() { }

datahandle::datahandle(const bufferview& view)
    : m_view(view) { }

datahandle::datahandle(const charbuff::const_ptr& buff)
    : m_view(*buff), m_buff(buff) { }

void removeTrailingZeroes(string& str)
{
    // Remove trailing zeroes
    const char* cursor = str.data();
    size_t len = str.size();
    while (cursor[len - 1] == '0')
        len--;

    if (cursor[len - 1] == '.')
        len--;

    if (len == 0)
    {
        str.resize(1);
        str[0] = '0';
    }
    else
    {
        str.resize(len);
    }
}
