/**
 * Copyright (C) 2021 by Francesco Pretto <ceztko@gmail.com>
 *
 * Licensed under GNU Lesser General Public License 2.1.
 * Some rights reserved. See COPYING, AUTHORS.
 */

#ifndef PDF_GRAPHICS_STATE_H
#define PDF_GRAPHICS_STATE_H

#include "PdfColor.h"
#include "PdfMath.h"

namespace mm
{
    enum class PdfGraphicsStateProperty
    {
        CTM,
        LineWidth,
        MiterLevel,
        LineCapStyle,
        LineJoinStyle,
        RenderingIntent,
        FillColor,
        StrokeColor,
    };

    class PDFMM_API PdfGraphicsState final
    {
        friend class PdfPainter;

    public:
        PdfGraphicsState();

    public:
        void SetCurrentMatrix(const Matrix& matrix);
        void SetLineWidth(double lineWidth);
        void SetMiterLevel(double value);
        void SetLineCapStyle(PdfLineCapStyle capStyle);
        void SetLineJoinStyle(PdfLineJoinStyle joinStyle);
        void SetRenderingIntent(const std::string_view& intent);
        void SetFillColor(const PdfColor& color);
        void SetStrokeColor(const PdfColor& color);

    public:
        const Matrix& GetCurrentMatrix() { return m_CTM; }
        double GetLineWidth() const { return m_LineWidth; }
        double GetMiterLevel() const { return m_MiterLevel; }
        PdfLineCapStyle GetLineCapStyle() const { return m_LineCapStyle; }
        PdfLineJoinStyle GetLineJoinStyle() const { return m_LineJoinStyle; }
        const std::string& GetRenderingIntent() const { return m_RenderingIntent; }
        const PdfColor& GetFillColor() const { return m_FillColor; }
        const PdfColor& GetStrokeColor() const { return m_StrokeColor; }

    private:
        void SetPropertyChangedCallback(const std::function<void(PdfGraphicsStateProperty)>& callback);

    private:
        std::function<void(PdfGraphicsStateProperty)> m_PropertyChanged;
        Matrix m_CTM;
        double m_LineWidth;
        double m_MiterLevel;
        PdfLineCapStyle m_LineCapStyle;
        PdfLineJoinStyle m_LineJoinStyle;
        std::string m_RenderingIntent;
        PdfColor m_FillColor;
        PdfColor m_StrokeColor;
    };
}

#endif // PDF_GRAPHICS_STATE_H
