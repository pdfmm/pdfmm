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

#include <pdfmm/base/PdfInputStream.h>
#include <pdfmm/base/PdfOutputStream.h>

#include <pdfmm/private/istringviewstream.h>

#ifdef _WIN32
#include <pdfmm/private/WindowsLeanMean.h>
#else
 // NOTE: There's no <cstrings>, <strings.h> is a posix header
#include <strings.h>
#endif

using namespace std;
using namespace mm;

constexpr unsigned BUFFER_SIZE = 4096;

static const locale s_cachedLocale("C");

static void removeTrailingZeroes(string& str);

struct VersionIdentity
{
    string_view Name;
    PdfVersion Version;
};

static VersionIdentity s_PdfVersions[] = {
    { "1.0", PdfVersion::V1_0 },
    { "1.1", PdfVersion::V1_1 },
    { "1.2", PdfVersion::V1_2 },
    { "1.3", PdfVersion::V1_3 },
    { "1.4", PdfVersion::V1_4 },
    { "1.5", PdfVersion::V1_5 },
    { "1.6", PdfVersion::V1_6 },
    { "1.7", PdfVersion::V1_7 },
    { "2.0", PdfVersion::V2_0 },
};

template<typename TInt, class = typename std::enable_if_t<std::is_integral_v<TInt>>>
void formatTo(string& str, TInt value)
{
    str.clear();
    array<char, numeric_limits<TInt>::digits10> arr;
    auto res = std::to_chars(arr.data(), arr.data() + arr.size(), value);
    str.append(arr.data(), res.ptr - arr.data());
}

PdfVersion mm::GetPdfVersion(const string_view& str)
{
    for (unsigned i = 0; i < std::size(s_PdfVersions); i++)
    {
        auto& version = s_PdfVersions[i];
        if (version.Name == str)
            return version.Version;
    }

    return PdfVersion::Unknown;
}

string_view mm::GetPdfVersionName(PdfVersion version)
{
    switch (version)
    {
        case PdfVersion::V1_0:
            return s_PdfVersions[0].Name;
        case PdfVersion::V1_1:
            return s_PdfVersions[1].Name;
        case PdfVersion::V1_2:
            return s_PdfVersions[2].Name;
        case PdfVersion::V1_3:
            return s_PdfVersions[3].Name;
        case PdfVersion::V1_4:
            return s_PdfVersions[4].Name;
        case PdfVersion::V1_5:
            return s_PdfVersions[5].Name;
        case PdfVersion::V1_6:
            return s_PdfVersions[6].Name;
        case PdfVersion::V1_7:
            return s_PdfVersions[7].Name;
        case PdfVersion::V2_0:
            return s_PdfVersions[8].Name;
        default:
            PDFMM_RAISE_ERROR(PdfErrorCode::InvalidEnumValue);
    }
}

vector<string> mm::ToPdfKeywordsList(const string_view& str)
{
    vector<string> ret;
    auto it = str.begin();
    auto tokenStart = it;
    auto end = str.end();
    string token;
    while (it != end)
    {
        auto ch = (char32_t)utf8::next(it, end);
        switch (ch)
        {
            case U'\r':
            case U'\n':
            {
                token = string(tokenStart, it);
                if (token.length() != 0)
                    ret.push_back(std::move(token));
                tokenStart = it;
                break;
            }
        }
    }

    token = string(tokenStart, it);
    if (token.length() != 0)
        ret.push_back(std::move(token));

    return ret;
}

string mm::ToPdfKeywordsString(const cspan<string>& keywords)
{
    string ret;
    bool first = true;
    for (auto& keyword : keywords)
    {
        if (first)
            first = false;
        else
            ret.append("\r\n");

        ret.append(keyword);
    }

    return ret;
}

const locale& utls::GetInvariantLocale()
{
    return s_cachedLocale;
}

bool utls::IsWhiteSpace(char32_t ch)
{
    switch (ch)
    {
        case U' ':
        case U'\n':
        case U'\t':
        case U'\v':
        case U'\f':
        case U'\r':
            return true;
        default:
            return false;
    }
}

bool utls::IsStringEmptyOrWhiteSpace(const string_view& str)
{
    auto it = str.begin();
    auto end = str.end();
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (!utls::IsWhiteSpace(cp))
            return false;
    }

    return true;
}

string utls::TrimSpacesEnd(const string_view& str)
{
    auto it = str.begin();
    auto end = str.end();
    auto prev = it;
    auto firstWhiteSpace = end;
    while (it != end)
    {
        char32_t cp = utf8::next(it, end);
        if (utls::IsWhiteSpace(cp))
        {
            if (firstWhiteSpace == end)
                firstWhiteSpace = prev;
        }
        else
        {
            firstWhiteSpace = end;
        }

        prev = it;
    }

    if (firstWhiteSpace == end)
        return (string)str;
    else
        return (string)str.substr(0, firstWhiteSpace - str.begin());
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

bool utls::TryGetHexValue(char ch, unsigned char& value)
{
    switch (ch)
    {
        case '0':
            value = 0x0;
            return true;
        case '1':
            value = 0x1;
            return true;
        case '2':
            value = 0x2;
            return true;
        case '3':
            value = 0x3;
            return true;
        case '4':
            value = 0x4;
            return true;
        case '5':
            value = 0x5;
            return true;
        case '6':
            value = 0x6;
            return true;
        case '7':
            value = 0x7;
            return true;
        case '8':
            value = 0x8;
            return true;
        case '9':
            value = 0x9;
            return true;
        case 'a':
        case 'A':
            value = 0xA;
            return true;
        case 'b':
        case 'B':
            value = 0xB;
            return true;
        case 'c':
        case 'C':
            value = 0xC;
            return true;
        case 'd':
        case 'D':
            value = 0xD;
            return true;
        case 'e':
        case 'E':
            value = 0xE;
            return true;
        case 'f':
        case 'F':
            value = 0xF;
            return true;
        default:
            value = 0x0;
            return false;
    }
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

void utls::CopyTo(ostream& dst, istream& src)
{
    if (src.eof())
        return;

    array<char, BUFFER_SIZE> buffer;
    bool eof;
    do
    {
        streamsize read = utls::ReadBuffer(src, buffer.data(), BUFFER_SIZE, eof);
        dst.write(buffer.data(), read);
    } while (!eof);
}

void utls::ReadTo(charbuff& str, const string_view& filepath)
{
    ifstream istream = utls::open_ifstream(filepath, ios_base::binary);
    ReadTo(str, istream);
}

void utls::ReadTo(charbuff& str, istream& stream)
{
    stream.seekg(0, ios::end);
    if (stream.tellg() == -1)
        throw runtime_error("Error reading from stream");

    str.reserve((size_t)stream.tellg());
    stream.seekg(0, ios::beg);

    str.assign((istreambuf_iterator<char>(stream)),
        istreambuf_iterator<char>());
}

void utls::WriteTo(const string_view& filepath, const bufferview& view)
{
    ofstream ostream = utls::open_ofstream(string(filepath), ios_base::binary);
    WriteTo(ostream, view);
}

void utls::WriteTo(ostream& stream, const bufferview& view)
{
    cmn::istringviewstream istream(view.data(), view.size());
    CopyTo(stream, istream);
}

// Read from stream an amount of bytes or less
// without setting failbit
// https://stackoverflow.com/a/22593639/213871
size_t utls::ReadBuffer(istream& stream, char* buffer, size_t size, bool& eof)
{
    PDFMM_ASSERT(!stream.eof());

    size_t read = 0;
    do
    {
        // This consistently fails on gcc (linux) 4.8.1 with failbit set on read
        // failure. This apparently never fails on VS2010 and VS2013 (Windows 7)
        read += (size_t)stream.rdbuf()->sgetn(buffer + read, (streamsize)(size - read));

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

        eof = stream.eof();

        if (read == size)
            return read;

    } while (!eof);

    return read;
}

// See utls:Read(stream, buffer, count) above
bool utls::ReadChar(istream& stream, char& ch)
{
    PDFMM_ASSERT(!stream.eof());

    streamsize read;
    do
    {
        read = stream.rdbuf()->sgetn(&ch, 1);
        (void)stream.rdstate();
        (void)stream.peek();
        if (stream.fail())
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidDeviceOperation, "Stream I/O error while reading");

        if (read == 1)
            return true;

    } while (!stream.eof());

    return false;
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

void utls::WriteUtf16BETo(u16string& str, char32_t codePoint)
{
    str.clear();
    utf8::unchecked::append16(codePoint, std::back_inserter(str));
#ifdef PDFMM_IS_LITTLE_ENDIAN
    ByteSwap(str);
#endif
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

void utls::FormatTo(string& str, signed char value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned char value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, short value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned short value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, int value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, long long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, unsigned long long value)
{
    formatTo(str, value);
}

void utls::FormatTo(string& str, float value, unsigned short precision)
{
    utls::FormatTo(str, "{:.{}f}", value, precision);
    removeTrailingZeroes(str);
}

void utls::FormatTo(string& str, double value, unsigned short precision)
{
    utls::FormatTo(str, "{:.{}f}", value, precision);
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

void utls::WriteUInt32BE(OutputStream& output, uint32_t value)
{
    char buf[4];
    WriteUInt32BE(buf, value);
    output.Write(buf, 4);
}

void utls::WriteInt32BE(OutputStream& output, int32_t value)
{
    char buf[4];
    WriteInt32BE(buf, value);
    output.Write(buf, 4);
}

void utls::WriteUInt16BE(OutputStream& output, uint16_t value)
{
    char buf[2];
    WriteUInt16BE(buf, value);
    output.Write(buf, 2);
}

void utls::WriteInt16BE(OutputStream& output, int16_t value)
{
    char buf[2];
    WriteInt16BE(buf, value);
    output.Write(buf, 2);
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

void utls::ReadUInt32BE(InputStream& input, uint32_t& value)
{
    char buf[4];
    input.Read(buf, 4);
    ReadUInt32BE(buf, value);
}

void utls::ReadInt32BE(InputStream& input, int32_t& value)
{
    char buf[4];
    input.Read(buf, 4);
    ReadInt32BE(buf, value);
}

void utls::ReadUInt16BE(InputStream& input, uint16_t& value)
{
    char buf[2];
    input.Read(buf, 2);
    ReadUInt16BE(buf, value);
}

void utls::ReadInt16BE(InputStream& input, int16_t& value)
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
