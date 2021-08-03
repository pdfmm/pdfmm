/**
 * Copyright (C) 2006 by Dominik Seichter <domseichter@web.de>
 * Copyright (C) 2020 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Library General Public License 2.0 or later.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfDefinesPrivate.h"
#include "PdfRect.h"

#include "PdfArray.h"
#include "PdfVariant.h"

#include <iostream>
#include <sstream>
#include <iomanip>

static void CreateRect(double x1, double y1, double x2, double y2,
    double& left, double& bottom, double& width, double& height);
static void NormalizeCoordinates(double& coord1, double& coord2);

namespace PoDoFo {

PdfRect::PdfRect()
{
    m_dBottom = m_dLeft = m_dWidth = m_dHeight = 0;
}

PdfRect::PdfRect(double dLeft, double dBottom, double dWidth, double dHeight)
{
    m_dBottom = dBottom;
    m_dLeft = dLeft;
    m_dWidth = dWidth;
    m_dHeight = dHeight;
}

PdfRect PdfRect::FromCorners(double x1, double y1, double x2, double y2)
{
    PdfRect rect;
    CreateRect(x1, y1, x2, y2, rect.m_dLeft, rect.m_dBottom, rect.m_dWidth, rect.m_dHeight);
    return rect;
}

PdfRect::PdfRect(const PdfArray& inArray)
{
    m_dBottom = m_dLeft = m_dWidth = m_dHeight = 0;
    FromArray(inArray);
}

PdfRect::PdfRect(const PdfRect& rhs)
{
    this->operator=(rhs);
}

void PdfRect::ToVariant(PdfVariant& var) const
{
    PdfArray array;

    array.push_back(PdfVariant(m_dLeft));
    array.push_back(PdfVariant(m_dBottom));
    array.push_back(PdfVariant((m_dWidth + m_dLeft)));
    array.push_back(PdfVariant((m_dHeight + m_dBottom)));

    var = array;
}

std::string PdfRect::ToString() const
{
    PdfVariant  var;
    std::string str;
    this->ToVariant(var);
    var.ToString(str);

    return str;
}

void PdfRect::FromArray(const PdfArray& inArray)
{
    if (inArray.size() == 4)
    {
        double x1 = inArray[0].GetReal();
        double y1 = inArray[1].GetReal();
        double x2 = inArray[2].GetReal();
        double y2 = inArray[3].GetReal();

        CreateRect(x1, y1, x2, y2, m_dLeft, m_dBottom, m_dWidth, m_dHeight);
    }
    else
    {
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
    }
}

double PdfRect::GetRight() const
{
    return m_dLeft + m_dWidth;
}

double PdfRect::GetTop() const
{
    return m_dBottom + m_dHeight;
}

void PdfRect::Intersect(const PdfRect& rRect)
{
    if (rRect.GetBottom() != 0 || rRect.GetHeight() != 0 || rRect.GetLeft() != 0 || rRect.GetWidth() != 0)
    {
        double diff;

        diff = rRect.m_dLeft - m_dLeft;
        if (diff > 0.0)
        {
            m_dLeft += diff;
            m_dWidth -= diff;
        }

        diff = (m_dLeft + m_dWidth) - (rRect.m_dLeft + rRect.m_dWidth);
        if (diff > 0.0)
        {
            m_dWidth -= diff;
        }

        diff = rRect.m_dBottom - m_dBottom;
        if (diff > 0.0)
        {
            m_dBottom += diff;
            m_dHeight -= diff;
        }

        diff = (m_dBottom + m_dHeight) - (rRect.m_dBottom + rRect.m_dHeight);
        if (diff > 0.0)
        {
            m_dHeight -= diff;
        }
    }
}

PdfRect& PdfRect::operator=(const PdfRect& rhs)
{
    this->m_dBottom = rhs.m_dBottom;
    this->m_dLeft = rhs.m_dLeft;
    this->m_dWidth = rhs.m_dWidth;
    this->m_dHeight = rhs.m_dHeight;

    return *this;
}

};

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
