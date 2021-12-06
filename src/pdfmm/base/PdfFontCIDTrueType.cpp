/**
 * Copyright (C) 2007 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontCIDTrueType.h"

#include <unordered_map>
#include <iostream>
#include <sstream>

#include <pdfmm/private/FreetypePrivate.h>

#include "PdfDocument.h"
#include "PdfIndirectObjectList.h"
#include "PdfArray.h"
#include "PdfDictionary.h"
#include "PdfLocale.h"
#include "PdfName.h"
#include "PdfObjectStream.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontTrueTypeSubset.h"
#include "PdfInputDevice.h"
#include "PdfOutputDevice.h"

using namespace std;
using namespace mm;

class WidthExporter
{
private:
    WidthExporter(unsigned cid, unsigned width);
public:
    static PdfArray GetPdfWidths(const CIDToGIDMap& glyphWidths,
        const PdfFontMetrics& metrics);
private:
    void update(unsigned cid, unsigned width);
    PdfArray finish();
    void reset(unsigned cid, unsigned width);
    void emitSameWidth();
    void emitArrayWidths();
    static unsigned getPdfWidth(unsigned gid, const PdfFontMetrics& metrics,
        const Matrix2D& matrix);

private:
    PdfArray m_output;
    PdfArray m_widths;         // array of consecutive different widths
    unsigned m_start;          // glyphIndex of start range
    unsigned m_width;
    unsigned m_rangeCount;     // number of processed glyphIndex'es since start of range
};

PdfFontCIDTrueType::PdfFontCIDTrueType(PdfDocument& doc, const PdfFontMetricsConstPtr& metrics,
        const PdfEncoding& encoding) :
    PdfFont(doc, metrics, encoding),
    m_descendantFont(nullptr),
    m_descriptor(nullptr)
{
}

bool PdfFontCIDTrueType::SupportsSubsetting() const
{
    return true;
}

PdfFontType PdfFontCIDTrueType::GetType() const
{
    return PdfFontType::CIDTrueType;
}

bool PdfFontCIDTrueType::TryMapCIDToGID(unsigned cid, unsigned& gid) const
{
    // TODO Read /CIDToGIDMap
    //const PdfName& subType = obj.GetDictionary().FindKey(PdfName::KeySubtype)->GetName();
    //if (subType == "CIDFontType2")
    //{
    //    auto cidToGidMap = obj.GetDictionary().FindKey("CIDToGIDMap");
    //    if (cidToGidMap != nullptr)
    //    {
    //
    //    }
    //}

    gid = cid;
    return true;
}

bool PdfFontCIDTrueType::TryMapGIDToCID(unsigned gid, unsigned& cid) const
{
    // TODO: use "/CIDToGIDMap" if present
    cid = gid;
    return true;
}

void PdfFontCIDTrueType::initImported()
{
    PdfArray arr;

    // Now setting each of the entries of the font
    this->GetObject().GetDictionary().AddKey(PdfName::KeySubtype, PdfName("Type0"));
    this->GetObject().GetDictionary().AddKey("BaseFont", PdfName(this->GetName()));

    // The descendant font is a CIDFont:
    m_descendantFont = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("Font");

    // The DecendantFonts, should be an indirect object:
    arr.push_back(m_descendantFont->GetIndirectReference());
    this->GetObject().GetDictionary().AddKey("DescendantFonts", arr);

    // Setting the /DescendantFonts
    // This is a type2 CIDFont, which has TrueType backend font
    m_descendantFont->GetDictionary().AddKey(PdfName::KeySubtype, PdfName("CIDFontType2"));

    // Same base font as the owner font:
    m_descendantFont->GetDictionary().AddKey("BaseFont", PdfName(this->GetName()));

    m_descendantFont->GetDictionary().AddKey("CIDToGIDMap", PdfName("Identity"));

    if (!IsSubsettingEnabled())
    {
        createWidths(m_descendantFont->GetDictionary(), getCIDToGIDMap(false));
        m_Encoding->ExportToFont(*this);
    }

    // The FontDescriptor, should be an indirect object:
    auto descriptorObj = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject("FontDescriptor");
    m_descendantFont->GetDictionary().AddKeyIndirect("FontDescriptor", descriptorObj);
    FillDescriptor(descriptorObj->GetDictionary());
    m_descriptor = descriptorObj;
}

void PdfFontCIDTrueType::embedFont()
{
    this->embedFontFile(*m_descriptor);
}

void PdfFontCIDTrueType::embedFontSubset()
{
    this->embedFontFile(*m_descriptor);
}

PdfObject* PdfFontCIDTrueType::getDescendantFontObject()
{
    return m_descendantFont;
}

void PdfFontCIDTrueType::embedFontFile(PdfObject& descriptor)
{
    if (IsSubsettingEnabled())
    {
        // Prepare a CID to GID for the subsetting
        CIDToGIDMap cidToGidMap = getCIDToGIDMap(true);
        createWidths(m_descendantFont->GetDictionary(), cidToGidMap);
        m_Encoding->ExportToFont(*this);

        auto& metrics = GetMetrics();
        PdfMemoryInputDevice input(metrics.GetFontFileData());
        chars buffer;

        PdfFontTrueTypeSubset::BuildFont(buffer, input, 0, cidToGidMap);
        auto contents = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
        descriptor.GetDictionary().AddKeyIndirect("FontFile2", contents);

        contents->GetDictionary().AddKey("Length1", PdfObject(static_cast<int64_t>(buffer.size())));
        contents->GetOrCreateStream().Set(buffer);
    }
    else
    {
        auto contents = this->GetObject().GetDocument()->GetObjects().CreateDictionaryObject();
        descriptor.GetDictionary().AddKeyIndirect("FontFile2", contents);

        auto fontdata = m_Metrics->GetFontFileData();

        // NOTE: Set Length1 before creating the stream
        // as PdfStreamedDocument does not allow
        // adding keys to an object after a stream was written
        contents->GetDictionary().AddKey("Length1",
            PdfObject(static_cast<int64_t>(fontdata.size())));
        contents->GetOrCreateStream().Set(fontdata);
    }
}

void PdfFontCIDTrueType::createWidths(PdfDictionary& fontDict, const CIDToGIDMap& cidToGidMap)
{
    auto& metrics = GetMetrics();
    PdfArray arr = WidthExporter::GetPdfWidths(cidToGidMap, metrics);
    if (arr.size() == 0)
        return;

    fontDict.AddKey("W", arr);
    fontDict.AddKey("DW", static_cast<int64_t>(
        std::round(GetMetrics().GetDefaultWidth() / metrics.GetMatrix()[0])));
}

CIDToGIDMap PdfFontCIDTrueType::getCIDToGIDMap(bool subsetting)
{
    CIDToGIDMap ret;
    if (subsetting)
    {
        auto& usedGIDs = GetUsedGIDs();
        for (auto& pair : usedGIDs)
        {
            unsigned gid = pair.first;
            unsigned cid = pair.second.Id;
            ret.insert(std::make_pair(cid, gid));
        }
    }
    else
    {
        unsigned gidCount = GetMetrics().GetGlyphCount();
        for (unsigned gid = 0; gid < gidCount; gid++)
        {
            unsigned cid;
            if (!TryMapGIDToCID(gid, cid))
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::InvalidFontFile, "Unable to map gid to cid");
            ret.insert(std::make_pair(cid, gid));
        }
    }

    return ret;
}

WidthExporter::WidthExporter(unsigned cid, unsigned width)
{
    reset(cid, width);
}

void WidthExporter::update(unsigned cid, unsigned width)
{
    if (cid == (m_start + m_rangeCount))
    {
        // continous gid
        if (width - m_width != 0)
        {
            // different width, so emit if previous range was with same width
            if ((m_rangeCount != 1) && m_widths.empty())
            {
                emitSameWidth();
                reset(cid, width);
                return;
            }
            m_widths.push_back(PdfObject(static_cast<int64_t>(m_width)));
            m_width = width;
            m_rangeCount++;
            return;
        }
        // two or more gids with same width
        if (!m_widths.empty())
        {
            emitArrayWidths();
            /* setup previous width as start position */
            m_start += m_rangeCount - 1;
            m_rangeCount = 2;
            return;
        }
        // consecutive range of same widths
        m_rangeCount++;
        return;
    }
    // gid gap (font subset)
    finish();
    reset(cid, width);
}

PdfArray WidthExporter::finish()
{
    // if there is a single glyph remaining, emit it as array
    if (!m_widths.empty() || m_rangeCount == 1)
    {
        m_widths.push_back(PdfObject(static_cast<int64_t>(m_width)));
        emitArrayWidths();
        return m_output;
    }
    emitSameWidth();

    return m_output;
}

PdfArray WidthExporter::GetPdfWidths(const CIDToGIDMap& cidToGidMap,
    const PdfFontMetrics& metrics)
{
    if (cidToGidMap.size() == 0)
        return PdfArray();

    auto& matrix = metrics.GetMatrix();
    auto it = cidToGidMap.begin();
    WidthExporter exporter(it->first, getPdfWidth(it->second, metrics, matrix));
    while (++it != cidToGidMap.end())
        exporter.update(it->first, getPdfWidth(it->second, metrics, matrix));

    return exporter.finish();
}

void WidthExporter::reset(unsigned cid, unsigned width)
{
    m_start = cid;
    m_width = width;
    m_rangeCount = 1;
}

void WidthExporter::emitSameWidth()
{
    m_output.push_back(static_cast<int64_t>(m_start));
    m_output.push_back(static_cast<int64_t>(m_start + m_rangeCount - 1));
    m_output.push_back(static_cast<int64_t>(std::round(m_width)));
}

void WidthExporter::emitArrayWidths()
{
    m_output.push_back(static_cast<int64_t>(m_start));
    m_output.push_back(m_widths);
    m_widths.Clear();
}

// Return thousands of PDF units
unsigned WidthExporter::getPdfWidth(unsigned gid, const PdfFontMetrics& metrics,
    const Matrix2D& matrix)
{
    return (unsigned)std::round(metrics.GetGlyphWidth(gid) / matrix[0]);
}
