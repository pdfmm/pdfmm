/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#include <pdfmm/private/PdfDeclarationsPrivate.h>
#include "PdfTextState.h"

using namespace mm;

PdfTextState::PdfTextState() :
    m_Font(nullptr),
    m_FontSize(-1),
    m_FontScale(1),
    m_CharSpacing(0),
    m_WordSpacing(0),
    m_RenderingMode(PdfTextRenderingMode::Fill),
    m_Underlined(false),
    m_StrikedOut(false)
{
    m_PropertyChanged = [](PdfTextStateProperty) { };
}

void PdfTextState::SetPropertyChangedCallback(const std::function<void(PdfTextStateProperty)>& callback)
{
    m_PropertyChanged = callback;
}

void PdfTextState::SetFont(const PdfFont* font, double fontSize)
{
    if (m_Font == font && m_FontSize == fontSize)
        return;

    m_Font = font;
    m_FontSize = fontSize;
    m_PropertyChanged(PdfTextStateProperty::Font);
}

void PdfTextState::SetFontScale(double scale)
{
    if (m_FontScale == scale)
        return;

    m_FontScale = scale;
    m_PropertyChanged(PdfTextStateProperty::FontScale);
}

void PdfTextState::SetCharSpacing(double charSpacing)
{
    if (m_CharSpacing == charSpacing)
        return;

    m_CharSpacing = charSpacing;
    m_PropertyChanged(PdfTextStateProperty::CharSpacing);
}

void PdfTextState::SetWordSpacing(double wordSpacing)
{
    if (m_WordSpacing == wordSpacing)
        return;

    m_WordSpacing = wordSpacing;
    m_PropertyChanged(PdfTextStateProperty::WordSpacing);
}

void PdfTextState::SetRenderingMode(PdfTextRenderingMode mode)
{
    if (m_RenderingMode == mode)
        return;

    m_RenderingMode = mode;
    m_PropertyChanged(PdfTextStateProperty::RenderingMode);
}
