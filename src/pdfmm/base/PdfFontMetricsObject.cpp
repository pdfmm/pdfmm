/**
 * Copyright (C) 2010 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfFontMetricsObject.h"

#include "PdfDocument.h"
#include "PdfDictionary.h"
#include "PdfName.h"
#include "PdfObject.h"
#include "PdfVariant.h"

using namespace mm;
using namespace std;

static PdfFontStretch stretchFromString(const string_view& str);

PdfFontMetricsObject::PdfFontMetricsObject(const PdfObject& font, const PdfObject* descriptor) :
    m_DefaultWidth(0),
    m_FontFileObject(nullptr),
    m_FontFileType(PdfFontFileType::Unknown)
{
    const PdfName& subType = font.GetDictionary().MustFindKey(PdfName::KeySubtype).GetName();

    // Widths of a Type 1 font, which are in thousandths
    // of a unit of text space
    m_Matrix = { 1e-3, 0.0, 0.0, 1e-3, 0, 0 };

    // /FirstChar /LastChar /Widths are in the Font dictionary and not in the FontDescriptor
    if (subType == "Type1" || subType == "Type3" || subType == "TrueType")
    {
        if (subType == "Type1")
        {
            m_FontFileType = PdfFontFileType::Type1;
        }
        else if (subType == "TrueType")
        {
            m_FontFileType = PdfFontFileType::TrueType;
        }
        else if (subType == "Type3")
        {
            // Type3 fonts don't have a /FontFile entry
            m_FontFileType = PdfFontFileType::Type3;
        }

        if (descriptor == nullptr && subType == "Type3")
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
            {
                m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile");
            }
            else if (subType == "TrueType")
            {
                m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile2");
            }

            if (subType != "Type3" && m_FontFileObject == nullptr)
            {
                m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile3");
                if (m_FontFileObject != nullptr)
                {
                    auto fontFileSubtype = m_FontFileObject->GetDictionary().FindKeyAs<PdfName>(PdfName::KeySubtype);
                    if (subType == "Type1")
                    {
                        if (fontFileSubtype == "Type1C")
                            m_FontFileType = PdfFontFileType::Type1CCF;
                        else if (fontFileSubtype == "OpenType")
                            m_FontFileType = PdfFontFileType::OpenType;
                    }
                    else if (subType == "TrueType" && fontFileSubtype == "OpenType")
                    {
                        m_FontFileType = PdfFontFileType::OpenType;
                    }
                }
            }
        }

        // Type3 fonts have a custom /FontMatrix
        const PdfObject* fontmatrix = nullptr;
        if (subType == "Type3" && (fontmatrix = font.GetDictionary().FindKey("FontMatrix")) != nullptr)
        {
            auto& fontmatrixArr = fontmatrix->GetArray();
            for (int i = 0; i < 6; i++)
                m_Matrix[i] = fontmatrixArr[i].GetReal();
        }

        auto widths = font.GetDictionary().FindKey("Widths");
        if (widths != nullptr)
        {
            auto& arrWidths = widths->GetArray();
            m_Widths.reserve(arrWidths.size());
            for (auto& obj : arrWidths)
                m_Widths.push_back(obj.GetReal() * m_Matrix[0]);
        }

        m_DefaultWidth = descriptor->GetDictionary().FindKeyAs<double>("MissingWidth", 0);
    }
    else if (subType == "CIDFontType0" || subType == "CIDFontType2")
    {
        if (descriptor == nullptr)
            PDFMM_RAISE_ERROR_INFO(PdfErrorCode::NoObject, "Missing descriptor for CID ont");

        auto obj = descriptor->GetDictionary().FindKey("FontName");
        if (obj != nullptr)
            m_FontName = obj->GetName().GetString();

        obj = descriptor->GetDictionary().FindKey("FontBBox");
        if (obj != nullptr)
            m_BBox = GetBBox(*obj);

        if (subType == "CIDFontType0"
            && (m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile")) != nullptr)
        {
            m_FontFileType = PdfFontFileType::Type1;
        }
        else if (subType == "CIDFontType2"
            && (m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile2")) != nullptr)
        {
            m_FontFileType = PdfFontFileType::TrueType;
        }

        if (m_FontFileObject == nullptr)
        {
            m_FontFileObject = descriptor->GetDictionary().FindKey("FontFile3");
            if (m_FontFileObject != nullptr)
            {
                auto fontFileSubtype = m_FontFileObject->GetDictionary().FindKeyAs<PdfName>(PdfName::KeySubtype);
                if (subType == "CIDFontType0")
                {
                    if (fontFileSubtype == "CIDFontType0C")
                        m_FontFileType = PdfFontFileType::CIDType1CCF;
                    else if (fontFileSubtype == "OpenType")
                        m_FontFileType = PdfFontFileType::OpenType;
                }
                else if (subType == "CIDFontType2" && fontFileSubtype == "OpenType")
                {
                    m_FontFileType = PdfFontFileType::OpenType;
                }
            }
        }

        m_DefaultWidth = font.GetDictionary().FindKeyAs<double>("DW", 1000.0) * m_Matrix[0];
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
                        m_Widths[start + i] = arr[i].GetReal() * m_Matrix[0];
                }
                else
                {
                    unsigned end = (unsigned)widthsArr[pos++].GetNumberLenient();
                    unsigned length = end + 1;
                    PDFMM_ASSERT(length >= start);
                    if (length > m_Widths.size())
                        m_Widths.resize(length, m_DefaultWidth);

                    double width = widthsArr[pos++].GetReal() * m_Matrix[0];
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
        // Add some sensible defaults
        m_FontFamilyName.clear();
        m_FontStretch = PdfFontStretch::Unknown;
        m_Weight = -1;
        m_Flags = PdfFontDescriptorFlags::Symbolic;
        m_ItalicAngle = 0;
        m_Ascent = 0;
        m_Descent = 0;
        m_Leading = -1;
        m_CapHeight = 0;
        m_XHeight = 0;
        m_StemV = 0;
        m_StemH = -1;
        m_AvgWidth = -1;
        m_MaxWidth = -1;
    }
    else
    {
        auto& dict = descriptor->GetDictionary();
        m_FontFamilyName = dict.FindKeyAs<PdfString>("FontFamily").GetString();
        auto stretchObj = dict.FindKey("FontStretch");
        if (stretchObj == nullptr)
        {
            m_FontStretch = PdfFontStretch::Unknown;
        }
        else
        {
            const PdfName* name;
            const PdfString* str;
            if (stretchObj->TryGetName(name))
                m_FontStretch = stretchFromString(name->GetString());
            else if (stretchObj->TryGetString(str))
                m_FontStretch = stretchFromString(name->GetString());
            else
                m_FontStretch = PdfFontStretch::Unknown;
        }

        // NOTE: Found a valid document with "/FontWeight 400.0" so just read the value as double
        m_Weight = static_cast<int>(dict.FindKeyAs<double>("FontWeight", -1));
        m_Flags = (PdfFontDescriptorFlags)dict.FindKeyAs<int64_t>("Flags", 0);
        m_ItalicAngle = static_cast<int>(dict.FindKeyAs<double>("ItalicAngle", 0));
        m_Ascent = dict.FindKeyAs<double>("Ascent", 0) * m_Matrix[3];
        m_Descent = dict.FindKeyAs<double>("Descent", 0) * m_Matrix[3];
        m_Leading = dict.FindKeyAs<double>("Leading", -1) * m_Matrix[3];
        m_CapHeight = dict.FindKeyAs<double>("CapHeight", 0) * m_Matrix[3];
        m_XHeight = dict.FindKeyAs<double>("XHeight", 0) * m_Matrix[3];
        // NOTE: StemV is measured horizzontally, StemH vertically
        m_StemV = dict.FindKeyAs<double>("StemV", 0) * m_Matrix[0];
        m_StemH = dict.FindKeyAs<double>("StemH", -1) * m_Matrix[3];
        m_AvgWidth = dict.FindKeyAs<double>("AvgWidth", -1) * m_Matrix[0];
        m_MaxWidth = dict.FindKeyAs<double>("MaxWidth", -1) * m_Matrix[0];
    }

    m_FontBaseName = PdfFont::ExtractBaseName(m_FontName, m_IsItalicHint, m_IsBoldHint);
    m_LineSpacing = m_Ascent + m_Descent;

    // Try to fine some sensible values
    m_UnderlineThickness = 1.0;
    m_UnderlinePosition = 0.0;
    m_StrikeOutThickness = m_UnderlinePosition;
    m_StrikeOutPosition = m_Ascent / 2.0;
}

string PdfFontMetricsObject::GetFontName() const
{
    return m_FontName;
}

string PdfFontMetricsObject::GetBaseFontName() const
{
    return m_FontBaseName;
}

string PdfFontMetricsObject::GetFontFamilyName() const
{
    return m_FontFamilyName;
}

PdfFontStretch PdfFontMetricsObject::GetFontStretch() const
{
    return m_FontStretch;
}

PdfFontFileType PdfFontMetricsObject::GetFontFileType() const
{
    return m_FontFileType;
}

void PdfFontMetricsObject::GetBoundingBox(vector<double>& bbox) const
{
    bbox = m_BBox;
}

unique_ptr<PdfFontMetricsObject> PdfFontMetricsObject::Create(const PdfObject& font, const PdfObject* descriptor)
{
    return unique_ptr<PdfFontMetricsObject>(new PdfFontMetricsObject(font, descriptor));
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
    // NOTE: We don't (and we won't) support retrieval of GID
    // from loaded metrics froma a codepoint. If one just needs
    // to retrieve the width of a codepoint then one map the
    // codepoint to a CID and retrieve the width directly
    gid = { };
    return false;
}

PdfFontDescriptorFlags PdfFontMetricsObject::GetFlags() const
{
    return m_Flags;
}

double PdfFontMetricsObject::GetDefaultWidthRaw() const
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

double PdfFontMetricsObject::GetLeadingRaw() const
{
    return m_Leading;
}

int PdfFontMetricsObject::GetWeightRaw() const
{
    return m_Weight;
}

double PdfFontMetricsObject::GetCapHeight() const
{
    return m_CapHeight;
}

double PdfFontMetricsObject::GetXHeightRaw() const
{
    return m_XHeight;
}

double PdfFontMetricsObject::GetStemV() const
{
    return m_StemV;
}

double PdfFontMetricsObject::GetStemHRaw() const
{
    return m_StemH;
}

double PdfFontMetricsObject::GetAvgWidthRaw() const
{
    return m_AvgWidth;
}

double PdfFontMetricsObject::GetMaxWidthRaw() const
{
    return m_MaxWidth;
}

double PdfFontMetricsObject::GetItalicAngle() const
{
    return m_ItalicAngle;
}

const Matrix2D& PdfFontMetricsObject::GetMatrix() const
{
    return m_Matrix;
}

bool PdfFontMetricsObject::getIsBoldHint() const
{
    return m_IsBoldHint;
}

bool PdfFontMetricsObject::getIsItalicHint() const
{
    return m_IsItalicHint;
}

const PdfObject* PdfFontMetricsObject::GetFontFileObject() const
{
    return m_FontFileObject;
}

vector<double> PdfFontMetricsObject::GetBBox(const PdfObject& obj)
{
    vector<double> ret;
    ret.reserve(4);
    auto& arr = obj.GetArray();
    ret.push_back(arr[0].GetNumberLenient() * m_Matrix[0]);
    ret.push_back(arr[1].GetNumberLenient() * m_Matrix[3]);
    ret.push_back(arr[2].GetNumberLenient() * m_Matrix[0]);
    ret.push_back(arr[3].GetNumberLenient() * m_Matrix[3]);
    return ret;
}

PdfFontStretch stretchFromString(const string_view& str)
{
    if (str == "UltraCondensed")
        return PdfFontStretch::UltraCondensed;
    if (str == "ExtraCondensed")
        return PdfFontStretch::ExtraCondensed;
    if (str == "Condensed")
        return PdfFontStretch::Condensed;
    if (str == "SemiCondensed")
        return PdfFontStretch::SemiCondensed;
    if (str == "Normal")
        return PdfFontStretch::Normal;
    if (str == "SemiExpanded")
        return PdfFontStretch::SemiExpanded;
    if (str == "Expanded")
        return PdfFontStretch::Expanded;
    if (str == "ExtraExpanded")
        return PdfFontStretch::ExtraExpanded;
    if (str == "UltraExpanded")
        return PdfFontStretch::UltraExpanded;
    else
        return PdfFontStretch::Unknown;
}
