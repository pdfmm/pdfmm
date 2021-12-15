/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfRect.h"

#include "PdfArray.h"
#include "PdfVariant.h"

#include <iostream>
#include <sstream>
#include <iomanip>

static void CreateRect(double x1, double y1, double x2, double y2,
    double& left, double& bottom, double& width, double& height);
static void NormalizeCoordinates(double& coord1, double& coord2);

using namespace std;
using namespace mm;

PdfRect::PdfRect()
{
    m_Bottom = m_Left = m_Width = m_Height = 0;
}

PdfRect::PdfRect(double left, double bottom, double width, double height)
{
    m_Bottom = bottom;
    m_Left = left;
    m_Width = width;
    m_Height = height;
}

PdfRect PdfRect::FromCorners(double x1, double y1, double x2, double y2)
{
    PdfRect rect;
    CreateRect(x1, y1, x2, y2, rect.m_Left, rect.m_Bottom, rect.m_Width, rect.m_Height);
    return rect;
}

PdfRect::PdfRect(const PdfArray& arr)
{
    m_Bottom = m_Left = m_Width = m_Height = 0;
    FromArray(arr);
}

PdfRect::PdfRect(const PdfRect& rhs)
{
    this->operator=(rhs);
}

void PdfRect::ToArray(PdfArray& arr) const
{
    arr.Clear();
    arr.Add(PdfObject(m_Left));
    arr.Add(PdfObject(m_Bottom));
    arr.Add(PdfObject((m_Width + m_Left)));
    arr.Add(PdfObject((m_Height + m_Bottom)));
}

string PdfRect::ToString() const
{
    PdfArray arr;
    string str;
    this->ToArray(arr);
    PdfVariant(arr).ToString(str);
    return str;
}

void PdfRect::FromArray(const PdfArray& arr)
{
    if (arr.size() == 4)
    {
        double x1 = arr[0].GetReal();
        double y1 = arr[1].GetReal();
        double x2 = arr[2].GetReal();
        double y2 = arr[3].GetReal();

        CreateRect(x1, y1, x2, y2, m_Left, m_Bottom, m_Width, m_Height);
    }
    else
    {
        PDFMM_RAISE_ERROR(PdfErrorCode::ValueOutOfRange);
    }
}

double PdfRect::GetRight() const
{
    return m_Left + m_Width;
}

double PdfRect::GetTop() const
{
    return m_Bottom + m_Height;
}

void PdfRect::Intersect(const PdfRect& rect)
{
    if (rect.GetBottom() != 0 || rect.GetHeight() != 0 || rect.GetLeft() != 0 || rect.GetWidth() != 0)
    {
        double diff;

        diff = rect.m_Left - m_Left;
        if (diff > 0.0)
        {
            m_Left += diff;
            m_Width -= diff;
        }

        diff = (m_Left + m_Width) - (rect.m_Left + rect.m_Width);
        if (diff > 0.0)
        {
            m_Width -= diff;
        }

        diff = rect.m_Bottom - m_Bottom;
        if (diff > 0.0)
        {
            m_Bottom += diff;
            m_Height -= diff;
        }

        diff = (m_Bottom + m_Height) - (rect.m_Bottom + rect.m_Height);
        if (diff > 0.0)
        {
            m_Height -= diff;
        }
    }
}

PdfRect& PdfRect::operator=(const PdfRect& rhs)
{
    this->m_Bottom = rhs.m_Bottom;
    this->m_Left = rhs.m_Left;
    this->m_Width = rhs.m_Width;
    this->m_Height = rhs.m_Height;

    return *this;
}

void CreateRect(double x1, double y1, double x2, double y2, double& left, double& bottom, double& width, double& height)
{
    // See Pdf Reference 1.7, 3.8.4 Rectangles
    NormalizeCoordinates(x1, x2);
    NormalizeCoordinates(y1, y2);

    left = x1;
    bottom = y1;
    width = x2 - x1;
    height = y2 - y1;
}

void NormalizeCoordinates(double& coord1, double& coord2)
{
    if (coord1 > coord2)
    {
        double temp = coord1;
        coord1 = coord2;
        coord2 = temp;
    }
}
