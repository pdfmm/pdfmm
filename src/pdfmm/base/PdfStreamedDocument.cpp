/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfStreamedDocument.h"

using namespace std;
using namespace mm;

PdfStreamedDocument::PdfStreamedDocument(const shared_ptr<PdfOutputDevice>& device, PdfVersion version,
        PdfEncrypt* encrypt, PdfSaveOptions opts) :
    m_Writer(nullptr),
    m_Device(device),
    m_Encrypt(encrypt)
{
    init(version, opts);
}

PdfStreamedDocument::PdfStreamedDocument(const string_view& filename, PdfVersion version,
        PdfEncrypt* encrypt, PdfSaveOptions opts) :
    m_Writer(nullptr),
    m_Device(new PdfFileOutputDevice(filename)),
    m_Encrypt(encrypt)
{
    init(version, opts);
}

void PdfStreamedDocument::init(PdfVersion version, PdfSaveOptions opts)
{
    m_Writer.reset(new PdfImmediateWriter(this->GetObjects(), this->GetTrailer().GetObject(), *m_Device, version, m_Encrypt, opts));
}

void PdfStreamedDocument::Close()
{
    GetFontManager().EmbedFonts();
    this->GetObjects().Finish();
}

PdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_Writer->GetPdfVersion();
}

void PdfStreamedDocument::SetPdfVersion(PdfVersion version)
{
    (void)version;
    PDFMM_RAISE_ERROR(PdfErrorCode::NotImplemented);
}
