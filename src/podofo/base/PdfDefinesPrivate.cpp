#include "PdfDefinesPrivate.h"
#include "PdfDefines.h"
#include <utfcpp/utf8.h>

#ifndef WIN32
#include <cstrings>
#endif

using namespace std;
using namespace PoDoFo;

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
    PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Failed to read file size");
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
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDeviceOperation, "Stream I/O error while reading");

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
    return std::ifstream((wchar_t*)filename16.c_str(), mode);
#else
    return std::ifstream((string)filename, openmode);
#endif
}

ofstream io::open_ofstream(const string_view& filename, ios_base::openmode mode)
{
#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return std::ofstream((wchar_t*)filename16.c_str(), mode);
#else
    return std::ofstream((string)filename, openmode);
#endif
}

fstream io::open_fstream(const std::string_view& filename, std::ios_base::openmode mode)
{
#ifdef WIN32
    auto filename16 = utf8::utf8to16((string)filename);
    return std::fstream((wchar_t*)filename16.c_str(), mode);
#else
    return std::fstream((string)filename, openmode);
#endif
}
