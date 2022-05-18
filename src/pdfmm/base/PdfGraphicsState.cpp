/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include "PdfGraphicsState.h"

using namespace std;
using namespace mm;

PdfGraphicsState::PdfGraphicsState() :
    m_LineWidth(0),
    m_MiterLevel(10),
    m_LineCapStyle(PdfLineCapStyle::Square),
    m_LineJoinStyle(PdfLineJoinStyle::Miter)
{
}

void PdfGraphicsState::SetPropertyChangedCallback(const std::function<void(PdfGraphicsStateProperty)>& callback)
{
    m_PropertyChanged = callback;
}

void PdfGraphicsState::SetCurrentMatrix(const Matrix& matrix)
{
    if (m_CTM == matrix)
        return;

    m_CTM = matrix;
    m_PropertyChanged(PdfGraphicsStateProperty::CTM);
}

void PdfGraphicsState::SetLineWidth(double lineWidth)
{
    if (m_LineWidth == lineWidth)
        return;

    m_LineWidth = lineWidth;
    m_PropertyChanged(PdfGraphicsStateProperty::LineWidth);
}

void PdfGraphicsState::SetMiterLevel(double value)
{
    if (m_MiterLevel == value)
        return;

    m_MiterLevel = value;
    m_PropertyChanged(PdfGraphicsStateProperty::MiterLevel);
}

void PdfGraphicsState::SetLineCapStyle(PdfLineCapStyle capStyle)
{
    if (m_LineCapStyle == capStyle)
        return;

    m_LineCapStyle = capStyle;
    m_PropertyChanged(PdfGraphicsStateProperty::LineCapStyle);
}

void PdfGraphicsState::SetLineJoinStyle(PdfLineJoinStyle joinStyle)
{
    if (m_LineJoinStyle == joinStyle)
        return;

    m_LineJoinStyle = joinStyle;
    m_PropertyChanged(PdfGraphicsStateProperty::LineJoinStyle);
}

void PdfGraphicsState::SetRenderingIntent(const string_view& intent)
{
    if (m_RenderingIntent == intent)
        return;

    m_RenderingIntent = intent;
    m_PropertyChanged(PdfGraphicsStateProperty::RenderingIntent);
}

void PdfGraphicsState::SetFillColor(const PdfColor& color)
{
    if (m_FillColor == color)
        return;

    m_FillColor = color;
    m_PropertyChanged(PdfGraphicsStateProperty::FillColor);
}

void PdfGraphicsState::SetStrokeColor(const PdfColor& color)
{
    if (m_StrokeColor == color)
        return;

    m_StrokeColor = color;
    m_PropertyChanged(PdfGraphicsStateProperty::StrokeColor);
}
