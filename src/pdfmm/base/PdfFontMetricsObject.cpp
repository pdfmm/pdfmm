/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDefinesPrivate.h>
#include "PdfFontMetricsObject.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfVariant.h"

using namespace mm;
using namespace std;

PdfFontMetricsObject::PdfFontMetricsObject(const PdfObject& font, const PdfObject* descriptor) :
    PdfFontMetrics(PdfFontMetricsType::Unknown), m_DefaultWidth(0), m_FontDataObject(nullptr)
{
    const PdfName& subType = font.GetDictionary().MustFindKey(PdfName::KeySubtype).GetName();

    // Widths of a Type 1 font, which are in thousandths
    // of a unit of text space
    m_matrix = { 0.001, 0.0, 0.0, 0.001, 0, 0 };

    // /FirstChar /LastChar /Widths are in the Font dictionary and not in the FontDescriptor
    if (subType == "Type1" || subType == "Type3" || subType == "TrueType")
    {
        if (descriptor == nullptr)
        {
            if (font.GetDictionary().HasKey("Name"))
                m_FontName = font.GetDictionary().MustFindKey("Name").GetName().GetString();
            if (font.GetDictionary().HasKey("FontBBox"))
                m_BBox = GetBBox(font.GetDictionary().MustFindKey("FontBBox"));
        }
        else
        {
            if (descriptor->GetDictionary().HasKey("FontName"))
                m_FontName = descriptor->GetDictionary().MustFindKey("FontName").GetName().GetString();
            if (descriptor->GetDictionary().HasKey("FontBBox"))
                m_BBox = GetBBox(descriptor->GetDictionary().MustFindKey("FontBBox"));

            if (subType == "Type1")
                m_FontDataObject = descriptor->GetDictionary().FindKey("FontFile");
            else if (subType == "TrueType")
                m_FontDataObject = descriptor->GetDictionary().FindKey("FontFile2");
        }

        // Type3 fonts have a custom /FontMatrix
        const PdfObject* fontmatrix = nullptr;
        if (subType == "Type3" && (fontmatrix = font.GetDictionary().FindKey("FontMatrix")) != nullptr)
        {
            auto& fontmatrixArr = fontmatrix->GetArray();
            for (int i = 0; i < 6; i++)
                m_matrix[i] = fontmatrixArr[i].GetReal();
        }

        auto widths = font.GetDictionary().FindKey("Widths");
        if (widths != nullptr)
        {
            auto& arrWidths = widths->GetArray();
            m_Widths.reserve(arrWidths.size());
            for (auto& obj : arrWidths)
                m_Widths.push_back(obj.GetReal() * m_matrix[0]);
        }

        const PdfObject* missingWidth;
        if (descriptor == nullptr)
            missingWidth = font.GetDictionary().FindKey("MissingWidth");
        else
            missingWidth = descriptor->GetDictionary().FindKey("MissingWidth");

        if (missingWidth == nullptr)
        {
            if (widths == nullptr)
                PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Font object defines neither Widths, nor MissingWidth values!");
        }
        else
        {
            m_DefaultWidth = missingWidth->GetReal() * m_matrix[0];
        }
    }
    else if (subType == "CIDFontType0" || subType == "CIDFontType2")
    {
        auto obj = descriptor->GetDictionary().FindKey("FontName");
        if (obj != nullptr)
            m_FontName = obj->GetName().GetString();

        obj = descriptor->GetDictionary().FindKey("FontBBox");
        if (obj != nullptr)
            m_BBox = GetBBox(*obj);

        if (subType == "CIDFontType0")
            m_FontDataObject = descriptor->GetDictionary().FindKey("FontFile");
        else if (subType == "CIDFontType2")
            m_FontDataObject = descriptor->GetDictionary().FindKey("FontFile2");

        if (m_FontDataObject == nullptr)
            m_FontDataObject = descriptor->GetDictionary().FindKey("FontFile3");

        m_DefaultWidth = font.GetDictionary().FindKeyAs<double>("DW", 1000) * m_matrix[0];
        auto widths = font.GetDictionary().FindKey("W");
        if (widths != nullptr)
        {
            // "W" array format is described in Pdf 32000:2008 "9.7.4.3
            // Glyph Metrics in CIDFonts"
            PdfArray widthsArr = widths->GetArray();
            unsigned pos = 0;
            while (pos < widthsArr.GetSize())
            {
                unsigned start = (unsigned)widthsArr[pos++].GetNumberLenient();
                PdfObject* second = &widthsArr[pos];
                if (second->IsReference())
                {
                    // second do not have an associated owner; use the one in pw
                    second = &widths->GetDocument()->GetObjects().MustGetObject(second->GetReference());
                    PDFMM_ASSERT(!second->IsNull());
                }
                if (second->IsArray())
                {
                    PdfArray arr = second->GetArray();
                    pos++;
                    unsigned length = start + arr.GetSize();
                    PDFMM_ASSERT(length >= start);
                    if (length > m_Widths.size())
                        m_Widths.resize(length, m_DefaultWidth);

                    for (unsigned i = 0; i < arr.GetSize(); i++)
                        m_Widths[start + i] = arr[i].GetReal() * m_matrix[0];
                }
                else
                {
                    unsigned end = (unsigned)widthsArr[pos++].GetNumberLenient();
                    unsigned length = end + 1;
                    PDFMM_ASSERT(length >= start);
                    if (length > m_Widths.size())
                        m_Widths.resize(length, m_DefaultWidth);

                    double width = widthsArr[pos++].GetReal() * m_matrix[0];
                    for (unsigned i = start; i <= end; i++)
                        m_Widths[i] = width;
                }
            }
        }
    }
    else
    {
        PDFMM_RAISE_ERROR_INFO(PdfErrorCode::UnsupportedFontFormat, subType.GetEscapedName());
    }

    if (descriptor == nullptr)
    {
        m_Weight = 400;
        m_CapHeight = 0;
        m_XHeight = 0;
        m_StemV = 0;
        m_ItalicAngle = 0;
        m_Ascent = 0.0;
        m_Descent = 0.0;
    }
    else
    {
        // NOTE: Found a valid document with "/FontWeight 400.0" so just read the value as double
        m_Weight = static_cast<unsigned>(descriptor->GetDictionary().FindKeyAs<double>("FontWeight", 400));
        m_CapHeight = descriptor->GetDictionary().FindKeyAs<double>("CapHeight", 0) * m_matrix[3];
        m_XHeight = descriptor->GetDictionary().FindKeyAs<double>("XHeight", 0) * m_matrix[3];
        m_StemV = descriptor->GetDictionary().FindKeyAs<double>("StemV", 0) * m_matrix[3];
        m_ItalicAngle = static_cast<int>(descriptor->GetDictionary().FindKeyAs<double>("ItalicAngle", 0));
        m_Ascent = descriptor->GetDictionary().FindKeyAs<double>("Ascent", 0.0) * m_matrix[3];
        m_Descent = descriptor->GetDictionary().FindKeyAs<double>("Descent", 0.0) * m_matrix[3];
    }

    m_BaseName = PdfFont::ExtractBaseName(m_FontName, m_IsBold, m_IsItalic);
    m_LineSpacing = m_Ascent + m_Descent;

    // Try to fine some sensible values
    m_UnderlineThickness = 1.0;
    m_UnderlinePosition = 0.0;
    m_StrikeOutThickness = m_UnderlinePosition;
    m_StrikeOutPosition = m_Ascent / 2.0;

    m_IsSymbol = false; // TODO
}

string PdfFontMetricsObject::GetFontName() const
{
    return m_FontName;
}

string PdfFontMetricsObject::GetBaseFontName() const
{
    return m_BaseName;
}

void PdfFontMetricsObject::GetBoundingBox(vector<double>& bbox) const
{
    bbox = m_BBox;
}

unsigned PdfFontMetricsObject::GetGlyphCount() const
{
    return (unsigned)m_Widths.size();
}

bool PdfFontMetricsObject::TryGetGlyphWidth(unsigned gid, double& width) const
{
    if (gid >= m_Widths.size())
    {
        width = -1;
        return false;
    }

    width = m_Widths[gid];
    return true;
}

bool PdfFontMetricsObject::TryGetGID(char32_t codePoint, unsigned& gid) const
{
    (void)codePoint;
    // We currently don't support retrieval of GID from
    // codepoints from loaded metrics
    gid = { };
    return false;
}

double PdfFontMetricsObject::GetDefaultCharWidth() const
{
    return m_DefaultWidth;
}

double PdfFontMetricsObject::GetLineSpacing() const
{
    return m_LineSpacing;
}

double PdfFontMetricsObject::GetUnderlinePosition() const
{
    return m_UnderlinePosition;
}

double PdfFontMetricsObject::GetStrikeOutPosition() const
{
    return m_StrikeOutPosition;
}

double PdfFontMetricsObject::GetUnderlineThickness() const
{
    return m_UnderlineThickness;
}

double PdfFontMetricsObject::GetStrikeOutThickness() const
{
    return m_StrikeOutThickness;
}

double PdfFontMetricsObject::GetAscent() const
{
    return m_Ascent;
}

double PdfFontMetricsObject::GetDescent() const
{
    return m_Descent;
}

unsigned PdfFontMetricsObject::GetWeight() const
{
    return m_Weight;
}

double PdfFontMetricsObject::GetCapHeight() const
{
    return m_CapHeight;
}

double PdfFontMetricsObject::GetXHeight() const
{
    return m_XHeight;
}

double PdfFontMetricsObject::GetStemV() const
{
    return m_StemV;
}

double PdfFontMetricsObject::GetItalicAngle() const
{
    return m_ItalicAngle;
}

bool PdfFontMetricsObject::IsSymbol() const
{
    return m_IsSymbol;
}

bool PdfFontMetricsObject::IsBold() const
{
    return m_IsBold;
}

bool PdfFontMetricsObject::IsItalic() const
{
    return m_IsItalic;
}

const PdfObject* PdfFontMetricsObject::GetFontDataObject() const
{
    return m_FontDataObject;
}

vector<double> PdfFontMetricsObject::GetBBox(const PdfObject& obj)
{
    vector<double> ret;
    ret.reserve(4);
    auto& arr = obj.GetArray();
    ret.push_back(arr[0].GetNumberLenient() * m_matrix[0]);
    ret.push_back(arr[1].GetNumberLenient() * m_matrix[0]);
    ret.push_back(arr[2].GetNumberLenient() * m_matrix[0]);
    ret.push_back(arr[3].GetNumberLenient() * m_matrix[0]);
    return ret;
}
