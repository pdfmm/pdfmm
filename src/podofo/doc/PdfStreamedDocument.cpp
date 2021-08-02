/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "base/PdfDefinesPrivate.h"
#include "PdfStreamedDocument.h"

using namespace std;
using namespace PoDoFo;

PdfStreamedDocument::PdfStreamedDocument( PdfOutputDevice& pDevice, PdfVersion eVersion, PdfEncrypt* pEncrypt, PdfWriteMode eWriteMode )
    : m_pWriter( nullptr ), m_pDevice( nullptr ), m_pEncrypt( pEncrypt ), m_bOwnDevice( false )
{
    Init(pDevice, eVersion, pEncrypt, eWriteMode);
}

PdfStreamedDocument::PdfStreamedDocument(const string_view& filename, PdfVersion eVersion, PdfEncrypt* pEncrypt, PdfWriteMode eWriteMode )
    : m_pWriter( nullptr ), m_pEncrypt( pEncrypt ), m_bOwnDevice( true )
{
    m_pDevice = new PdfOutputDevice(filename);
    Init(*m_pDevice, eVersion, pEncrypt, eWriteMode);
}

PdfStreamedDocument::~PdfStreamedDocument()
{
    delete m_pWriter;
    if( m_bOwnDevice )
        delete m_pDevice;
}

void PdfStreamedDocument::Init(PdfOutputDevice& pDevice, PdfVersion eVersion, 
                                PdfEncrypt* pEncrypt, PdfWriteMode eWriteMode)
{
    m_pWriter = new PdfImmediateWriter( this->GetObjects(), this->GetTrailer(), pDevice, eVersion, pEncrypt, eWriteMode );
}

void PdfStreamedDocument::Close()
{
    // TODO: Check if this works correctly
	// makes sure pending subset-fonts are embedded
	GetFontCache().EmbedSubsetFonts();
    
    this->GetObjects().Finish();
}

PdfWriteMode PdfStreamedDocument::GetWriteMode() const
{
    return m_pWriter->GetWriteMode();
}

PdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_pWriter->GetPdfVersion();
}

bool PdfStreamedDocument::IsLinearized() const
{
    // Linearization is currently not supported by PdfStreamedDocument
    return false;
}

bool PdfStreamedDocument::IsPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsPrintAllowed() : true;
}

bool PdfStreamedDocument::IsEditAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditAllowed() : true;
}

bool PdfStreamedDocument::IsCopyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsCopyAllowed() : true;
}

bool PdfStreamedDocument::IsEditNotesAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditNotesAllowed() : true;
}

bool PdfStreamedDocument::IsFillAndSignAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsFillAndSignAllowed() : true;
}

bool PdfStreamedDocument::IsAccessibilityAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsAccessibilityAllowed() : true;
}

bool PdfStreamedDocument::IsDocAssemblyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsDocAssemblyAllowed() : true;
}

bool PdfStreamedDocument::IsHighPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsHighPrintAllowed() : true;
}
