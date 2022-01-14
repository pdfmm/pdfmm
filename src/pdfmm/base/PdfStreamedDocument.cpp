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

PdfStreamedDocument::PdfStreamedDocument(PdfOutputDevice& device, PdfVersion version, PdfEncrypt* encrypt, PdfSaveOptions opts)
    : m_Writer(nullptr), m_Device(nullptr), m_Encrypt(encrypt), m_OwnDevice(false)
{
    Init(device, version, encrypt, opts);
}

PdfStreamedDocument::PdfStreamedDocument(const string_view& filename, PdfVersion version, PdfEncrypt* encrypt, PdfSaveOptions opts)
    : m_Writer(nullptr), m_Encrypt(encrypt), m_OwnDevice(true)
{
    m_Device = new PdfFileOutputDevice(filename);
    Init(*m_Device, version, encrypt, opts);
}

PdfStreamedDocument::~PdfStreamedDocument()
{
    delete m_Writer;
    if (m_OwnDevice)
        delete m_Device;
}

void PdfStreamedDocument::Init(PdfOutputDevice& device, PdfVersion version,
    PdfEncrypt* encrypt, PdfSaveOptions opts)
{
    m_Writer = new PdfImmediateWriter(this->GetObjects(), this->GetTrailer(), device, version, encrypt, opts);
}

void PdfStreamedDocument::Close()
{
    // TODO: Check if this works correctly
    // makes sure pending subset-fonts are embedded
    GetFontManager().HandleSave();

    this->GetObjects().Finish();
}

PdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_Writer->GetPdfVersion();
}

bool PdfStreamedDocument::IsPrintAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsPrintAllowed() : true;
}

bool PdfStreamedDocument::IsEditAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsEditAllowed() : true;
}

bool PdfStreamedDocument::IsCopyAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsCopyAllowed() : true;
}

bool PdfStreamedDocument::IsEditNotesAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsEditNotesAllowed() : true;
}

bool PdfStreamedDocument::IsFillAndSignAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsFillAndSignAllowed() : true;
}

bool PdfStreamedDocument::IsAccessibilityAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsAccessibilityAllowed() : true;
}

bool PdfStreamedDocument::IsDocAssemblyAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsDocAssemblyAllowed() : true;
}

bool PdfStreamedDocument::IsHighPrintAllowed() const
{
    return m_Encrypt ? m_Encrypt->IsHighPrintAllowed() : true;
}
