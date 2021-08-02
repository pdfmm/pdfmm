/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfFileSpec.h"

#include "base/PdfDictionary.h"
#include "base/PdfInputStream.h"
#include "base/PdfObject.h"
#include "base/PdfStream.h"

#include <sstream>

using namespace std;
using namespace PoDoFo;

PdfFileSpec::PdfFileSpec(PdfDocument& doc, const string_view& filename, bool bEmbedd, bool bStripPath)
    : PdfElement(doc, "Filespec")
{
    Init(filename, bEmbedd, bStripPath);
}

PdfFileSpec::PdfFileSpec(PdfDocument& doc, const string_view& filename, const char* data, size_t size, bool bStripPath)
    : PdfElement(doc, "Filespec")
{
    Init(filename, data, size, bStripPath);
}

PdfFileSpec::PdfFileSpec(PdfObject& obj)
    : PdfElement(obj)
{
}

void PdfFileSpec::Init(const string_view& filename, bool bEmbedd, bool bStripPath)
{
    PdfObject* pEmbeddedStream;
    this->GetObject().GetDictionary().AddKey("F", this->CreateFileSpecification(MaybeStripPath(filename, bStripPath)));
    this->GetObject().GetDictionary().AddKey("UF", PdfString(MaybeStripPath(filename, true)));

    if (bEmbedd)
    {
        PdfDictionary ef;

        pEmbeddedStream = this->CreateObject("EmbeddedFile");
        this->EmbeddFile(pEmbeddedStream, filename);

        ef.AddKey("F", pEmbeddedStream->GetIndirectReference());

        this->GetObject().GetDictionary().AddKey("EF", ef);
    }
}

void PdfFileSpec::Init(const string_view& filename, const char* data, size_t size, bool bStripPath)
{
    PdfObject* pEmbeddedStream;
    this->GetObject().GetDictionary().AddKey("F", this->CreateFileSpecification(MaybeStripPath(filename, bStripPath)));
    this->GetObject().GetDictionary().AddKey("UF", PdfString(MaybeStripPath(filename, true)));

    PdfDictionary ef;

    pEmbeddedStream = this->CreateObject("EmbeddedFile");
    this->EmbeddFileFromMem(pEmbeddedStream, data, size);

    ef.AddKey("F", pEmbeddedStream->GetIndirectReference());

    this->GetObject().GetDictionary().AddKey("EF", ef);
}

PdfString PdfFileSpec::CreateFileSpecification(const string_view& filename) const
{
    // FIX-ME: The following is not Unicode compliant

    ostringstream str;
    char buff[5];

    // Construct a platform independent file specifier

    for (size_t i = 0; i < filename.size(); i++)
    {
        char ch = filename[i];
        if (ch == ':' || ch == '\\')
            ch = '/';
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_')
        {
            str.put(ch & 0xFF);
        }
        else if (ch == '/')
        {
            str.put('\\');
            str.put('\\');
            str.put('/');
        }
        else
        {
            sprintf(buff, "%02X", ch & 0xFF);
            str << buff;
        }
    }

    return PdfString(str.str());
}

void PdfFileSpec::EmbeddFile(PdfObject* pStream, const string_view& filename) const
{
    size_t size = io::FileSize(filename);

    PdfFileInputStream stream(filename);
    pStream->GetOrCreateStream().Set(stream);

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey("Size", static_cast<int64_t>(size));
    // TODO: CreationDate and ModDate
    pStream->GetDictionary().AddKey("Params", params);
}

string PdfFileSpec::MaybeStripPath(const string_view& filename, bool bStripPath) const
{
    // FIX-ME: The following is not Unicode compliant
    if (!bStripPath)
        return (string)filename;

    string_view lastFrom = filename;
    for (size_t i = 0; i < filename.size(); i++)
    {
        char ch = filename[i];
        if (
#ifdef WIN32
            ch == ':' || ch == '\\' ||
#endif // WIN32
            ch == '/')
        {
            lastFrom = ((string_view)filename).substr(i + 1);
        }
    }

    return (string)lastFrom;
}

void PdfFileSpec::EmbeddFileFromMem(PdfObject* pStream, const char* data, size_t size) const
{
    PdfMemoryInputStream memstream(reinterpret_cast<const char*>(data), size);
    pStream->GetOrCreateStream().Set(memstream);

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey("Size", static_cast<int64_t>(size));
    pStream->GetDictionary().AddKey("Params", params);
}

const PdfString& PdfFileSpec::GetFilename(bool canUnicode) const
{
    if (canUnicode && this->GetObject().GetDictionary().HasKey("UF"))
        return this->GetObject().GetDictionary().GetKey("UF")->GetString();

    if (this->GetObject().GetDictionary().HasKey("F"))
        return this->GetObject().GetDictionary().GetKey("F")->GetString();

    PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
}
